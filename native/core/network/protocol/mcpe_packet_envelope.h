#ifndef NUMC3DS_MCPE_PACKET_ENVELOPE_H
#define NUMC3DS_MCPE_PACKET_ENVELOPE_H

#include "mcpe_protocol.h"
#include "mcpe_wire.h"

typedef struct {
    mcpe_u32 packet_id;
    const void *payload;
    mcpe_u32 payload_length;
} McpePacketEnvelopeView;

typedef struct {
    mcpe_u32 packet_id;
    const mcpe_u8 *payload;
    mcpe_u32 payload_length;
} McpePacketEnvelopeTarget;

int mcpe_packet_envelope_encode(const McpePacketEnvelopeView *packet,
                                void *output, mcpe_u32 capacity,
                                mcpe_u32 *length);
int mcpe_packet_envelope_decode(const void *payload, mcpe_u32 length,
                                McpePacketEnvelopeTarget *packet);

#endif
