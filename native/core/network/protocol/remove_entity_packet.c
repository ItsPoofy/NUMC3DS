#include "remove_entity_packet.h"

int mcpe_remove_entity_packet_encode(
    const McpeRemoveEntityPacketView *packet,
    void *output, mcpe_u32 capacity, mcpe_u32 *length)
{
    McpeWireWriter writer;
    int result;
    if (!packet || !length) return MCPE_WIRE_INVALID_ARGUMENT;
    *length = 0;
    mcpe_wire_writer_init(&writer, output, capacity);
    result = mcpe_wire_write_svarint64(&writer, &packet->entity_unique_id);
    if (result) return result;
    *length = mcpe_wire_writer_size(&writer);
    return mcpe_wire_writer_status(&writer);
}

int mcpe_remove_entity_packet_decode(
    const void *payload, mcpe_u32 length,
    McpeRemoveEntityPacketView *packet)
{
    McpeWireReader reader;
    int result;
    if (!packet || (!payload && length)) return MCPE_WIRE_INVALID_ARGUMENT;
    packet->entity_unique_id.lo = 0;
    packet->entity_unique_id.hi = 0;
    mcpe_wire_reader_init(&reader, payload, length);
    result = mcpe_wire_read_svarint64(&reader, &packet->entity_unique_id);
    if (result) return result;
    if (mcpe_wire_reader_remaining(&reader) != 0) {
        reader.error = MCPE_WIRE_INVALID_VALUE;
        return MCPE_WIRE_INVALID_VALUE;
    }
    return mcpe_wire_reader_status(&reader);
}
