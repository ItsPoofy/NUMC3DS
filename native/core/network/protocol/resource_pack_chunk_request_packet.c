#include "resource_pack_chunk_request_packet.h"

int mcpe_resource_pack_chunk_request_encode(
    const McpeResourcePackChunkRequestView *packet,
    void *output, mcpe_u32 capacity, mcpe_u32 *length)
{
    McpeWireWriter writer;
    int result;
    if (!packet || !length) return MCPE_WIRE_INVALID_ARGUMENT;
    *length = 0;
    mcpe_wire_writer_init(&writer, output, capacity);
    result = mcpe_wire_write_string(&writer, packet->pack_id, packet->pack_id_length,
                                    MCPE_MAX_STRING_BYTES);
    if (result) return result;
    result = mcpe_wire_write_le32(&writer, packet->chunk_index);
    if (result) return result;
    *length = mcpe_wire_writer_size(&writer);
    return mcpe_wire_writer_status(&writer);
}

int mcpe_resource_pack_chunk_request_decode(
    const void *payload, mcpe_u32 length,
    McpeResourcePackChunkRequestTarget *packet)
{
    McpeWireReader reader;
    int result;
    if (!packet || (!payload && length)) return MCPE_WIRE_INVALID_ARGUMENT;
    packet->pack_id_length = 0;
    mcpe_wire_reader_init(&reader, payload, length);
    result = mcpe_wire_read_string(&reader, packet->pack_id, packet->pack_id_capacity,
                                   MCPE_MAX_STRING_BYTES, &packet->pack_id_length);
    if (result) return result;
    result = mcpe_wire_read_le32(&reader, &packet->chunk_index);
    if (result) return result;
    if (mcpe_wire_reader_remaining(&reader) != 0) {
        reader.error = MCPE_WIRE_INVALID_VALUE;
        return MCPE_WIRE_INVALID_VALUE;
    }
    return mcpe_wire_reader_status(&reader);
}
