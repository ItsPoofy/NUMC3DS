#ifndef NUMC3DS_MCPE_SESSION_OWNER_H
#define NUMC3DS_MCPE_SESSION_OWNER_H

#include "mcpe_session.h"
#include "../platform/datagram_backend.h"
#include "../raknet/raknet_transport.h"

typedef struct {
    McpeDatagramBackend *datagram;
    McpeRaknetTransport raknet;
    McpeSession session;
    McpeDatagramEndpoint remote;
    mcpe_u8 active;
    mcpe_u8 hosting;
    mcpe_u8 has_remote;
    mcpe_u8 reserved;
} McpeSessionOwner;

void mcpe_session_owner_init(McpeSessionOwner *owner, McpeDatagramBackend *datagram);
int mcpe_session_owner_host(McpeSessionOwner *owner, mcpe_u16 port, int allow_broadcast);
int mcpe_session_owner_connect(McpeSessionOwner *owner,
                               const McpeDatagramEndpoint *remote,
                               mcpe_u16 local_port);
int mcpe_session_owner_tick(McpeSessionOwner *owner, mcpe_u32 timeout_ms);
int mcpe_session_owner_disconnect(McpeSessionOwner *owner);

#endif
