#include "mcpe_handshake_activation.h"
#include "mcpe_peer_encryption.h"
#include "../auth/mcpe_handshake_crypto.h"
#include "../auth/mcpe_identity.h"
#include "../session/mcpe_transport_selector.h"
#include "../../commands/native_types.h"
#include "../../hook_manager.h"
#include "../../rt.h"
#include "../../../diagnostics/network_debug.h"

typedef void (*ServerHandshakeHandlerFn)(void *, void *, void *);
typedef void *(*GetEncryptedPeerForUserFn)(void *, void *);
typedef void (*NetworkHandlerOnHandshakeFn)(void *, void *);
typedef void (*NetworkHandlerSendFn)(void *, void *, void *);

static NuMC3DS_Hook server_handshake_hook;

static int native_string_view(const NativeGstdString *value, const char **bytes, u32 *length)
{
    u32 handle;
    if (!value || !bytes || !length || !(handle = value->handle)) return 0;
    *bytes = (const char *)handle;
    *length = *(const u32 *)(handle - 4u);
    return 1;
}

static void handshake_failure(const char *message, u32 length)
{
    net_log_open(NET_LOG_INFO, "handshake", "rejected");
    net_log_text("reason", message);
    net_log_dec("length", length);
    net_log_close();
}

static void on_server_handshake(void *handler, void *identifier, void *packet)
{
    ServerHandshakeHandlerFn original;
    GetEncryptedPeerForUserFn get_peer;
    McpeIdentity *identity;
    const char *public_key;
    const char *salt;
    u32 public_key_length;
    u32 salt_length;
    mcpe_u8 key[MCPE_CRYPTO_AES256_KEY_BYTES];
    void *peer;
    void *network_handler;
    struct {
        u32 vtable;
        u8 reliability;
        u8 client_sub_id;
        u8 pad[6];
    } handshake_pkt;
    static const char invalid_packet[] = "NuMC3DS: invalid MCPE handshake packet\n";
    static const char crypto_failed[] = "NuMC3DS: MCPE handshake crypto rejected\n";

    original = (ServerHandshakeHandlerFn)server_handshake_hook.trampoline;
    if (mcpe_transport_selector_kind() != MCPE_TRANSPORT_IPV4) {
        original(handler, identifier, packet);
        return;
    }
    if (!handler || !identifier || !packet ||
        !native_string_view((const NativeGstdString *)((const u8 *)packet + 8u),
                            &public_key, &public_key_length) ||
        !native_string_view((const NativeGstdString *)((const u8 *)packet + 12u),
                            &salt, &salt_length) ||
        public_key_length > 512u || salt_length != 16u) {
        handshake_failure(invalid_packet, sizeof(invalid_packet) - 1u);
        return;
    }
    network_handler = *(void **)((u8 *)handler + 4u);
    get_peer = (GetEncryptedPeerForUserFn)SEAM_NetworkHandler_getEncryptedPeerForUser;
    peer = get_peer(network_handler, identifier);
    if (!peer || !mcpe_identity_get(&identity) ||
        !mcpe_handshake_derive_key(identity, public_key, public_key_length,
                                   (const mcpe_u8 *)salt, key) ||
        !mcpe_peer_encryption_enable(peer, key)) {
        handshake_failure(crypto_failed, sizeof(crypto_failed) - 1u);
        return;
    }
    ((NetworkHandlerOnHandshakeFn)SEAM_NetworkHandler_onHandshake)(network_handler, identifier);
    *(u8 *)((u8 *)handler + 29u) = 1;
    zero(&handshake_pkt, sizeof(handshake_pkt));
    handshake_pkt.vtable = SEAM_ClientToServerHandshakePacket_vtable;
    handshake_pkt.reliability = 2;
    handshake_pkt.client_sub_id = 1;
    ((NetworkHandlerSendFn)SEAM_NetworkHandler_send)(network_handler, identifier, &handshake_pkt);
}

int mcpe_handshake_activation_install(void)
{
    zero(&server_handshake_hook, sizeof(server_handshake_hook));
    server_handshake_hook.target = SEAM_ClientNetworkHandler_handleServerToClientHandshakePacket;
    server_handshake_hook.replacement = (u32)on_server_handshake;
    server_handshake_hook.expected[0] = 0xE92D4030u;
    server_handshake_hook.expected[1] = 0xE1A04000u;
    if (s->host.install_hook(&server_handshake_hook)) return -80;
    return 0;
}
