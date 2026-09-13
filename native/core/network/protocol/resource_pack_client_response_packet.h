#ifndef NUMC3DS_MCPE_RESOURCE_PACK_CLIENT_RESPONSE_PACKET_H
#define NUMC3DS_MCPE_RESOURCE_PACK_CLIENT_RESPONSE_PACKET_H

#include "mcpe_protocol.h"
#include "mcpe_wire.h"

enum McpeResourcePackResponseStatus {
    MCPE_RESOURCE_PACK_RESPONSE_REFUSED = 1,
    MCPE_RESOURCE_PACK_RESPONSE_SEND_PACKS = 2,
    MCPE_RESOURCE_PACK_RESPONSE_HAVE_ALL_PACKS = 3,
    MCPE_RESOURCE_PACK_RESPONSE_COMPLETED = 4
};

typedef struct {
    mcpe_u8 response_status;
    mcpe_u16 pack_count;
    const char *const *pack_ids;
    const mcpe_u32 *pack_id_lengths;
} McpeResourcePackClientResponseView;

typedef struct {
    mcpe_u8 response_status;
    mcpe_u16 pack_count;
    char **pack_ids;
    mcpe_u32 *pack_id_lengths;
    mcpe_u32 pack_capacity;
    mcpe_u32 pack_id_capacity;
} McpeResourcePackClientResponseTarget;

int mcpe_resource_pack_client_response_encode(
    const McpeResourcePackClientResponseView *packet,
    void *output, mcpe_u32 capacity, mcpe_u32 *length);
int mcpe_resource_pack_client_response_decode(
    const void *payload, mcpe_u32 length,
    McpeResourcePackClientResponseTarget *packet);

#endif
