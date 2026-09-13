#include "handshake_packet.h"

static int mcpe_handshake_write_bytes(McpeWireWriter *writer,
                                       const mcpe_u8 *value,
                                       mcpe_u32 length,
                                       mcpe_u32 max_bytes)
{
    int result;
    if (!length || length > max_bytes || !value) return MCPE_WIRE_INVALID_VALUE;
    result = mcpe_wire_write_uvarint(writer, length);
    if (result) return result;
    return mcpe_wire_write_bytes(writer, value, length);
}

static int mcpe_handshake_read_bytes(McpeWireReader *reader,
                                      const mcpe_u8 **value,
                                      mcpe_u32 *length,
                                      mcpe_u32 max_bytes)
{
    mcpe_u32 size;
    int result;
    if (!reader || !value || !length) return MCPE_WIRE_INVALID_ARGUMENT;
    result = mcpe_wire_read_uvarint(reader, &size);
    if (result) return result;
    if (!size || size > max_bytes) {
        reader->error = MCPE_WIRE_INVALID_VALUE;
        return MCPE_WIRE_INVALID_VALUE;
    }
    result = mcpe_wire_read_bytes(reader, value, size);
    if (result) return result;
    *length = size;
    return MCPE_WIRE_OK;
}

int mcpe_server_to_client_handshake_encode(const McpeServerToClientHandshakeView *packet,
                                           void *output, mcpe_u32 capacity,
                                           mcpe_u32 *length)
{
    McpeWireWriter writer;
    int result;
    if (!packet || !length) return MCPE_WIRE_INVALID_ARGUMENT;
    *length = 0;
    if (packet->salt_length != MCPE_HANDSHAKE_SALT_BYTES) return MCPE_WIRE_INVALID_VALUE;
    mcpe_wire_writer_init(&writer, output, capacity);
    result = mcpe_handshake_write_bytes(&writer, packet->public_key,
                                        packet->public_key_length,
                                        MCPE_MAX_STRING_BYTES);
    if (result) return result;
    result = mcpe_handshake_write_bytes(&writer, packet->salt,
                                        packet->salt_length,
                                        MCPE_HANDSHAKE_SALT_BYTES);
    if (result) return result;
    *length = mcpe_wire_writer_size(&writer);
    return mcpe_wire_writer_status(&writer);
}

int mcpe_server_to_client_handshake_decode(const void *payload, mcpe_u32 length,
                                           McpeServerToClientHandshakeView *packet)
{
    McpeWireReader reader;
    int result;
    if (!packet || (!payload && length)) return MCPE_WIRE_INVALID_ARGUMENT;
    packet->public_key = 0;
    packet->public_key_length = 0;
    packet->salt = 0;
    packet->salt_length = 0;
    mcpe_wire_reader_init(&reader, payload, length);
    result = mcpe_handshake_read_bytes(&reader, &packet->public_key,
                                       &packet->public_key_length,
                                       MCPE_MAX_STRING_BYTES);
    if (result) return result;
    result = mcpe_handshake_read_bytes(&reader, &packet->salt,
                                       &packet->salt_length,
                                       MCPE_HANDSHAKE_SALT_BYTES);
    if (result) return result;
    if (packet->salt_length != MCPE_HANDSHAKE_SALT_BYTES ||
        mcpe_wire_reader_remaining(&reader) != 0) {
        reader.error = MCPE_WIRE_INVALID_VALUE;
        return MCPE_WIRE_INVALID_VALUE;
    }
    return mcpe_wire_reader_status(&reader);
}

int mcpe_client_to_server_handshake_encode(void *output, mcpe_u32 capacity,
                                           mcpe_u32 *length)
{
    if (!length) return MCPE_WIRE_INVALID_ARGUMENT;
    *length = 0;
    if (capacity && !output) return MCPE_WIRE_INVALID_ARGUMENT;
    return MCPE_WIRE_OK;
}

int mcpe_client_to_server_handshake_decode(const void *payload, mcpe_u32 length)
{
    (void)payload;
    if (length) return MCPE_WIRE_INVALID_VALUE;
    return MCPE_WIRE_OK;
}
