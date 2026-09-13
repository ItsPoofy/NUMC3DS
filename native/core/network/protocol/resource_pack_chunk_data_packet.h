#ifndef NUMC3DS_MCPE_RESOURCE_PACK_CHUNK_DATA_PACKET_H
#define NUMC3DS_MCPE_RESOURCE_PACK_CHUNK_DATA_PACKET_H

#include "mcpe_protocol.h"
#include "mcpe_wire.h"

typedef struct {
    const char *pack_id;
    mcpe_u32 pack_id_length;
    mcpe_u32 chunk_index;
    McpeWireU64 progress;
    const mcpe_u8 *data;
    mcpe_u32 data_length;
} McpeResourcePackChunkDataView;

typedef struct {
    char *pack_id;
    mcpe_u32 pack_id_length;
    mcpe_u32 pack_id_capacity;
    mcpe_u32 chunk_index;
    McpeWireU64 progress;
    const mcpe_u8 *data;
    mcpe_u32 data_length;
} McpeResourcePackChunkDataTarget;

int mcpe_resource_pack_chunk_data_encode(
    const McpeResourcePackChunkDataView *packet,
    void *output, mcpe_u32 capacity, mcpe_u32 *length);
int mcpe_resource_pack_chunk_data_decode(
    const void *payload, mcpe_u32 length,
    McpeResourcePackChunkDataTarget *packet);

#endif
