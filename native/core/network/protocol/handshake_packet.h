#ifndef NUMC3DS_MCPE_HANDSHAKE_PACKET_H
#define NUMC3DS_MCPE_HANDSHAKE_PACKET_H

#include "mcpe_protocol.h"
#include "mcpe_wire.h"

#define MCPE_HANDSHAKE_SALT_BYTES 16u

typedef struct {
    const mcpe_u8 *public_key;
    mcpe_u32 public_key_length;
    const mcpe_u8 *salt;
    mcpe_u32 salt_length;
} McpeServerToClientHandshakeView;

int mcpe_server_to_client_handshake_encode(const McpeServerToClientHandshakeView *packet,
                                           void *output, mcpe_u32 capacity,
                                           mcpe_u32 *length);
int mcpe_server_to_client_handshake_decode(const void *payload, mcpe_u32 length,
                                           McpeServerToClientHandshakeView *packet);

int mcpe_client_to_server_handshake_encode(void *output, mcpe_u32 capacity,
                                           mcpe_u32 *length);
int mcpe_client_to_server_handshake_decode(const void *payload, mcpe_u32 length);

#endif
