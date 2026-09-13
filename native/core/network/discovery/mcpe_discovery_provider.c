#include "mcpe_discovery_provider.h"
#include "../../../diagnostics/network_debug.h"
#include "../protocol/mcpe_wire.h"

static void mcpe_discovery_copy_record(McpeDiscoveryRecord *destination,
                                        const McpeDiscoveryRecord *source)
{
    mcpe_u8 *to = (mcpe_u8 *)destination;
    const mcpe_u8 *from = (const mcpe_u8 *)source;
    mcpe_u32 index;
    if (!destination || !source) return;
    for (index = 0; index < sizeof(*destination); ++index) to[index] = from[index];
}

static void mcpe_discovery_provider_expire(McpeDiscoveryProvider *provider,
                                            mcpe_u32 now_ms)
{
    mcpe_u32 read_index;
    mcpe_u32 write_index = 0;
    if (!provider || !provider->records) return;
    for (read_index = 0; read_index < provider->count; ++read_index) {
        McpeDiscoveryRecord *record = &provider->records[read_index];
        if ((mcpe_u32)(now_ms - record->observed_ms) <= provider->expiry_ms) {
            if (write_index != read_index) {
                mcpe_discovery_copy_record(&provider->records[write_index], record);
            }
            ++write_index;
        }
    }
    provider->count = write_index;
}

static int mcpe_discovery_provider_update(McpeDiscoveryProvider *provider,
                                          const McpeDiscoveryRecord *incoming)
{
    mcpe_u32 index;
    if (!provider || !incoming) return MCPE_WIRE_INVALID_ARGUMENT;
    for (index = 0; index < provider->count; ++index) {
        if (mcpe_discovery_record_same_identity(&provider->records[index], incoming)) {
            mcpe_discovery_copy_record(&provider->records[index], incoming);
            return MCPE_WIRE_OK;
        }
    }
    if (provider->count >= provider->capacity || !provider->records) return MCPE_WIRE_LIMIT;
    mcpe_discovery_copy_record(&provider->records[provider->count], incoming);
    provider->count++;
    net_log_open(NET_LOG_VERBOSE, "discovery", "record_add");
    net_log_dec("count", provider->count);
    net_log_text("name", incoming->name);
    net_log_text("world", incoming->world);
    net_log_text("version", incoming->version);
    net_log_text("game", incoming->game_type);
    net_log_dec("players", incoming->current_players);
    net_log_dec("max", incoming->maximum_players);
    net_log_dec("protocol", incoming->protocol);
    net_log_ipv4("endpoint", incoming->endpoint.address_be, incoming->endpoint.port_be);
    net_log_close();
    return MCPE_WIRE_OK;
}

void mcpe_discovery_provider_init(McpeDiscoveryProvider *provider,
                                  McpeDatagramBackend *backend,
                                  McpeDiscoveryRecord *records,
                                  mcpe_u32 capacity,
                                  mcpe_u32 expiry_ms)
{
    if (!provider) return;
    provider->backend = backend;
    provider->records = records;
    provider->capacity = capacity;
    provider->count = 0;
    provider->expiry_ms = expiry_ms ? expiry_ms : MCPE_DISCOVERY_DEFAULT_EXPIRY_MS;
    provider->started = 0;
    provider->cancelled = 0;
    provider->reserved[0] = 0;
    provider->reserved[1] = 0;
    provider->client_guid = 0;
}

int mcpe_discovery_provider_refresh(McpeDiscoveryProvider *provider,
                                    mcpe_u32 now_ms,
                                    mcpe_u64 ping_timestamp)
{
    McpeDatagramEndpoint broadcast;
    mcpe_u8 ping[40];
    mcpe_u32 ping_length;
    int result;
    if (!provider || !provider->backend) return MCPE_DATAGRAM_INVALID_ARGUMENT;
    if (!provider->started) {
        result = mcpe_datagram_backend_start(provider->backend, 0, 1);
        if (result != MCPE_DATAGRAM_OK) return result;
        provider->started = 1;
    }
    provider->cancelled = 0;
    mcpe_discovery_provider_expire(provider, now_ms);
    if (!provider->client_guid) {
        provider->client_guid = (ping_timestamp ^ 0x4e754d4333445331ull) | 1ull;
    }
    result = mcpe_discovery_build_unconnected_ping(ping, sizeof(ping), ping_timestamp,
                                                   provider->client_guid, &ping_length);
    broadcast.address_be = 0xffffffffu;
    broadcast.port_be = (mcpe_u16)MCPE_GAME_PORT;
    broadcast.reserved = 0;
    result = mcpe_datagram_backend_send(provider->backend, &broadcast, ping, ping_length);

    broadcast.address_be = 0x7f000001u;
    (void)mcpe_datagram_backend_send(provider->backend, &broadcast, ping, ping_length);
    net_log_open(NET_LOG_VERBOSE, "discovery", "refresh");
    net_log_dec("records", provider->count);
    net_log_dec("ping_length", ping_length);
    net_log_signed("send_result", result);
    net_log_dec("now", now_ms);
    net_log_close();
    return result;
}

int mcpe_discovery_provider_tick(McpeDiscoveryProvider *provider,
                                 mcpe_u32 now_ms,
                                 mcpe_u32 timeout_ms)
{
    mcpe_u8 payload[MCPE_MAX_ANNOUNCEMENT_BYTES];
    mcpe_u32 receive_count = 0;
    int updated = 0;
    int result;
    if (!provider || !provider->backend) return MCPE_DATAGRAM_INVALID_ARGUMENT;
    if (!provider->started) return MCPE_DATAGRAM_NOT_STARTED;
    mcpe_discovery_provider_expire(provider, now_ms);
    if (provider->cancelled) return MCPE_DATAGRAM_WOULD_BLOCK;
    result = mcpe_datagram_backend_poll(provider->backend, timeout_ms);
    if (result < 0) return result;
    for (;;) {
        McpeDatagramEndpoint source;
        mcpe_u32 length = 0;
        McpeDiscoveryRecord record;
        result = mcpe_datagram_backend_receive(provider->backend, &source, payload,
                                               sizeof(payload), &length);
        if (result == MCPE_DATAGRAM_WOULD_BLOCK) break;
        if (result != MCPE_DATAGRAM_OK) return result;
        if (!length) continue;
        if (mcpe_discovery_decode_unconnected_pong(payload, length, &record) != MCPE_WIRE_OK ||
            !mcpe_discovery_record_is_compatible(&record)) {
            if (++receive_count >= MCPE_DISCOVERY_MAX_RECEIVES_PER_TICK) break;
            continue;
        }
        record.endpoint = source;
        record.observed_ms = now_ms;
        result = mcpe_discovery_provider_update(provider, &record);
        if (result != MCPE_WIRE_OK && result != MCPE_WIRE_LIMIT) return result;
        if (result == MCPE_WIRE_OK) updated = 1;
        if (++receive_count >= MCPE_DISCOVERY_MAX_RECEIVES_PER_TICK) break;
    }
    return updated ? MCPE_DATAGRAM_OK : MCPE_DATAGRAM_WOULD_BLOCK;
}

void mcpe_discovery_provider_cancel(McpeDiscoveryProvider *provider)
{
    if (!provider) return;
    provider->cancelled = 1;
    if (provider->backend) mcpe_datagram_backend_wakeup(provider->backend);
}

void mcpe_discovery_provider_stop(McpeDiscoveryProvider *provider)
{
    if (!provider) return;
    net_log_open(NET_LOG_VERBOSE, "discovery", "stop");
    net_log_dec("records", provider->count);
    net_log_flag("started", provider->started);
    net_log_close();
    /* Deliberately keep the datagram backend running. Stopping it closes the
       SOC socket, releases the shared-memory block and issues SOCU_Shutdown;
       that global teardown disturbs the stock network state and crashes the
       game during screen destruction. The socket stays open for the process
       lifetime and is reused on the next refresh; only discovery records reset. */
    provider->cancelled = 0;
    provider->count = 0;
}

mcpe_u32 mcpe_discovery_provider_count(const McpeDiscoveryProvider *provider)
{
    return provider ? provider->count : 0;
}

const McpeDiscoveryRecord *mcpe_discovery_provider_get(const McpeDiscoveryProvider *provider,
                                                       mcpe_u32 index)
{
    if (!provider || index >= provider->count || !provider->records) return 0;
    return &provider->records[index];
}
