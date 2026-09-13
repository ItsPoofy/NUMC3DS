#ifndef NUMC3DS_MCPE_RESOURCE_PACKS_INFO_PACKET_H
#define NUMC3DS_MCPE_RESOURCE_PACKS_INFO_PACKET_H

#include "mcpe_protocol.h"
#include "mcpe_wire.h"

typedef struct {
    const char *uuid;
    mcpe_u32 uuid_length;
    const char *version;
    mcpe_u32 version_length;
    McpeWireU64 content_size;
    const char *content_key;
    mcpe_u32 content_key_length;
} McpeResourcePackInfoEntry;

typedef struct {
    int must_accept;
    McpeResourcePackInfoEntry *behavior_packs;
    mcpe_u16 behavior_count;
    mcpe_u16 behavior_capacity;
    McpeResourcePackInfoEntry *resource_packs;
    mcpe_u16 resource_count;
    mcpe_u16 resource_capacity;
} McpeResourcePacksInfoView;

int mcpe_resource_packs_info_encode(const McpeResourcePacksInfoView *packet,
                                    void *output, mcpe_u32 capacity,
                                    mcpe_u32 *length);
int mcpe_resource_packs_info_decode(const void *payload, mcpe_u32 length,
                                    McpeResourcePacksInfoView *packet);

#endif
