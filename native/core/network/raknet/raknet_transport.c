#include "raknet_transport.h"

const mcpe_u8 mcpe_raknet_offline_magic[MCPE_RAKNET_OFFLINE_MAGIC_SIZE] = {
    0x00, 0xff, 0xff, 0x00, 0xfe, 0xfe, 0xfe, 0xfe,
    0xfd, 0xfd, 0xfd, 0xfd, 0x12, 0x34, 0x56, 0x78
};

void mcpe_raknet_default_config(McpeRaknetConfig *config)
{
    if (!config) return;
    config->mtu_payload = 0u;
    config->max_split_parts = 0u;
    config->timeout_ms = 10000u;
    config->protocol_version = 10u;
    config->reserved[0] = 0;
    config->reserved[1] = 0;
    config->reserved[2] = 0;
}

int mcpe_raknet_transport_init(McpeRaknetTransport *transport, McpeDatagramBackend *datagram, const McpeRaknetConfig *config)
{
    if (!transport || !datagram) return MCPE_DATAGRAM_INVALID_ARGUMENT;
    transport->datagram = datagram;
    if (config) {
        transport->config = *config;
    } else {
        mcpe_raknet_default_config(&transport->config);
    }
    if (!transport->config.timeout_ms || !transport->config.protocol_version) {
        transport->active = 0;
        return MCPE_DATAGRAM_INVALID_ARGUMENT;
    }
    transport->active = 1;
    return MCPE_DATAGRAM_OK;
}

void mcpe_raknet_transport_shutdown(McpeRaknetTransport *transport)
{
    if (!transport) return;
    transport->active = 0;
    transport->datagram = 0;
}

int mcpe_raknet_transport_send_datagram(McpeRaknetTransport *transport, const McpeDatagramEndpoint *destination, const void *payload, mcpe_u32 length)
{
    if (!transport || !transport->active || !transport->datagram) return MCPE_DATAGRAM_NOT_STARTED;
    if (transport->config.mtu_payload && length > transport->config.mtu_payload) return MCPE_DATAGRAM_TOO_LARGE;
    return mcpe_datagram_backend_send(transport->datagram, destination, payload, length);
}

int mcpe_raknet_transport_receive_datagram(McpeRaknetTransport *transport, McpeDatagramEndpoint *source, void *payload, mcpe_u32 capacity, mcpe_u32 *length)
{
    if (!transport || !transport->active || !transport->datagram) return MCPE_DATAGRAM_NOT_STARTED;
    return mcpe_datagram_backend_receive(transport->datagram, source, payload, capacity, length);
}
