#include "mcpe_transport_selector.h"
#include "../../../diagnostics/network_debug.h"
#include "../raknet/stock_socket_adapter.h"
#include "../../commands/native_types.h"
#include "../../hook_manager.h"
#include "../../seams.h"
#include "../../state.h"
#include "../../skin/remote_skin_service.h"

typedef void (*SelectorStrCtorFn)(void *, const char *, u32 *);
typedef void (*SelectorStrAssignFn)(void *, void *);
typedef void (*SelectorStrDtorFn)(void *);

typedef int (*NetworkHandlerHostFn)(void *, int, int, int);
typedef int (*NetworkHandlerConnectFn)(void *, short *, void *);
typedef void (*RakNetInstanceDisconnectFn)(void *);
typedef void (*NetworkHandlerDisconnectFn)(void *);
typedef void (*ClientNetworkCallbackFn)(void *, void *, void *, void *);
typedef int (*LocalWirelessNetworkStartFn)(void *);
typedef int (*LocalWirelessNetworkPollFn)(void *, int);
typedef u32 (*LocalWirelessNetworkGetFlagsFn)(u32 *);
typedef void (*LocalWirelessNetworkFinalizeFn)(void *);
typedef void (*ScreenChooserSetDisconnectFn)(void *, const NativeGstdString *, const NativeGstdString *, const NativeGstdString *);
static NuMC3DS_Hook host_hook;
static NuMC3DS_Hook connect_hook;
static NuMC3DS_Hook disconnect_hook;
static NuMC3DS_Hook nh_disconnect_hook;
static NuMC3DS_Hook unable_to_connect_hook;
static NuMC3DS_Hook wireless_start_hook;
static NuMC3DS_Hook wireless_poll_hook;
static NuMC3DS_Hook wireless_flags_hook;
static NuMC3DS_Hook wireless_finalize_hook;
static NuMC3DS_Hook disconnect_screen_hook;
static enum McpeTransportKind active_kind;
static char server_address[96];
static int remote_available;
static u32 remote_address_be;
static u16 remote_port_be;

static const char *selector_kind_name(enum McpeTransportKind kind)
{
    return kind == MCPE_TRANSPORT_IPV4 ? "ipv4" : "native";
}

static void capture_server_address(const void *connection)
{
    u32 handle;
    u32 length;
    u32 port;
    if (!connection) return;
    handle = *(const u32 *)((const u8 *)connection + 4u);
    port = *(const u32 *)((const u8 *)connection + 8u);
    if (!handle) {
        server_address[0] = 0;
        net_log_open(NET_LOG_VERBOSE, "transport", "capture");
        net_log_text("host", "<null>");
        net_log_dec("port", port);
        net_log_close();
        return;
    }
    length = *(const u32 *)(handle - 4u);
    if (length >= sizeof(server_address)) length = sizeof(server_address) - 1u;
    cp(server_address, (const void *)handle, length);
    server_address[length] = 0;
    net_log_open(NET_LOG_VERBOSE, "transport", "capture");
    net_log_text("host", server_address);
    net_log_dec("port", port);
    net_log_close();
}

static void selector_format_ipv4(u32 address_be, char *output, u32 capacity)
{
    u32 octets[4];
    u32 length = 0;
    u32 index;
    if (!output || capacity < 8u) return;
    octets[0] = (address_be >> 24) & 0xffu;
    octets[1] = (address_be >> 16) & 0xffu;
    octets[2] = (address_be >> 8) & 0xffu;
    octets[3] = address_be & 0xffu;
    for (index = 0; index < 4u; ++index) {
        char digits[3];
        u32 count = 0;
        u32 value = octets[index];
        do { digits[count++] = (char)('0' + (value % 10u)); value /= 10u; } while (value && count < 3u);
        while (count > 0 && length + 1u < capacity) output[length++] = digits[--count];
        if (index != 3u && length + 1u < capacity) output[length++] = '.';
    }
    output[length] = 0;
}

/* The stock connection object carries the fake UDS address (10.0.0.1:14).
   Rewrite its host string and port to the discovered MCPE endpoint so the
   RakNet peer actually targets the phone. */
static void apply_remote_endpoint(void *connection)
{
    u32 address_be = 0;
    u16 port_be = 0;
    NativeGstdString host;
    u32 scratch = 0;
    char text[32];
    if (!connection) return;
    if (!mcpe_transport_selector_remote_endpoint(&address_be, &port_be)) return;
    selector_format_ipv4(address_be, text, sizeof(text));
    ((SelectorStrCtorFn)SEAM_StrCtor)(&host, text, &scratch);
    if (host.handle) {
        ((SelectorStrAssignFn)SEAM_StrAssign)((u8 *)connection + 4u, &host);
        ((SelectorStrDtorFn)SEAM_StrDtor)(&host);
    }
    *(u32 *)((u8 *)connection + 8u) = (u32)port_be;
    net_log_open(NET_LOG_INFO, "transport", "endpoint_override");
    net_log_text("host", text);
    net_log_dec("port", port_be);
    net_log_close();
}

static int mcpe_transport_selector_set_kind(enum McpeTransportKind kind)
{
    int result;
    if (kind == MCPE_TRANSPORT_IPV4) {
        result = mcpe_stock_socket_adapter_set_ip_mode(1);
        if (result) {
            net_log_open(NET_LOG_INFO, "transport", "set_kind");
            net_log_text("kind", "ipv4");
            net_log_signed("result", result);
            net_log_close();
            return result;
        }
        active_kind = MCPE_TRANSPORT_IPV4;
        remote_skin_service_reset();
    } else {
        result = mcpe_stock_socket_adapter_set_ip_mode(0);
        if (result) {
            net_log_open(NET_LOG_INFO, "transport", "set_kind");
            net_log_text("kind", "native");
            net_log_signed("result", result);
            net_log_close();
            return result;
        }
        active_kind = MCPE_TRANSPORT_NATIVE;
    }
    net_log_open(NET_LOG_VERBOSE, "transport", "set_kind");
    net_log_text("kind", selector_kind_name(active_kind));
    net_log_signed("result", 0);
    net_log_close();
    return 0;
}

static int on_network_handler_host(void *handler, int port, int port_v6, int max_connections)
{
    NetworkHandlerHostFn original;
    int result;
    original = (NetworkHandlerHostFn)host_hook.trampoline;
    net_log_open(NET_LOG_INFO, "transport", "host");
    net_log_hex("handler", (u32)handler);
    net_log_close();
    (void)mcpe_transport_selector_set_kind(MCPE_TRANSPORT_NATIVE);
    result = original(handler, port, port_v6, max_connections);
    net_log_open(NET_LOG_INFO, "transport", "host_result");
    net_log_signed("result", result);
    net_log_close();
    return result;
}

static int on_network_handler_connect(void *handler, short *connection, void *local)
{
    NetworkHandlerConnectFn original;
    unsigned short type = 0xffffu;
    int selected = 0;
    net_log_open(NET_LOG_INFO, "transport", "connect");
    net_log_hex("handler", (u32)handler);
    net_log_hex("connection", (u32)connection);
    net_log_hex("local", (u32)local);
    if (connection) {
        type = (unsigned short)*connection;
        net_log_dec("type", (u32)type);
        if (type == 1u || type == 2u || type == 5u) {
            selected = MCPE_TRANSPORT_IPV4;
        } else if (type == 0u || type == 4u) {
            selected = MCPE_TRANSPORT_NATIVE;
        }
        net_log_text("select", selected ? selector_kind_name((enum McpeTransportKind)selected)
                                        : "unsupported");
    }
    net_log_close();
    original = (NetworkHandlerConnectFn)connect_hook.trampoline;
    if (connection) {
        if (type == 1u || type == 2u || type == 5u) {
            void *net;
            apply_remote_endpoint(connection);
            capture_server_address(connection);
            (void)mcpe_transport_selector_set_kind(MCPE_TRANSPORT_IPV4);
            net = ((void *(*)(void))SEAM_LocalWirelessNetwork_getOrCreate)();
            if (net) {
                *(u16 *)((u8 *)net + 0x118u) = 3u;
                net_log_open(NET_LOG_INFO, "transport", "state_transition_connected");
                net_log_hex("network", (u32)net);
                net_log_close();
            }
        } else if (type == 0u || type == 4u) {
            server_address[0] = 0;
            (void)mcpe_transport_selector_set_kind(MCPE_TRANSPORT_NATIVE);
        }
    }
    {
        int result = original(handler, connection, local);
        net_log_open(NET_LOG_INFO, "transport", "connect_result");
        net_log_signed("result", result);
        net_log_close();
        return result;
    }
}

static void on_raknet_instance_disconnect(void *instance)
{
    RakNetInstanceDisconnectFn original;
    void *net;
    net_log_open(NET_LOG_INFO, "transport", "disconnect");
    net_log_hex("instance", (u32)instance);
    net_log_hex("caller", (u32)__builtin_return_address(0));
    net_log_text("kind", selector_kind_name(active_kind));
    net_log_close();
    original = (RakNetInstanceDisconnectFn)disconnect_hook.trampoline;
    original(instance);
    server_address[0] = 0;
    (void)mcpe_transport_selector_set_kind(MCPE_TRANSPORT_NATIVE);
    remote_skin_service_reset();
    net = ((void *(*)(void))SEAM_LocalWirelessNetwork_getOrCreate)();
    if (net) {
        *(u16 *)((u8 *)net + 0x118u) = remote_available ? 1u : 0u;
    }
}

static void on_network_handler_disconnect(void *handler)
{
    net_log_open(NET_LOG_INFO, "transport", "nh_disconnect");
    net_log_hex("handler", (u32)handler);
    net_log_hex("caller", (u32)__builtin_return_address(0));
    net_log_close();
    remote_skin_service_reset();
    ((NetworkHandlerDisconnectFn)nh_disconnect_hook.trampoline)(handler);
}

static void on_client_unable_to_connect(void *a, void *b, void *c, void *d)
{
    mcpe_transport_selector_clear_session();
    ((ClientNetworkCallbackFn)unable_to_connect_hook.trampoline)(a, b, c, d);
}

static void selector_log_gstd_string(const char *key, const NativeGstdString *str)
{
    if (str && str->handle) {
        u32 len = *(const u32 *)(str->handle - 4u);
        char buf[68];
        if (len > 64u) len = 64u;
        cp(buf, (const void *)str->handle, len);
        buf[len] = 0;
        net_log_text(key, buf);
    } else {
        net_log_text(key, "<null>");
    }
}

static void on_screen_chooser_set_disconnect_screen(void *chooser, const NativeGstdString *title,
                                                    const NativeGstdString *message,
                                                    const NativeGstdString *param)
{
    net_log_open(NET_LOG_INFO, "transport", "set_disconnect_screen");
    net_log_hex("chooser", (u32)chooser);
    net_log_hex("caller", (u32)__builtin_return_address(0));
    selector_log_gstd_string("title", title);
    selector_log_gstd_string("message", message);
    selector_log_gstd_string("param", param);
    net_log_text("kind", selector_kind_name(active_kind));
    net_log_close();
    remote_skin_service_reset();
    ((ScreenChooserSetDisconnectFn)disconnect_screen_hook.trampoline)(chooser, title, message, param);
}

static int on_local_wireless_network_start(void *network)
{
    LocalWirelessNetworkStartFn original;
    int result;
    if (remote_available) {
        if (network) {
            *(u16 *)((u8 *)network + 0x118u) = 1u;
        }
        net_log_open(NET_LOG_INFO, "transport", "uds_start_skipped");
        net_log_hex("network", (u32)network);
        net_log_close();
        return 0;
    }
    original = (LocalWirelessNetworkStartFn)wireless_start_hook.trampoline;
    result = original(network);
    net_log_open(NET_LOG_VERBOSE, "transport", "uds_start");
    net_log_hex("network", (u32)network);
    net_log_signed("result", result);
    net_log_flag("remote_available", remote_available);
    net_log_close();
    return result;
}

static int on_local_wireless_network_poll(void *network, int flag)
{
    if (remote_available || active_kind == MCPE_TRANSPORT_IPV4) {
        return 1;
    }
    return ((LocalWirelessNetworkPollFn)wireless_poll_hook.trampoline)(network, flag);
}

static u32 on_local_wireless_network_get_availability_flags(u32 *flags)
{
    if (remote_available || active_kind == MCPE_TRANSPORT_IPV4) {
        if (flags) *flags = 0u;
        return 0u;
    }
    return ((LocalWirelessNetworkGetFlagsFn)wireless_flags_hook.trampoline)(flags);
}

static void on_local_wireless_network_finalize_if_state_one(void *network)
{
    if (remote_available || active_kind == MCPE_TRANSPORT_IPV4) {
        if (network) {
            *(u16 *)((u8 *)network + 0x118u) = 0u;
        }
        net_log_open(NET_LOG_INFO, "transport", "uds_finalize_skipped");
        net_log_hex("network", (u32)network);
        net_log_close();
        return;
    }
    ((LocalWirelessNetworkFinalizeFn)wireless_finalize_hook.trampoline)(network);
}

static int on_check_wireless_error(void)
{
    if (active_kind == MCPE_TRANSPORT_IPV4) {
        return 0;
    }
    return ((int (*)(void))SEAM_MinecraftGame_checkWirelessError)();
}

void mcpe_transport_selector_set_remote_available(int available)
{
    remote_available = available != 0;
}

int mcpe_transport_selector_remote_available(void)
{
    return remote_available;
}

void mcpe_transport_selector_set_remote_endpoint(u32 address_be, u16 port_be)
{
    remote_address_be = address_be;
    remote_port_be = port_be;
}

int mcpe_transport_selector_remote_endpoint(u32 *address_be, u16 *port_be)
{
    if (address_be) *address_be = remote_address_be;
    if (port_be) *port_be = remote_port_be;
    return remote_address_be != 0;
}

void mcpe_transport_selector_clear_session(void)
{
    void *net;
    server_address[0] = 0;
    (void)mcpe_transport_selector_set_kind(MCPE_TRANSPORT_NATIVE);
    net = ((void *(*)(void))SEAM_LocalWirelessNetwork_getOrCreate)();
    if (net) {
        *(u16 *)((u8 *)net + 0x118u) = remote_available ? 1u : 0u;
    }
}

static int selector_install_hook(NuMC3DS_Hook *hook, u32 target, u32 replacement,
                                 u32 expected0, u32 expected1, const char *name)
{
    int result;
    zero(hook, sizeof(*hook));
    hook->target = target;
    hook->replacement = replacement;
    hook->expected[0] = expected0;
    hook->expected[1] = expected1;
    result = s->host.install_hook(hook);
    net_log_open(NET_LOG_INFO, "transport", "install_hook");
    net_log_text("name", name);
    net_log_hex("target", target);
    net_log_signed("result", result);
    net_log_close();
    return result;
}

int mcpe_transport_selector_install(void)
{
    if (selector_install_hook(&host_hook, SEAM_NetworkHandler_host,
                              (u32)on_network_handler_host,
                              0xE92D4010u, 0xE1A04000u, "NetworkHandler_host")) return -77;
    if (selector_install_hook(&connect_hook, SEAM_NetworkHandler_connect,
                              (u32)on_network_handler_connect,
                              0xE92D4070u, 0xE1A06000u, "NetworkHandler_connect")) return -78;
    if (selector_install_hook(&disconnect_hook, SEAM_RakNetInstance_disconnect,
                              (u32)on_raknet_instance_disconnect,
                              0xE92D47F0u, 0xE1A05000u, "RakNetInstance_disconnect")) return -79;
    if (selector_install_hook(&nh_disconnect_hook, SEAM_NetworkHandler_disconnect,
                              (u32)on_network_handler_disconnect,
                              0xE92D4010u, 0xE1A04000u, "NetworkHandler_disconnect")) return -89;
    if (selector_install_hook(&unable_to_connect_hook, SEAM_ClientNetworkHandler_onUnableToConnect,
                              (u32)on_client_unable_to_connect,
                              0xE92D40F0u, 0xE24DD01Cu, "ClientNetworkHandler_onUnableToConnect")) return -92;
    if (selector_install_hook(&wireless_start_hook, SEAM_LocalWirelessNetwork_start,
                              (u32)on_local_wireless_network_start,
                              0xE92D4070u, 0xE1A05000u, "LocalWirelessNetwork_start")) return -85;
    if (selector_install_hook(&wireless_poll_hook, SEAM_LocalWirelessNetwork_pollConnectionStatus,
                              (u32)on_local_wireless_network_poll,
                              0xE92D4070u, 0xE24DD030u, "LocalWirelessNetwork_pollConnectionStatus")) return -86;
    if (selector_install_hook(&wireless_flags_hook, SEAM_LocalWirelessNetwork_getAvailabilityFlags,
                              (u32)on_local_wireless_network_get_availability_flags,
                              0xE92D4070u, 0xE1A04000u, "LocalWirelessNetwork_getAvailabilityFlags")) return -87;
    if (selector_install_hook(&wireless_finalize_hook, SEAM_LocalWirelessNetwork_finalizeIfStateOne,
                              (u32)on_local_wireless_network_finalize_if_state_one,
                              0xE92D4070u, 0xE2805C01u, "LocalWirelessNetwork_finalizeIfStateOne")) return -90;
    if (selector_install_hook(&disconnect_screen_hook, SEAM_ScreenChooser_setDisconnectScreen,
                              (u32)on_screen_chooser_set_disconnect_screen,
                              0xE92D43F0u, 0xE24DD014u, "ScreenChooser_setDisconnectScreen")) return -91;
    if (hook_manager_patch_call(SEAM_MinecraftGame_checkWirelessError_call,
                                0xEB0A5F62u,
                                (u32)on_check_wireless_error)) return -88;
    active_kind = MCPE_TRANSPORT_NATIVE;
    server_address[0] = 0;
    net_log_open(NET_LOG_INFO, "transport", "install_done");
    net_log_text("kind", selector_kind_name(active_kind));
    net_log_close();
    return 0;
}

int mcpe_transport_selector_kind(void)
{
    return (int)active_kind;
}

const char *mcpe_transport_selector_server_address(void)
{
    return server_address;
}
