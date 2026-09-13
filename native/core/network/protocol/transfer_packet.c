#include "transfer_packet.h"

int mcpe_transfer_packet_encode(
    const McpeTransferPacketView *packet,
    void *output, mcpe_u32 capacity, mcpe_u32 *length)
{
    McpeWireWriter writer;
    int result;
    if (!packet || !length) return MCPE_WIRE_INVALID_ARGUMENT;
    *length = 0;
    mcpe_wire_writer_init(&writer, output, capacity);
    result = mcpe_wire_write_string(&writer, packet->address, packet->address_length,
                                    MCPE_MAX_STRING_BYTES);
    if (result) return result;
    result = mcpe_wire_write_le16(&writer, packet->port);
    if (result) return result;
    *length = mcpe_wire_writer_size(&writer);
    return mcpe_wire_writer_status(&writer);
}

int mcpe_transfer_packet_decode(
    const void *payload, mcpe_u32 length,
    McpeTransferPacketTarget *packet)
{
    McpeWireReader reader;
    int result;
    if (!packet || (!payload && length)) return MCPE_WIRE_INVALID_ARGUMENT;
    packet->address_length = 0;
    mcpe_wire_reader_init(&reader, payload, length);
    result = mcpe_wire_read_string(&reader, packet->address, packet->address_capacity,
                                   MCPE_MAX_STRING_BYTES, &packet->address_length);
    if (result) return result;
    result = mcpe_wire_read_le16(&reader, &packet->port);
    if (result) return result;
    if (mcpe_wire_reader_remaining(&reader) != 0) {
        reader.error = MCPE_WIRE_INVALID_VALUE;
        return MCPE_WIRE_INVALID_VALUE;
    }
    return mcpe_wire_reader_status(&reader);
}
