#include "mcpe_protocol_override.h"
#include "mcpe_protocol.h"
#include "../../../diagnostics/network_debug.h"
#include "../auth/mcpe_native_login.h"
#include "../session/mcpe_transport_selector.h"
#include "../../hook_manager.h"
#include "../../seams.h"
#include "../../state.h"

typedef void (*NetworkHandlerSendFn)(void *, void *, void *);
typedef void (*LoginPacketWriteFn)(void *, void *, void *, void *);
typedef void (*BinaryStreamWriteBigEndianIntFn)(void *, u32);
typedef void (*BinaryStreamWriteByteFn)(void *, u32);
typedef void (*BinaryStreamWriteStringFn)(void *, void *);

static NuMC3DS_Hook network_handler_send_hook;
static NuMC3DS_Hook login_packet_write_hook;
static NativeGstdString *prepared_login_payload;

static int mcpe_protocol_override_enabled(void)
{
    return mcpe_transport_selector_kind() == MCPE_TRANSPORT_IPV4;
}

static int native_string_view(const NativeGstdString *value, const char **bytes, u32 *length)
{
    u32 handle;
    if (!value || !bytes || !length || !(handle = value->handle)) return 0;
    *bytes = (const char *)handle;
    *length = *(const u32 *)(handle - 4u);
    return 1;
}

static void on_network_handler_send(void *handler, void *identifier, void *packet)
{
    NetworkHandlerSendFn original;
    u32 packet_id;
    void **vtable;
    NativeGstdString payload;
    int prepared = 0;

    if (!packet || !mcpe_protocol_override_enabled()) {
        ((NetworkHandlerSendFn)network_handler_send_hook.trampoline)(handler, identifier, packet);
        return;
    }
    vtable = *(void ***)packet;
    if (!vtable || !vtable[2]) {
        ((NetworkHandlerSendFn)network_handler_send_hook.trampoline)(handler, identifier, packet);
        return;
    }
    packet_id = ((u32 (*)(void *))vtable[2])(packet);
    net_log_open(NET_LOG_VERBOSE, "send", "packet");
    net_log_dec("id", packet_id);
    net_log_hex("packet", (u32)packet);
    net_log_close();
    if (packet_id == MCPE_PACKET_LOGIN) {
        char username_buf[64];
        const char *username = 0;
        void *login_data = *(void **)((char *)packet + 12);
        if (login_data) {
            NativeGstdString *p_str = (NativeGstdString *)((char *)login_data + 16);
            const char *raw_name = 0;
            u32 name_len = 0;
            if (native_string_view(p_str, &raw_name, &name_len) && raw_name && name_len > 0) {
                u32 copy_len = name_len < sizeof(username_buf) - 1 ? name_len : sizeof(username_buf) - 1;
                u32 i;
                for (i = 0; i < copy_len; ++i) {
                    username_buf[i] = raw_name[i];
                }
                username_buf[copy_len] = '\0';
                username = username_buf;
            }
        }
        if (username) {
            net_log_open(NET_LOG_INFO, "send", "login_username");
            net_log_text("name", username);
            net_log_close();
        }
        zero(&payload, sizeof(payload));
        if (prepared_login_payload || !mcpe_native_login_build_payload(&payload, username)) {
            net_log_open(NET_LOG_INFO, "send", "login_payload_rejected");
            net_log_close();
            return;
        }
        net_log_open(NET_LOG_INFO, "send", "login_payload_ready");
        net_log_close();
        prepared_login_payload = &payload;
        prepared = 1;
    }
    original = (NetworkHandlerSendFn)network_handler_send_hook.trampoline;
    original(handler, identifier, packet);
    if (prepared) {
        prepared_login_payload = 0;
        ((void (*)(void *))SEAM_StrDtor)(&payload);
    }
}

static void on_login_packet_write(void *packet, void *stream, void *arg2, void *arg3)
{
    if (!mcpe_protocol_override_enabled()) {
        ((LoginPacketWriteFn)login_packet_write_hook.trampoline)(packet, stream, arg2, arg3);
        return;
    }
    if (!prepared_login_payload) return;
    net_log_open(NET_LOG_INFO, "send", "login_packet_write");
    net_log_dec("protocol", MCPE_PROTOCOL_VERSION);
    net_log_close();
    ((BinaryStreamWriteBigEndianIntFn)SEAM_BinaryStream_writeSignedBigEndianInt)(stream,
                                                                                  MCPE_PROTOCOL_VERSION);
    ((BinaryStreamWriteByteFn)SEAM_BinaryStream_writeByte)(stream, 0u);
    ((BinaryStreamWriteStringFn)SEAM_BinaryStream_writeString)(stream, prepared_login_payload);
}

int mcpe_protocol_override_install(void)
{
    volatile u32 *protocol = (volatile u32 *)SEAM_game_protocol_version;
    prepared_login_payload = 0;
    /* This mod is an MCPE 1.1.5 update: the whole title now speaks protocol 113.
       Writing the game's own protocol global makes the stock writers, readers and
       ClientInstance_startExternalNetworkWorld's version guard all agree on 113,
       so no per-packet protocol redirection is needed. */
    *protocol = MCPE_PROTOCOL_VERSION;
    net_log_open(NET_LOG_INFO, "protocol", "set_version");
    net_log_dec("value", *protocol);
    net_log_close();

    zero(&login_packet_write_hook, sizeof(login_packet_write_hook));
    login_packet_write_hook.target = SEAM_LoginPacket_write;
    login_packet_write_hook.replacement = (u32)on_login_packet_write;
    login_packet_write_hook.expected[0] = 0xE92D4038u;
    login_packet_write_hook.expected[1] = 0xE1A05001u;
    if (s->host.install_hook(&login_packet_write_hook)) return -63;

    zero(&network_handler_send_hook, sizeof(network_handler_send_hook));
    network_handler_send_hook.target = SEAM_NetworkHandler_send;
    network_handler_send_hook.replacement = (u32)on_network_handler_send;
    network_handler_send_hook.expected[0] = 0xE92D41F0u;
    network_handler_send_hook.expected[1] = 0xE280601Cu;
    if (s->host.install_hook(&network_handler_send_hook)) return -61;
    return 0;
}
