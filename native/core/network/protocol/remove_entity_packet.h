#ifndef NUMC3DS_MCPE_REMOVE_ENTITY_PACKET_H
#define NUMC3DS_MCPE_REMOVE_ENTITY_PACKET_H

#include "mcpe_protocol.h"
#include "mcpe_wire.h"

typedef struct {
    McpeWireU64 entity_unique_id;
} McpeRemoveEntityPacketView;

int mcpe_remove_entity_packet_encode(
    const McpeRemoveEntityPacketView *packet,
    void *output, mcpe_u32 capacity, mcpe_u32 *length);
int mcpe_remove_entity_packet_decode(
    const void *payload, mcpe_u32 length,
    McpeRemoveEntityPacketView *packet);

#endif
