#ifndef NUMC3DS_MCPE_RESOURCE_PACK_DATA_INFO_PACKET_H
#define NUMC3DS_MCPE_RESOURCE_PACK_DATA_INFO_PACKET_H

#include "mcpe_protocol.h"
#include "mcpe_wire.h"

typedef struct {
    const char *pack_id;
    mcpe_u32 pack_id_length;
    mcpe_u32 max_chunk_size;
    mcpe_u32 chunk_count;
    McpeWireU64 compressed_pack_size;
    const char *sha256_hash;
    mcpe_u32 sha256_hash_length;
} McpeResourcePackDataInfoView;

typedef struct {
    char *pack_id;
    mcpe_u32 pack_id_length;
    mcpe_u32 pack_id_capacity;
    mcpe_u32 max_chunk_size;
    mcpe_u32 chunk_count;
    McpeWireU64 compressed_pack_size;
    char *sha256_hash;
    mcpe_u32 sha256_hash_length;
    mcpe_u32 sha256_hash_capacity;
} McpeResourcePackDataInfoTarget;

int mcpe_resource_pack_data_info_encode(
    const McpeResourcePackDataInfoView *packet,
    void *output, mcpe_u32 capacity, mcpe_u32 *length);
int mcpe_resource_pack_data_info_decode(
    const void *payload, mcpe_u32 length,
    McpeResourcePackDataInfoTarget *packet);

#endif
