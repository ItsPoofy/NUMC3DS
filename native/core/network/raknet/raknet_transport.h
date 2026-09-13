#ifndef NUMC3DS_RAKNET_TRANSPORT_H
#define NUMC3DS_RAKNET_TRANSPORT_H

#include "../platform/datagram_backend.h"

#define MCPE_RAKNET_OFFLINE_MAGIC_SIZE 16u

extern const mcpe_u8 mcpe_raknet_offline_magic[MCPE_RAKNET_OFFLINE_MAGIC_SIZE];

typedef struct {
    mcpe_u32 mtu_payload;
    mcpe_u32 max_split_parts;
    mcpe_u32 timeout_ms;
    mcpe_u8 protocol_version;
    mcpe_u8 reserved[3];
} McpeRaknetConfig;

typedef struct {
    McpeDatagramBackend *datagram;
    McpeRaknetConfig config;
    int active;
} McpeRaknetTransport;

void mcpe_raknet_default_config(McpeRaknetConfig *config);
int mcpe_raknet_transport_init(McpeRaknetTransport *transport, McpeDatagramBackend *datagram, const McpeRaknetConfig *config);
void mcpe_raknet_transport_shutdown(McpeRaknetTransport *transport);
int mcpe_raknet_transport_send_datagram(McpeRaknetTransport *transport, const McpeDatagramEndpoint *destination, const void *payload, mcpe_u32 length);
int mcpe_raknet_transport_receive_datagram(McpeRaknetTransport *transport, McpeDatagramEndpoint *source, void *payload, mcpe_u32 capacity, mcpe_u32 *length);

#endif
