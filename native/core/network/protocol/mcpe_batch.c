#include "mcpe_batch.h"

int mcpe_batch_write_packet(McpeWireWriter *writer,
                            const void *payload, mcpe_u32 length)
{
    int result;
    if (!writer || (length && !payload)) return MCPE_WIRE_INVALID_ARGUMENT;
    if (length > MCPE_MAX_BATCH_BYTES) return MCPE_WIRE_LIMIT;
    result = mcpe_wire_write_uvarint(writer, length);
    if (result) return result;
    return mcpe_wire_write_bytes(writer, payload, length);
}

int mcpe_batch_read_packet(McpeWireReader *reader,
                           const mcpe_u8 **payload, mcpe_u32 *length)
{
    mcpe_u32 packet_length;
    int result;
    if (!reader || !payload || !length) return MCPE_WIRE_INVALID_ARGUMENT;
    *payload = 0;
    *length = 0;
    if (mcpe_wire_reader_remaining(reader) == 0) return MCPE_BATCH_END;
    result = mcpe_wire_read_uvarint(reader, &packet_length);
    if (result) return result;
    if (packet_length > MCPE_MAX_BATCH_BYTES || packet_length > mcpe_wire_reader_remaining(reader)) {
        reader->error = packet_length > MCPE_MAX_BATCH_BYTES ? MCPE_WIRE_LIMIT : MCPE_WIRE_TRUNCATED;
        return reader->error;
    }
    result = mcpe_wire_read_bytes(reader, payload, packet_length);
    if (result) return result;
    *length = packet_length;
    return MCPE_WIRE_OK;
}
