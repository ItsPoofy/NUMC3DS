#ifndef NUMC3DS_MCPE_DISCOVERY_PROVIDER_H
#define NUMC3DS_MCPE_DISCOVERY_PROVIDER_H

#include "mcpe_discovery.h"

#define MCPE_DISCOVERY_DEFAULT_EXPIRY_MS 5000u
#define MCPE_DISCOVERY_MAX_RECEIVES_PER_TICK 32u

typedef struct {
    McpeDatagramBackend *backend;
    McpeDiscoveryRecord *records;
    mcpe_u32 capacity;
    mcpe_u32 count;
    mcpe_u32 expiry_ms;
    mcpe_u8 started;
    mcpe_u8 cancelled;
    mcpe_u8 reserved[2];
    mcpe_u64 client_guid;
} McpeDiscoveryProvider;

void mcpe_discovery_provider_init(McpeDiscoveryProvider *provider,
                                  McpeDatagramBackend *backend,
                                  McpeDiscoveryRecord *records,
                                  mcpe_u32 capacity,
                                  mcpe_u32 expiry_ms);
int mcpe_discovery_provider_refresh(McpeDiscoveryProvider *provider,
                                    mcpe_u32 now_ms,
                                    mcpe_u64 ping_timestamp);
int mcpe_discovery_provider_tick(McpeDiscoveryProvider *provider,
                                 mcpe_u32 now_ms,
                                 mcpe_u32 timeout_ms);
void mcpe_discovery_provider_cancel(McpeDiscoveryProvider *provider);
void mcpe_discovery_provider_stop(McpeDiscoveryProvider *provider);
mcpe_u32 mcpe_discovery_provider_count(const McpeDiscoveryProvider *provider);
const McpeDiscoveryRecord *mcpe_discovery_provider_get(const McpeDiscoveryProvider *provider,
                                                       mcpe_u32 index);

#endif
