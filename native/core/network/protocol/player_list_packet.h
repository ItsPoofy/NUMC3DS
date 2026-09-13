#ifndef NUMC3DS_MCPE_PLAYER_LIST_PACKET_H
#define NUMC3DS_MCPE_PLAYER_LIST_PACKET_H

#include "mcpe_protocol.h"
#include "mcpe_wire.h"

#define MCPE_PLAYER_LIST_ADD 0u
#define MCPE_PLAYER_LIST_REMOVE 1u
#define MCPE_PLAYER_UUID_BYTES 16u

typedef struct {
    mcpe_u8 uuid[MCPE_PLAYER_UUID_BYTES];
    McpeWireU64 entity_unique_id;
    const char *name;
    mcpe_u32 name_length;
    const char *skin_id;
    mcpe_u32 skin_id_length;
    const mcpe_u8 *skin_data;
    mcpe_u32 skin_data_length;
} McpePlayerListEntry;

typedef struct {
    mcpe_u8 action;
    McpePlayerListEntry *entries;
    mcpe_u32 count;
    mcpe_u32 capacity;
} McpePlayerListView;

int mcpe_player_list_encode(const McpePlayerListView *packet,
                            void *output, mcpe_u32 capacity,
                            mcpe_u32 *length);
int mcpe_player_list_decode(const void *payload, mcpe_u32 length,
                            McpePlayerListView *packet);

#endif
