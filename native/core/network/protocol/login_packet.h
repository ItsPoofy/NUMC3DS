#ifndef NUMC3DS_MCPE_LOGIN_PACKET_H
#define NUMC3DS_MCPE_LOGIN_PACKET_H

#include "mcpe_protocol.h"
#include "mcpe_wire.h"

typedef struct {
    mcpe_s32 protocol_version;
    mcpe_u8 client_network_version;
    const mcpe_u8 *connection_request;
    mcpe_u32 connection_request_length;
} McpeLoginPacketView;

int mcpe_login_packet_encode(const McpeLoginPacketView *packet,
                             void *output, mcpe_u32 capacity, mcpe_u32 *length);
int mcpe_login_packet_decode(const void *payload, mcpe_u32 length,
                             McpeLoginPacketView *packet,
                             mcpe_u8 *connection_request,
                             mcpe_u32 connection_request_capacity);

#endif
