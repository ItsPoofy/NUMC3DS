#ifndef NUMC3DS_MCPE_RESOURCE_PACK_CHUNK_REQUEST_PACKET_H
#define NUMC3DS_MCPE_RESOURCE_PACK_CHUNK_REQUEST_PACKET_H

#include "mcpe_protocol.h"
#include "mcpe_wire.h"

typedef struct {
    const char *pack_id;
    mcpe_u32 pack_id_length;
    mcpe_u32 chunk_index;
} McpeResourcePackChunkRequestView;

typedef struct {
    char *pack_id;
    mcpe_u32 pack_id_length;
    mcpe_u32 pack_id_capacity;
    mcpe_u32 chunk_index;
} McpeResourcePackChunkRequestTarget;

int mcpe_resource_pack_chunk_request_encode(
    const McpeResourcePackChunkRequestView *packet,
    void *output, mcpe_u32 capacity, mcpe_u32 *length);
int mcpe_resource_pack_chunk_request_decode(
    const void *payload, mcpe_u32 length,
    McpeResourcePackChunkRequestTarget *packet);

#endif
