#include "play_status_packet.h"

int mcpe_play_status_packet_encode(const McpePlayStatusPacketView *packet,
                                    void *output, mcpe_u32 capacity,
                                    mcpe_u32 *length)
{
    McpeWireWriter writer;
    int result;
    if (!packet || !length) return MCPE_WIRE_INVALID_ARGUMENT;
    *length = 0;
    mcpe_wire_writer_init(&writer, output, capacity);
    result = mcpe_wire_write_uvarint(&writer, packet->status);
    if (result) return result;
    *length = mcpe_wire_writer_size(&writer);
    return mcpe_wire_writer_status(&writer);
}

int mcpe_play_status_packet_decode(const void *payload, mcpe_u32 length,
                                    McpePlayStatusPacketView *packet)
{
    McpeWireReader reader;
    int result;
    if (!packet || (!payload && length)) return MCPE_WIRE_INVALID_ARGUMENT;
    packet->status = 0;
    mcpe_wire_reader_init(&reader, payload, length);
    result = mcpe_wire_read_uvarint(&reader, &packet->status);
    if (result) return result;
    if (mcpe_wire_reader_remaining(&reader) != 0) {
        reader.error = MCPE_WIRE_INVALID_VALUE;
        return MCPE_WIRE_INVALID_VALUE;
    }
    return mcpe_wire_reader_status(&reader);
}
