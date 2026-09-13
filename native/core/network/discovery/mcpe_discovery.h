#ifndef NUMC3DS_MCPE_DISCOVERY_H
#define NUMC3DS_MCPE_DISCOVERY_H

#include "../platform/datagram_backend.h"
#include "../protocol/mcpe_protocol.h"

#define MCPE_DISCOVERY_FIELD_BYTES 160u
#define MCPE_DISCOVERY_GUID_BYTES 80u

typedef struct {
    const char *name;
    mcpe_u32 name_length;
    const char *version;
    mcpe_u32 version_length;
    mcpe_u32 current_players;
    mcpe_u32 maximum_players;
    const char *guid;
    mcpe_u32 guid_length;
    const char *world;
    mcpe_u32 world_length;
    const char *game_type;
    mcpe_u32 game_type_length;
} McpeDiscoveryAdvertisement;

typedef struct {
    char name[MCPE_DISCOVERY_FIELD_BYTES];
    char version[MCPE_DISCOVERY_FIELD_BYTES];
    char guid[MCPE_DISCOVERY_GUID_BYTES];
    char world[MCPE_DISCOVERY_FIELD_BYTES];
    char game_type[MCPE_DISCOVERY_FIELD_BYTES];
    mcpe_u32 protocol;
    mcpe_u32 current_players;
    mcpe_u32 maximum_players;
    McpeDatagramEndpoint endpoint;
    mcpe_u32 observed_ms;
} McpeDiscoveryRecord;

int mcpe_discovery_build_unconnected_ping(void *output, mcpe_u32 capacity,
                                          mcpe_u64 timestamp, mcpe_u64 client_guid,
                                          mcpe_u32 *length);
int mcpe_discovery_encode_announcement(const McpeDiscoveryAdvertisement *advertisement, void *output, mcpe_u32 capacity, mcpe_u32 *length);
int mcpe_discovery_encode_unconnected_pong(const McpeDiscoveryAdvertisement *advertisement,
                                           mcpe_u64 timestamp, mcpe_u64 server_guid,
                                           void *output, mcpe_u32 capacity, mcpe_u32 *length);
int mcpe_discovery_decode_announcement(const void *payload, mcpe_u32 length, McpeDiscoveryRecord *record);
int mcpe_discovery_decode_unconnected_pong(const void *payload, mcpe_u32 length, McpeDiscoveryRecord *record);
int mcpe_discovery_record_is_compatible(const McpeDiscoveryRecord *record);
int mcpe_discovery_record_same_identity(const McpeDiscoveryRecord *left, const McpeDiscoveryRecord *right);

#endif
