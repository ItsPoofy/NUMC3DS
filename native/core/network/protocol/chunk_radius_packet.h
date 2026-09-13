#ifndef NUMC3DS_MCPE_CHUNK_RADIUS_PACKET_H
#define NUMC3DS_MCPE_CHUNK_RADIUS_PACKET_H

#include "mcpe_protocol.h"
#include "mcpe_wire.h"

typedef struct {
    mcpe_u32 radius;
} McpeChunkRadiusPacketView;

int mcpe_request_chunk_radius_packet_encode(
    const McpeChunkRadiusPacketView *packet,
    void *output, mcpe_u32 capacity, mcpe_u32 *length);
int mcpe_request_chunk_radius_packet_decode(
    const void *payload, mcpe_u32 length,
    McpeChunkRadiusPacketView *packet);
int mcpe_chunk_radius_updated_packet_encode(
    const McpeChunkRadiusPacketView *packet,
    void *output, mcpe_u32 capacity, mcpe_u32 *length);
int mcpe_chunk_radius_updated_packet_decode(
    const void *payload, mcpe_u32 length,
    McpeChunkRadiusPacketView *packet);

#endif
