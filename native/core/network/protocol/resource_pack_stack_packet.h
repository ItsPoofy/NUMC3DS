#ifndef NUMC3DS_MCPE_RESOURCE_PACK_STACK_PACKET_H
#define NUMC3DS_MCPE_RESOURCE_PACK_STACK_PACKET_H

#include "mcpe_protocol.h"
#include "mcpe_wire.h"

typedef struct {
    const char *pack_id;
    mcpe_u32 pack_id_length;
    const char *version;
    mcpe_u32 version_length;
} McpePackIdVersion;

typedef struct {
    int must_accept;
    McpePackIdVersion *behavior_packs;
    mcpe_u32 behavior_count;
    mcpe_u32 behavior_capacity;
    McpePackIdVersion *resource_packs;
    mcpe_u32 resource_count;
    mcpe_u32 resource_capacity;
} McpeResourcePackStackView;

int mcpe_resource_pack_stack_encode(const McpeResourcePackStackView *packet,
                                    void *output, mcpe_u32 capacity,
                                    mcpe_u32 *length);
int mcpe_resource_pack_stack_decode(const void *payload, mcpe_u32 length,
                                    McpeResourcePackStackView *packet);

#endif
