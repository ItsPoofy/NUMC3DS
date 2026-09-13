#include "mcpe_packet_envelope.h"

int mcpe_packet_envelope_encode(const McpePacketEnvelopeView *packet,
                                void *output, mcpe_u32 capacity,
                                mcpe_u32 *length)
{
    McpeWireWriter writer;
    int result;
    if (!packet || !length || (packet->payload_length && !packet->payload)) {
        return MCPE_WIRE_INVALID_ARGUMENT;
    }
    if (packet->payload_length > MCPE_MAX_BATCH_BYTES) return MCPE_WIRE_LIMIT;
    *length = 0;
    mcpe_wire_writer_init(&writer, output, capacity);
    result = mcpe_wire_write_uvarint(&writer, packet->packet_id);
    if (result) return result;
    result = mcpe_wire_write_bytes(&writer, packet->payload, packet->payload_length);
    if (result) return result;
    *length = mcpe_wire_writer_size(&writer);
    return mcpe_wire_writer_status(&writer);
}

int mcpe_packet_envelope_decode(const void *payload, mcpe_u32 length,
                                McpePacketEnvelopeTarget *packet)
{
    McpeWireReader reader;
    int result;
    if (!packet || (!payload && length)) return MCPE_WIRE_INVALID_ARGUMENT;
    packet->packet_id = 0;
    packet->payload = 0;
    packet->payload_length = 0;
    if (length > MCPE_MAX_BATCH_BYTES) return MCPE_WIRE_LIMIT;
    mcpe_wire_reader_init(&reader, payload, length);
    result = mcpe_wire_read_uvarint(&reader, &packet->packet_id);
    if (result) return result;
    packet->payload_length = mcpe_wire_reader_remaining(&reader);
    if (packet->payload_length) {
        result = mcpe_wire_read_bytes(&reader, &packet->payload,
                                      packet->payload_length);
        if (result) return result;
    }
    return mcpe_wire_reader_status(&reader);
}
