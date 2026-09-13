#include "login_packet.h"

static int mcpe_login_read_connection_request(McpeWireReader *reader,
                                              mcpe_u8 *output,
                                              mcpe_u32 capacity,
                                              mcpe_u32 *length)
{
    mcpe_u32 size;
    mcpe_u32 index;
    const mcpe_u8 *bytes;
    int result;
    if (!reader || !length) return MCPE_WIRE_INVALID_ARGUMENT;
    *length = 0;
    result = mcpe_wire_read_uvarint(reader, &size);
    if (result) return result;
    if (size > MCPE_MAX_BATCH_BYTES || (size && !output) || size > capacity) {
        reader->error = MCPE_WIRE_LIMIT;
        return MCPE_WIRE_LIMIT;
    }
    result = mcpe_wire_read_bytes(reader, &bytes, size);
    if (result) return result;
    for (index = 0; index < size; ++index) output[index] = bytes[index];
    *length = size;
    return MCPE_WIRE_OK;
}

static int mcpe_login_write_connection_request(McpeWireWriter *writer,
                                               const mcpe_u8 *request,
                                               mcpe_u32 length)
{
    int result;
    if (length > MCPE_MAX_BATCH_BYTES || (length && !request)) return MCPE_WIRE_LIMIT;
    result = mcpe_wire_write_uvarint(writer, length);
    if (result) return result;
    return mcpe_wire_write_bytes(writer, request, length);
}

int mcpe_login_packet_encode(const McpeLoginPacketView *packet,
                             void *output, mcpe_u32 capacity, mcpe_u32 *length)
{
    McpeWireWriter writer;
    int result;
    if (!packet || !length) return MCPE_WIRE_INVALID_ARGUMENT;
    *length = 0;
    if ((mcpe_u32)packet->protocol_version != MCPE_PROTOCOL_VERSION) return MCPE_WIRE_INVALID_VALUE;
    mcpe_wire_writer_init(&writer, output, capacity);
    result = mcpe_wire_write_be32(&writer, (mcpe_u32)packet->protocol_version);
    if (result) return result;
    result = mcpe_wire_write_u8(&writer, packet->client_network_version);
    if (result) return result;
    result = mcpe_login_write_connection_request(&writer,
                                                 packet->connection_request,
                                                 packet->connection_request_length);
    if (result) return result;
    *length = mcpe_wire_writer_size(&writer);
    return mcpe_wire_writer_status(&writer);
}

int mcpe_login_packet_decode(const void *payload, mcpe_u32 length,
                             McpeLoginPacketView *packet,
                             mcpe_u8 *connection_request,
                             mcpe_u32 connection_request_capacity)
{
    McpeWireReader reader;
    mcpe_u32 protocol;
    mcpe_u8 client_network_version;
    mcpe_u32 request_length;
    int result;
    if (!packet || (!payload && length)) return MCPE_WIRE_INVALID_ARGUMENT;
    packet->protocol_version = 0;
    packet->client_network_version = 0;
    packet->connection_request = connection_request;
    packet->connection_request_length = 0;
    mcpe_wire_reader_init(&reader, payload, length);
    result = mcpe_wire_read_be32(&reader, &protocol);
    if (result) return result;
    packet->protocol_version = (mcpe_s32)protocol;
    if (packet->protocol_version != (mcpe_s32)MCPE_PROTOCOL_VERSION) {
        reader.error = MCPE_WIRE_INVALID_VALUE;
        return MCPE_WIRE_INVALID_VALUE;
    }
    result = mcpe_wire_read_u8(&reader, &client_network_version);
    if (result) return result;
    packet->client_network_version = client_network_version;
    result = mcpe_login_read_connection_request(&reader, connection_request,
                                                connection_request_capacity,
                                                &request_length);
    if (result) return result;
    if (mcpe_wire_reader_remaining(&reader) != 0) {
        reader.error = MCPE_WIRE_INVALID_VALUE;
        return MCPE_WIRE_INVALID_VALUE;
    }
    packet->connection_request_length = request_length;
    return mcpe_wire_reader_status(&reader);
}
