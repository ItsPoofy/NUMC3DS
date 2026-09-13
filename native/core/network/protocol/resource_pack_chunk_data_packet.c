#include "resource_pack_chunk_data_packet.h"

int mcpe_resource_pack_chunk_data_encode(
    const McpeResourcePackChunkDataView *packet,
    void *output, mcpe_u32 capacity, mcpe_u32 *length)
{
    McpeWireWriter writer;
    int result;
    if (!packet || !length) return MCPE_WIRE_INVALID_ARGUMENT;
    *length = 0;
    if (packet->data_length > MCPE_MAX_BATCH_BYTES) return MCPE_WIRE_LIMIT;
    mcpe_wire_writer_init(&writer, output, capacity);
    result = mcpe_wire_write_string(&writer, packet->pack_id, packet->pack_id_length,
                                    MCPE_MAX_STRING_BYTES);
    if (result) return result;
    result = mcpe_wire_write_le32(&writer, packet->chunk_index);
    if (result) return result;
    result = mcpe_wire_write_le64(&writer, &packet->progress);
    if (result) return result;
    result = mcpe_wire_write_le32(&writer, packet->data_length);
    if (result) return result;
    result = mcpe_wire_write_bytes(&writer, packet->data, packet->data_length);
    if (result) return result;
    *length = mcpe_wire_writer_size(&writer);
    return mcpe_wire_writer_status(&writer);
}

int mcpe_resource_pack_chunk_data_decode(
    const void *payload, mcpe_u32 length,
    McpeResourcePackChunkDataTarget *packet)
{
    McpeWireReader reader;
    mcpe_u32 data_length;
    int result;
    if (!packet || (!payload && length)) return MCPE_WIRE_INVALID_ARGUMENT;
    packet->pack_id_length = 0;
    packet->data = 0;
    packet->data_length = 0;
    mcpe_wire_reader_init(&reader, payload, length);
    result = mcpe_wire_read_string(&reader, packet->pack_id, packet->pack_id_capacity,
                                   MCPE_MAX_STRING_BYTES, &packet->pack_id_length);
    if (result) return result;
    result = mcpe_wire_read_le32(&reader, &packet->chunk_index);
    if (result) return result;
    result = mcpe_wire_read_le64(&reader, &packet->progress);
    if (result) return result;
    result = mcpe_wire_read_le32(&reader, &data_length);
    if (result) return result;
    if (data_length > MCPE_MAX_BATCH_BYTES) {
        reader.error = MCPE_WIRE_LIMIT;
        return MCPE_WIRE_LIMIT;
    }
    result = mcpe_wire_read_bytes(&reader, &packet->data, data_length);
    if (result) return result;
    packet->data_length = data_length;
    if (mcpe_wire_reader_remaining(&reader) != 0) {
        reader.error = MCPE_WIRE_INVALID_VALUE;
        return MCPE_WIRE_INVALID_VALUE;
    }
    return mcpe_wire_reader_status(&reader);
}
