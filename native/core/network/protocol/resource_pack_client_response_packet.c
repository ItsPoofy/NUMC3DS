#include "resource_pack_client_response_packet.h"

static int mcpe_resource_pack_response_status_valid(mcpe_u8 status)
{
    return status >= MCPE_RESOURCE_PACK_RESPONSE_REFUSED &&
           status <= MCPE_RESOURCE_PACK_RESPONSE_COMPLETED;
}

int mcpe_resource_pack_client_response_encode(
    const McpeResourcePackClientResponseView *packet,
    void *output, mcpe_u32 capacity, mcpe_u32 *length)
{
    McpeWireWriter writer;
    mcpe_u16 index;
    int result;
    if (!packet || !length) return MCPE_WIRE_INVALID_ARGUMENT;
    *length = 0;
    if (!mcpe_resource_pack_response_status_valid(packet->response_status)) return MCPE_WIRE_INVALID_VALUE;
    if (packet->pack_count && (!packet->pack_ids || !packet->pack_id_lengths)) return MCPE_WIRE_INVALID_ARGUMENT;
    mcpe_wire_writer_init(&writer, output, capacity);
    result = mcpe_wire_write_u8(&writer, packet->response_status);
    if (result) return result;
    result = mcpe_wire_write_le16(&writer, packet->pack_count);
    if (result) return result;
    for (index = 0; index < packet->pack_count; ++index) {
        result = mcpe_wire_write_string(&writer, packet->pack_ids[index],
                                        packet->pack_id_lengths[index],
                                        MCPE_MAX_STRING_BYTES);
        if (result) return result;
    }
    *length = mcpe_wire_writer_size(&writer);
    return mcpe_wire_writer_status(&writer);
}

int mcpe_resource_pack_client_response_decode(
    const void *payload, mcpe_u32 length,
    McpeResourcePackClientResponseTarget *packet)
{
    McpeWireReader reader;
    mcpe_u16 index;
    int result;
    if (!packet || (!payload && length)) return MCPE_WIRE_INVALID_ARGUMENT;
    packet->response_status = 0;
    packet->pack_count = 0;
    mcpe_wire_reader_init(&reader, payload, length);
    result = mcpe_wire_read_u8(&reader, &packet->response_status);
    if (result) return result;
    if (!mcpe_resource_pack_response_status_valid(packet->response_status)) {
        reader.error = MCPE_WIRE_INVALID_VALUE;
        return MCPE_WIRE_INVALID_VALUE;
    }
    result = mcpe_wire_read_le16(&reader, &packet->pack_count);
    if (result) return result;
    if (packet->pack_count > packet->pack_capacity ||
        (packet->pack_count && (!packet->pack_ids || !packet->pack_id_lengths))) {
        reader.error = MCPE_WIRE_LIMIT;
        return MCPE_WIRE_LIMIT;
    }
    for (index = 0; index < packet->pack_count; ++index) {
        result = mcpe_wire_read_string(&reader, packet->pack_ids[index],
                                       packet->pack_id_capacity,
                                       MCPE_MAX_STRING_BYTES,
                                       &packet->pack_id_lengths[index]);
        if (result) return result;
    }
    if (mcpe_wire_reader_remaining(&reader) != 0) {
        reader.error = MCPE_WIRE_INVALID_VALUE;
        return MCPE_WIRE_INVALID_VALUE;
    }
    return mcpe_wire_reader_status(&reader);
}
