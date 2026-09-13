#include "stock_socket_adapter.h"
#include "../../../diagnostics/network_debug.h"
#include "../platform/datagram_backend.h"
#include "../platform/soc_udp_backend.h"
#include "../protocol/mcpe_protocol.h"
#include "../../hook_manager.h"
#include "../../seams.h"
#include "../../state.h"

enum {
    STOCK_RNS2_MODE_OFFSET = 0x3c,
    STOCK_RNS2_DESCRIPTOR_OFFSET = 0x1c,
    STOCK_RNS2_ADDRESS_OFFSET = 0x0c,
    STOCK_RNS2_RECEIVE_CAPACITY = 0x5d4,
    STOCK_RNS2_RECEIVE_SOURCE_PORT = 14
};

typedef int (*StockSocketInitializeFn)(void *, void *);
typedef int (*StockSocketSendFn)(void *, void *);
typedef int (*StockSocketReceiveFn)(void *, void *, u32, u32 *);
typedef void *(*SystemAddressConstructorFn)(void *, const char *, u32);
typedef int (*SystemAddressFromStringFn)(void *, const char *, int);
typedef void *(*StockSocketDestructorFn)(void *);
typedef void (*StockSocketDeleteDestructorFn)(void *);

static McpeDatagramBackend *datagram_backend;
static NuMC3DS_Hook initialize_hook;
static NuMC3DS_Hook send_hook;
static NuMC3DS_Hook receive_hook;
static NuMC3DS_Hook system_address_constructor_hook;
static NuMC3DS_Hook system_address_from_string_hook;
static NuMC3DS_Hook destructor_hook;
static NuMC3DS_Hook delete_destructor_hook;
static u32 backend_ready;
static u32 ip_mode;
static u32 socket_count;
static McpeDatagramEndpoint last_source;
static u32 have_last_source;
static u32 receive_poll_count;

static u16 byte_swap16(u16 value)
{
    return (u16)((value << 8) | (value >> 8));
}

static u32 byte_swap32(u32 value)
{
    return ((value & 0x000000ffu) << 24) |
           ((value & 0x0000ff00u) << 8) |
           ((value & 0x00ff0000u) >> 8) |
           ((value & 0xff000000u) >> 24);
}

static int same_text(const char *left, const char *right)
{
    u32 index;
    if (!left || !right) return 0;
    for (index = 0; index < 32; ++index) {
        if (left[index] != right[index]) return 0;
        if (!left[index]) return 1;
    }
    return 0;
}

static void ensure_backend(void)
{
    if (backend_ready) return;
    datagram_backend = mcpe_soc_udp_shared_backend();
    backend_ready = 1;
}

static int start_backend(u16 local_port)
{
    int result;
    ensure_backend();
    result = mcpe_datagram_backend_start(datagram_backend, local_port, 0);
    return result == MCPE_DATAGRAM_OK;
}

static void stop_backend_if_unused(void)
{
    if (socket_count != 0) return;
    /* The shared SOC context/service is process-lifetime; never tear it down
       (SOCU_Shutdown disturbs the stock network state). Just drop the cached
       sender identity. */
    have_last_source = 0;
}

static void populate_ip_system_address(void *address, const McpeDatagramEndpoint *endpoint)
{
    *(u8 *)address = 2;
    *(u16 *)((u8 *)address + 2) = byte_swap16(endpoint->port_be);
    *(u32 *)((u8 *)address + 4) = byte_swap32(endpoint->address_be);
    *(u16 *)((u8 *)address + 8) = endpoint->port_be;
    *(u16 *)((u8 *)address + 10) = 0xffffu;
}

static int on_stock_socket_initialize(void *socket, void *descriptor)
{
    u32 index;
    u16 local_port;
    int result;
    StockSocketInitializeFn original;
    SystemAddressConstructorFn address_constructor;

    original = (StockSocketInitializeFn)initialize_hook.trampoline;
    net_log_open(NET_LOG_INFO, "socket", "initialize");
    net_log_hex("socket", (u32)socket);
    net_log_hex("descriptor", (u32)descriptor);
    net_log_dec("ip_mode", ip_mode);
    net_log_close();
    if (!ip_mode) return original(socket, descriptor);
    if (!socket || !descriptor) return -1;
    local_port = *(u16 *)descriptor;
    if (!start_backend(local_port)) {
        net_log_open(NET_LOG_INFO, "socket", "initialize_ip_failed");
        net_log_dec("port", local_port);
        net_log_close();
        /* Leave the socket in the stock native state so the rest of the RNS2
           object is fully initialized instead of a half-built IP socket. */
        return original(socket, descriptor);
    }
    for (index = 0; index < 8; ++index) {
        *(u32 *)((u8 *)socket + STOCK_RNS2_DESCRIPTOR_OFFSET + index * 4) =
            ((u32 *)descriptor)[index];
    }
    *(u8 *)((u8 *)socket + STOCK_RNS2_MODE_OFFSET) = 2;
    address_constructor = (SystemAddressConstructorFn)SEAM_RakNet_SystemAddress_constructorFromString;
    address_constructor((u8 *)socket + STOCK_RNS2_ADDRESS_OFFSET, "0.0.0.0",
                        local_port);
    result = 0;
    ++socket_count;
    net_log_open(NET_LOG_INFO, "socket", "initialize_ip_ok");
    net_log_dec("port", local_port);
    net_log_dec("socket_count", socket_count);
    net_log_close();
    return result;
}

static int on_stock_socket_send(void *socket, void *parameters)
{
    StockSocketSendFn original;
    McpeDatagramEndpoint destination;
    void *payload;
    void *address;
    u32 length;
    int result;

    original = (StockSocketSendFn)send_hook.trampoline;
    if (!ip_mode || !socket || *(u8 *)((u8 *)socket + STOCK_RNS2_MODE_OFFSET) != 2) {
        return original(socket, parameters);
    }
    if (!parameters) return -1;
    payload = *(void **)parameters;
    length = *(u32 *)((u8 *)parameters + 4);
    address = (u8 *)parameters + 8;
    if (*(u8 *)address != 2 || length > STOCK_RNS2_RECEIVE_CAPACITY) return -1;
    destination.address_be = byte_swap32(*(u32 *)((u8 *)address + 4));
    destination.port_be = byte_swap16(*(u16 *)((u8 *)address + 2));
    destination.reserved = 0;
    result = mcpe_datagram_backend_send(datagram_backend, &destination, payload, length);
    net_log_open(NET_LOG_VERBOSE, "socket", "send");
    net_log_dec("length", length);
    net_log_ipv4("dest", destination.address_be, destination.port_be);
    net_log_signed("result", result);
    net_log_bytes("payload", payload, length);
    net_log_close();
    return result == MCPE_DATAGRAM_OK ? (int)length : -1;
}

static int on_stock_socket_receive(void *network, void *receive, u32 capacity, u32 *length)
{
    StockSocketReceiveFn original;
    McpeDatagramEndpoint source;
    u32 received = 0;
    int result;

    original = (StockSocketReceiveFn)receive_hook.trampoline;
    if (!ip_mode) return original(network, receive, capacity, length);
    if (length) *length = 0;
    have_last_source = 0;
    if (!receive || !length || !capacity || capacity > STOCK_RNS2_RECEIVE_CAPACITY) return 0;
    result = mcpe_datagram_backend_receive(datagram_backend, &source, receive, capacity, &received);
    if (result == MCPE_DATAGRAM_WOULD_BLOCK) {
        ++receive_poll_count;
        if (receive_poll_count <= 64u || (receive_poll_count & 0x3fu) == 0u) {
            net_log_open(NET_LOG_VERBOSE, "socket", "receive_poll");
            net_log_dec("calls", receive_poll_count);
            net_log_dec("result", (u32)result);
            net_log_close();
        }
        return 0;
    }
    if (result != MCPE_DATAGRAM_OK) return 0;
    *length = received;
    last_source = source;
    have_last_source = received != 0;
    net_log_open(NET_LOG_VERBOSE, "socket", "receive");
    net_log_dec("length", received);
    net_log_ipv4("source", source.address_be, source.port_be);
    net_log_bytes("payload", receive, received > 64u ? 64u : received);
    net_log_close();
    return 1;
}

static int adapter_parse_ipv4(const char *text, u32 *out)
{
    u32 value = 0;
    u32 octet = 0;
    int octets = 0;
    int digits = 0;
    const char *p = text;
    if (!text || !out) return 0;
    while (*p) {
        char c = *p;
        if (c >= '0' && c <= '9') {
            octet = octet * 10u + (u32)(c - '0');
            if (octet > 255u) return 0;
            digits = 1;
            ++p;
        } else if (c == '.') {
            if (!digits) return 0;
            value = (value << 8) | octet;
            octet = 0;
            digits = 0;
            if (++octets > 3) return 0;
            ++p;
        } else {
            break;
        }
    }
    if (!digits || octets != 3) return 0;
    *out = (value << 8) | octet;
    return 1;
}

/* The 3DS RakNet_SystemAddress_fromString is a stub: it parses only the port and
   forces the IPv4 field to 0 (the UDS layer owns the address). Restore the MCPE
   behaviour by parsing the dotted quad into the address field. */
static int on_system_address_from_string(void *address, const char *text, int separator)
{
    SystemAddressFromStringFn original;
    u32 parsed = 0;
    int result;
    original = (SystemAddressFromStringFn)system_address_from_string_hook.trampoline;
    result = original(address, text, separator);
    if (ip_mode && address && adapter_parse_ipv4(text, &parsed)) {
        *(u32 *)((u8 *)address + 4u) = byte_swap32(parsed);
    }
    return result;
}

static void *on_system_address_constructor(void *address, const char *text, u32 port)
{
    SystemAddressConstructorFn original;
    u32 parsed = 0;
    if (ip_mode && have_last_source && port == STOCK_RNS2_RECEIVE_SOURCE_PORT &&
        same_text(text, "10.0.0.1")) {
        original = (SystemAddressConstructorFn)system_address_constructor_hook.trampoline;
        original(address, "0.0.0.0", last_source.port_be);
        populate_ip_system_address(address, &last_source);
        net_log_open(NET_LOG_VERBOSE, "socket", "address_substitute");
        net_log_text("text", text);
        net_log_dec("port", port);
        net_log_ipv4("source", last_source.address_be, last_source.port_be);
        net_log_close();
        have_last_source = 0;
        return address;
    }
    original = (SystemAddressConstructorFn)system_address_constructor_hook.trampoline;
    original(address, text, port);
    if (ip_mode && address && adapter_parse_ipv4(text, &parsed)) {
        *(u32 *)((u8 *)address + 4u) = byte_swap32(parsed);
    }
    return address;
}

static void release_socket_reference(int ip_socket)
{
    if (!ip_socket) return;
    if (socket_count) --socket_count;
    net_log_open(NET_LOG_VERBOSE, "socket", "release");
    net_log_dec("socket_count", socket_count);
    net_log_close();
    stop_backend_if_unused();
}

static void *on_stock_socket_destructor(void *socket)
{
    StockSocketDestructorFn original;
    int ip_socket = socket && *(u8 *)((u8 *)socket + STOCK_RNS2_MODE_OFFSET) == 2;
    original = (StockSocketDestructorFn)destructor_hook.trampoline;
    socket = original(socket);
    release_socket_reference(ip_socket);
    return socket;
}

static void on_stock_socket_delete_destructor(void *socket)
{
    StockSocketDeleteDestructorFn original;
    int ip_socket = socket && *(u8 *)((u8 *)socket + STOCK_RNS2_MODE_OFFSET) == 2;
    original = (StockSocketDeleteDestructorFn)delete_destructor_hook.trampoline;
    original(socket);
    release_socket_reference(ip_socket);
}

int mcpe_stock_socket_adapter_install(void)
{
    ensure_backend();

    zero(&initialize_hook, sizeof(initialize_hook));
    initialize_hook.target = SEAM_RakNetSocket2_3DS_initialize;
    initialize_hook.replacement = (u32)on_stock_socket_initialize;
    initialize_hook.expected[0] = 0xE92D4030u;
    initialize_hook.expected[1] = 0xE1A05001u;
    if (s->host.install_hook(&initialize_hook)) return -71;

    zero(&send_hook, sizeof(send_hook));
    send_hook.target = SEAM_RakNetSocket2_3DS_send;
    send_hook.replacement = (u32)on_stock_socket_send;
    send_hook.expected[0] = 0xE92D4010u;
    send_hook.expected[1] = 0xE5D0003Cu;
    if (s->host.install_hook(&send_hook)) return -72;

    zero(&receive_hook, sizeof(receive_hook));
    receive_hook.target = SEAM_RakNetSocket2_3DS_receiveFrom;
    receive_hook.replacement = (u32)on_stock_socket_receive;
    receive_hook.expected[0] = 0xE92D4010u;
    receive_hook.expected[1] = 0xE5900008u;
    if (s->host.install_hook(&receive_hook)) return -73;

    zero(&system_address_constructor_hook, sizeof(system_address_constructor_hook));
    system_address_constructor_hook.target = SEAM_RakNet_SystemAddress_constructorFromString;
    system_address_constructor_hook.replacement = (u32)on_system_address_constructor;
    system_address_constructor_hook.expected[0] = 0xE92D4070u;
    system_address_constructor_hook.expected[1] = 0xE1A04000u;
    if (s->host.install_hook(&system_address_constructor_hook)) return -74;

    zero(&system_address_from_string_hook, sizeof(system_address_from_string_hook));
    system_address_from_string_hook.target = SEAM_RakNet_SystemAddress_fromString;
    system_address_from_string_hook.replacement = (u32)on_system_address_from_string;
    system_address_from_string_hook.expected[0] = 0xE92D4070u;
    system_address_from_string_hook.expected[1] = 0xE1A05001u;
    if (s->host.install_hook(&system_address_from_string_hook)) return -86;

    zero(&destructor_hook, sizeof(destructor_hook));
    destructor_hook.target = SEAM_RakNetSocket2_3DS_destructor;
    destructor_hook.replacement = (u32)on_stock_socket_destructor;
    destructor_hook.expected[0] = 0xE92D4010u;
    destructor_hook.expected[1] = 0xE1A04000u;
    if (s->host.install_hook(&destructor_hook)) return -75;

    zero(&delete_destructor_hook, sizeof(delete_destructor_hook));
    delete_destructor_hook.target = SEAM_RakNetSocket2_3DS_destructorDelete;
    delete_destructor_hook.replacement = (u32)on_stock_socket_delete_destructor;
    delete_destructor_hook.expected[0] = 0xE92D4010u;
    delete_destructor_hook.expected[1] = 0xE1A04000u;
    if (s->host.install_hook(&delete_destructor_hook)) return -76;
    net_log_open(NET_LOG_INFO, "socket", "install_done");
    net_log_close();
    return 0;
}

int mcpe_stock_socket_adapter_set_ip_mode(int enabled)
{
    net_log_open(NET_LOG_INFO, "socket", "set_ip_mode");
    net_log_dec("enabled", enabled ? 1u : 0u);
    net_log_dec("socket_count", socket_count);
    if (socket_count && !enabled) {
        net_log_signed("result", -1);
        net_log_close();
        return -1;
    }
    ip_mode = enabled ? 1u : 0u;
    if (!ip_mode) stop_backend_if_unused();
    net_log_dec("ip_mode", ip_mode);
    net_log_close();
    return 0;
}

int mcpe_stock_socket_adapter_ip_mode(void)
{
    return ip_mode != 0;
}
