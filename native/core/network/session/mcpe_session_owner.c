#include "mcpe_session_owner.h"

static void mcpe_session_owner_clear_endpoint(McpeDatagramEndpoint *endpoint)
{
    if (!endpoint) return;
    endpoint->address_be = 0;
    endpoint->port_be = 0;
    endpoint->reserved = 0;
}

void mcpe_session_owner_init(McpeSessionOwner *owner, McpeDatagramBackend *datagram)
{
    if (!owner) return;
    owner->datagram = datagram;
    owner->raknet.datagram = 0;
    owner->raknet.active = 0;
    owner->active = 0;
    owner->hosting = 0;
    owner->has_remote = 0;
    owner->reserved = 0;
    mcpe_session_init(&owner->session);
    mcpe_session_owner_clear_endpoint(&owner->remote);
}

static int mcpe_session_owner_start(McpeSessionOwner *owner,
                                    int hosting,
                                    mcpe_u16 local_port,
                                    int allow_broadcast,
                                    const McpeDatagramEndpoint *remote)
{
    McpeRaknetConfig config;
    int result;
    if (!owner || !owner->datagram || owner->active) return MCPE_SESSION_INVALID_ARGUMENT;
    if (hosting && !local_port) return MCPE_SESSION_INVALID_ARGUMENT;
    if (!hosting && (!remote || !remote->port_be)) return MCPE_SESSION_INVALID_ARGUMENT;
    result = mcpe_datagram_backend_start(owner->datagram, local_port, allow_broadcast);
    if (result != MCPE_DATAGRAM_OK) return result;
    mcpe_raknet_default_config(&config);
    result = mcpe_raknet_transport_init(&owner->raknet, owner->datagram, &config);
    if (result != MCPE_DATAGRAM_OK) {
        mcpe_datagram_backend_stop(owner->datagram);
        return result;
    }
    result = mcpe_session_begin(&owner->session, hosting, MCPE_PROTOCOL_VERSION);
    if (result != MCPE_SESSION_OK) {
        mcpe_raknet_transport_shutdown(&owner->raknet);
        mcpe_datagram_backend_stop(owner->datagram);
        return result;
    }
    owner->hosting = hosting ? 1 : 0;
    owner->has_remote = remote ? 1 : 0;
    if (remote) owner->remote = *remote;
    owner->active = 1;
    return MCPE_SESSION_OK;
}

int mcpe_session_owner_host(McpeSessionOwner *owner, mcpe_u16 port, int allow_broadcast)
{
    return mcpe_session_owner_start(owner, 1, port, allow_broadcast, 0);
}

int mcpe_session_owner_connect(McpeSessionOwner *owner,
                               const McpeDatagramEndpoint *remote,
                               mcpe_u16 local_port)
{
    return mcpe_session_owner_start(owner, 0, local_port, 0, remote);
}

int mcpe_session_owner_tick(McpeSessionOwner *owner, mcpe_u32 timeout_ms)
{
    if (!owner || !owner->active || !owner->datagram) return MCPE_SESSION_INVALID_STATE;
    return mcpe_datagram_backend_poll(owner->datagram, timeout_ms);
}

int mcpe_session_owner_disconnect(McpeSessionOwner *owner)
{
    int result = MCPE_SESSION_OK;
    if (!owner) return MCPE_SESSION_INVALID_ARGUMENT;
    if (owner->active) {
        result = mcpe_session_disconnect(&owner->session);
        mcpe_raknet_transport_shutdown(&owner->raknet);
        if (owner->datagram) mcpe_datagram_backend_stop(owner->datagram);
        owner->active = 0;
    }
    owner->hosting = 0;
    owner->has_remote = 0;
    mcpe_session_owner_clear_endpoint(&owner->remote);
    return result;
}
