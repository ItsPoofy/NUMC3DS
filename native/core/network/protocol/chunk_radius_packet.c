#include "chunk_radius_packet.h"

static int mcpe_chunk_radius_packet_encode(const McpeChunkRadiusPacketView *packet,
                                           void *output, mcpe_u32 capacity,
                                           mcpe_u32 *length)
{
    McpeWireWriter writer;
    int result;
    if (!packet || !length) return MCPE_WIRE_INVALID_ARGUMENT;
    *length = 0;
    mcpe_wire_writer_init(&writer, output, capacity);
    result = mcpe_wire_write_uvarint(&writer, packet->radius);
    if (result) return result;
    *length = mcpe_wire_writer_size(&writer);
    return mcpe_wire_writer_status(&writer);
}

static int mcpe_chunk_radius_packet_decode(const void *payload, mcpe_u32 length,
                                           McpeChunkRadiusPacketView *packet)
{
    McpeWireReader reader;
    int result;
    if (!packet || (!payload && length)) return MCPE_WIRE_INVALID_ARGUMENT;
    packet->radius = 0;
    mcpe_wire_reader_init(&reader, payload, length);
    result = mcpe_wire_read_uvarint(&reader, &packet->radius);
    if (result) return result;
    if (mcpe_wire_reader_remaining(&reader) != 0) {
        reader.error = MCPE_WIRE_INVALID_VALUE;
        return MCPE_WIRE_INVALID_VALUE;
    }
    return mcpe_wire_reader_status(&reader);
}

int mcpe_request_chunk_radius_packet_encode(
    const McpeChunkRadiusPacketView *packet,
    void *output, mcpe_u32 capacity, mcpe_u32 *length)
{
    return mcpe_chunk_radius_packet_encode(packet, output, capacity, length);
}

int mcpe_request_chunk_radius_packet_decode(
    const void *payload, mcpe_u32 length,
    McpeChunkRadiusPacketView *packet)
{
    return mcpe_chunk_radius_packet_decode(payload, length, packet);
}

int mcpe_chunk_radius_updated_packet_encode(
    const McpeChunkRadiusPacketView *packet,
    void *output, mcpe_u32 capacity, mcpe_u32 *length)
{
    return mcpe_chunk_radius_packet_encode(packet, output, capacity, length);
}

int mcpe_chunk_radius_updated_packet_decode(
    const void *payload, mcpe_u32 length,
    McpeChunkRadiusPacketView *packet)
{
    return mcpe_chunk_radius_packet_decode(payload, length, packet);
}
