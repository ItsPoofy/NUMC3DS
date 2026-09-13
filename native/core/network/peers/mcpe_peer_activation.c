#include "mcpe_peer_activation.h"
#include "../session/mcpe_transport_selector.h"
#include "../../hook_manager.h"
#include "../../seams.h"
#include "../../state.h"

typedef void *(*ConnectionConstructorFn)(void *, void *, void *, u32, u32, u32, int, int);
typedef void (*PeerToggleFn)(void *, int, int);

static NuMC3DS_Hook connection_constructor_hook;

static void activate_mcpe_wrappers(void *connection)
{
    void *compressed;
    void *batched;
    if (!connection || mcpe_transport_selector_kind() != MCPE_TRANSPORT_IPV4) return;
    compressed = *(void **)((u8 *)connection + 0x2cu);
    batched = *(void **)((u8 *)connection + 0x30u);
    if (!compressed || !batched) return;
    ((PeerToggleFn)SEAM_CompressedNetworkPeer_setCompression)(compressed, 1, 1);
    ((PeerToggleFn)SEAM_BatchedNetworkPeer_setBatching)(batched, 1, 1);
}

static void *on_connection_constructor(void *connection, void *identifier, void *peer,
                                       u32 reliability, u32 address_low, u32 address_high,
                                       int direct_peer, int observer)
{
    ConnectionConstructorFn original;
    void *result;
    original = (ConnectionConstructorFn)connection_constructor_hook.trampoline;
    result = original(connection, identifier, peer, reliability, address_low, address_high,
                      direct_peer, observer);
    activate_mcpe_wrappers(result);
    return result;
}

int mcpe_peer_activation_install(void)
{
    zero(&connection_constructor_hook, sizeof(connection_constructor_hook));
    connection_constructor_hook.target = SEAM_NetworkHandler_Connection_constructor;
    connection_constructor_hook.replacement = (u32)on_connection_constructor;
    connection_constructor_hook.expected[0] = 0xE92D47F0u;
    connection_constructor_hook.expected[1] = 0xE1A04000u;
    if (s->host.install_hook(&connection_constructor_hook)) return -64;
    return 0;
}
