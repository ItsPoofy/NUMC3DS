#ifndef NUMC3DS_SOC_UDP_BACKEND_H
#define NUMC3DS_SOC_UDP_BACKEND_H

#include "datagram_backend.h"

#define MCPE_SOC_CONTEXT_SIZE_DEFAULT 0x00100000u
#define MCPE_SOC_CONTEXT_ALIGNMENT 0x00001000u
#define MCPE_SOC_SHARED_PAYLOAD_LIMIT 0x000005d4u

typedef struct {
    mcpe_u32 service_handle;
    mcpe_u32 memory_handle;
    mcpe_u32 context_address;
    mcpe_u32 context_raw;
    mcpe_u32 context_size;
    mcpe_s32 socket_handle;
    mcpe_u8 started;
    mcpe_u8 service_initialized;
    mcpe_u8 nonblocking;
    mcpe_u8 broadcast_requested;
    mcpe_u8 wake_requested;
    mcpe_u8 reserved[3];
} McpeSocUdpBackend;

void mcpe_soc_udp_backend_init(McpeSocUdpBackend *backend, mcpe_u32 context_size);
const McpeDatagramBackendOps *mcpe_soc_udp_backend_ops(void);

/* One SOC context/service/socket shared by the discovery provider and the stock
   RNS2 adapter. The game heap can only host a single 1 MiB block in the region
   svcCreateMemoryBlock accepts, so a second context fails outright. Both users
   run at different times, so they share the same socket. */
McpeDatagramBackend *mcpe_soc_udp_shared_backend(void);

#endif
