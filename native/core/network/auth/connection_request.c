#include "connection_request.h"

static int mcpe_connection_request_write_long_string(McpeWireWriter *writer,
                                                      const mcpe_u8 *value,
                                                      mcpe_u32 length,
                                                      int allow_empty)
{
    int result;
    if (length > MCPE_MAX_BATCH_BYTES || (length && !value) || (!allow_empty && !length)) return MCPE_WIRE_INVALID_VALUE;
    result = mcpe_wire_write_le32(writer, length);
    if (result) return result;
    return mcpe_wire_write_bytes(writer, value, length);
}

static int mcpe_connection_request_read_long_string(McpeWireReader *reader,
                                                     const mcpe_u8 **value,
                                                     mcpe_u32 *length,
                                                     int allow_empty)
{
    mcpe_u32 size;
    int result;
    if (!reader || !value || !length) return MCPE_WIRE_INVALID_ARGUMENT;
    result = mcpe_wire_read_le32(reader, &size);
    if (result) return result;
    if (size > MCPE_MAX_BATCH_BYTES || (!allow_empty && !size)) {
        reader->error = MCPE_WIRE_INVALID_VALUE;
        return MCPE_WIRE_INVALID_VALUE;
    }
    result = mcpe_wire_read_bytes(reader, value, size);
    if (result) return result;
    *length = size;
    return MCPE_WIRE_OK;
}

int mcpe_connection_request_encode(const McpeConnectionRequestView *request,
                                   void *output, mcpe_u32 capacity,
                                   mcpe_u32 *length)
{
    McpeWireWriter writer;
    int result;
    if (!request || !length) return MCPE_WIRE_INVALID_ARGUMENT;
    *length = 0;
    mcpe_wire_writer_init(&writer, output, capacity);
    result = mcpe_connection_request_write_long_string(&writer,
                                                       request->certificate,
                                                       request->certificate_length,
                                                       0);
    if (result) return result;
    result = mcpe_connection_request_write_long_string(&writer,
                                                       request->web_token,
                                                       request->web_token_length,
                                                       1);
    if (result) return result;
    *length = mcpe_wire_writer_size(&writer);
    return mcpe_wire_writer_status(&writer);
}

int mcpe_connection_request_decode(const void *payload, mcpe_u32 length,
                                   McpeConnectionRequestView *request)
{
    McpeWireReader reader;
    int result;
    if (!request || (!payload && length)) return MCPE_WIRE_INVALID_ARGUMENT;
    request->certificate = 0;
    request->certificate_length = 0;
    request->web_token = 0;
    request->web_token_length = 0;
    mcpe_wire_reader_init(&reader, payload, length);
    result = mcpe_connection_request_read_long_string(&reader,
                                                      &request->certificate,
                                                      &request->certificate_length,
                                                      0);
    if (result) return result;
    result = mcpe_connection_request_read_long_string(&reader,
                                                      &request->web_token,
                                                      &request->web_token_length,
                                                      1);
    if (result) return result;
    if (mcpe_wire_reader_remaining(&reader) != 0) {
        reader.error = MCPE_WIRE_INVALID_VALUE;
        return MCPE_WIRE_INVALID_VALUE;
    }
    return mcpe_wire_reader_status(&reader);
}
