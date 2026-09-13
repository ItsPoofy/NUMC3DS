#include "disconnect_packet.h"

int mcpe_disconnect_packet_encode(
    const McpeDisconnectPacketView *packet,
    void *output, mcpe_u32 capacity, mcpe_u32 *length)
{
    McpeWireWriter writer;
    int result;
    if (!packet || !length) return MCPE_WIRE_INVALID_ARGUMENT;
    *length = 0;
    if (packet->hide_disconnection_screen > 1) return MCPE_WIRE_INVALID_VALUE;
    if (!packet->hide_disconnection_screen &&
        packet->message_length && !packet->message) return MCPE_WIRE_INVALID_ARGUMENT;
    mcpe_wire_writer_init(&writer, output, capacity);
    result = mcpe_wire_write_bool(&writer, packet->hide_disconnection_screen);
    if (result) return result;
    if (!packet->hide_disconnection_screen) {
        result = mcpe_wire_write_string(&writer, packet->message,
                                        packet->message_length,
                                        MCPE_MAX_STRING_BYTES);
        if (result) return result;
    }
    *length = mcpe_wire_writer_size(&writer);
    return mcpe_wire_writer_status(&writer);
}

int mcpe_disconnect_packet_decode(
    const void *payload, mcpe_u32 length,
    McpeDisconnectPacketTarget *packet)
{
    McpeWireReader reader;
    int result;
    if (!packet || (!payload && length)) return MCPE_WIRE_INVALID_ARGUMENT;
    packet->hide_disconnection_screen = 0;
    packet->message_length = 0;
    mcpe_wire_reader_init(&reader, payload, length);
    result = mcpe_wire_read_bool(&reader, &packet->hide_disconnection_screen);
    if (result) return result;
    if (!packet->hide_disconnection_screen) {
        result = mcpe_wire_read_string(&reader, packet->message,
                                       packet->message_capacity,
                                       MCPE_MAX_STRING_BYTES,
                                       &packet->message_length);
        if (result) return result;
    }
    if (mcpe_wire_reader_remaining(&reader) != 0) {
        reader.error = MCPE_WIRE_INVALID_VALUE;
        return MCPE_WIRE_INVALID_VALUE;
    }
    return mcpe_wire_reader_status(&reader);
}
