#include "mcpe_wire.h"

static int mcpe_wire_reader_take(McpeWireReader *reader, mcpe_u32 size, const mcpe_u8 **value)
{
    if (!reader || (!reader->data && reader->size) || reader->error) {
        return reader ? reader->error : MCPE_WIRE_INVALID_ARGUMENT;
    }
    if (size > reader->size - reader->offset) {
        reader->error = MCPE_WIRE_TRUNCATED;
        return reader->error;
    }
    if (value) *value = reader->data + reader->offset;
    reader->offset += size;
    return MCPE_WIRE_OK;
}

static int mcpe_wire_writer_take(McpeWireWriter *writer, mcpe_u32 size, mcpe_u8 **value)
{
    if (!writer || (!writer->data && writer->capacity) || writer->error) {
        return writer ? writer->error : MCPE_WIRE_INVALID_ARGUMENT;
    }
    if (size > writer->capacity - writer->offset) {
        writer->error = MCPE_WIRE_NO_SPACE;
        return writer->error;
    }
    if (value) *value = writer->data + writer->offset;
    writer->offset += size;
    return MCPE_WIRE_OK;
}

void mcpe_wire_reader_init(McpeWireReader *reader, const void *data, mcpe_u32 size)
{
    if (!reader) return;
    reader->data = (const mcpe_u8 *)data;
    reader->size = size;
    reader->offset = 0;
    reader->error = (!data && size) ? MCPE_WIRE_INVALID_ARGUMENT : MCPE_WIRE_OK;
}

void mcpe_wire_writer_init(McpeWireWriter *writer, void *data, mcpe_u32 capacity)
{
    if (!writer) return;
    writer->data = (mcpe_u8 *)data;
    writer->capacity = capacity;
    writer->offset = 0;
    writer->error = (!data && capacity) ? MCPE_WIRE_INVALID_ARGUMENT : MCPE_WIRE_OK;
}

mcpe_u32 mcpe_wire_reader_remaining(const McpeWireReader *reader)
{
    if (!reader || reader->offset > reader->size) return 0;
    return reader->size - reader->offset;
}

mcpe_u32 mcpe_wire_writer_size(const McpeWireWriter *writer)
{
    return writer ? writer->offset : 0;
}

int mcpe_wire_reader_status(const McpeWireReader *reader)
{
    return reader ? reader->error : MCPE_WIRE_INVALID_ARGUMENT;
}

int mcpe_wire_writer_status(const McpeWireWriter *writer)
{
    return writer ? writer->error : MCPE_WIRE_INVALID_ARGUMENT;
}

int mcpe_wire_read_u8(McpeWireReader *reader, mcpe_u8 *value)
{
    const mcpe_u8 *bytes;
    int result;
    if (!value) return MCPE_WIRE_INVALID_ARGUMENT;
    result = mcpe_wire_reader_take(reader, 1, &bytes);
    if (result) return result;
    *value = bytes[0];
    return MCPE_WIRE_OK;
}

int mcpe_wire_read_bool(McpeWireReader *reader, int *value)
{
    mcpe_u8 byte;
    int result;
    if (!value) return MCPE_WIRE_INVALID_ARGUMENT;
    result = mcpe_wire_read_u8(reader, &byte);
    if (result) return result;
    if (byte > 1) {
        if (reader) reader->error = MCPE_WIRE_INVALID_VALUE;
        return MCPE_WIRE_INVALID_VALUE;
    }
    *value = byte ? 1 : 0;
    return MCPE_WIRE_OK;
}

int mcpe_wire_read_le16(McpeWireReader *reader, mcpe_u16 *value)
{
    const mcpe_u8 *bytes;
    int result;
    if (!value) return MCPE_WIRE_INVALID_ARGUMENT;
    result = mcpe_wire_reader_take(reader, 2, &bytes);
    if (result) return result;
    *value = (mcpe_u16)bytes[0] | ((mcpe_u16)bytes[1] << 8);
    return MCPE_WIRE_OK;
}

int mcpe_wire_read_le32(McpeWireReader *reader, mcpe_u32 *value)
{
    const mcpe_u8 *bytes;
    int result;
    if (!value) return MCPE_WIRE_INVALID_ARGUMENT;
    result = mcpe_wire_reader_take(reader, 4, &bytes);
    if (result) return result;
    *value = (mcpe_u32)bytes[0] | ((mcpe_u32)bytes[1] << 8) |
             ((mcpe_u32)bytes[2] << 16) | ((mcpe_u32)bytes[3] << 24);
    return MCPE_WIRE_OK;
}

int mcpe_wire_read_be16(McpeWireReader *reader, mcpe_u16 *value)
{
    const mcpe_u8 *bytes;
    int result;
    if (!value) return MCPE_WIRE_INVALID_ARGUMENT;
    result = mcpe_wire_reader_take(reader, 2, &bytes);
    if (result) return result;
    *value = ((mcpe_u16)bytes[0] << 8) | (mcpe_u16)bytes[1];
    return MCPE_WIRE_OK;
}

int mcpe_wire_read_be32(McpeWireReader *reader, mcpe_u32 *value)
{
    const mcpe_u8 *bytes;
    int result;
    if (!value) return MCPE_WIRE_INVALID_ARGUMENT;
    result = mcpe_wire_reader_take(reader, 4, &bytes);
    if (result) return result;
    *value = ((mcpe_u32)bytes[0] << 24) | ((mcpe_u32)bytes[1] << 16) |
             ((mcpe_u32)bytes[2] << 8) | (mcpe_u32)bytes[3];
    return MCPE_WIRE_OK;
}

int mcpe_wire_read_le64(McpeWireReader *reader, McpeWireU64 *value)
{
    int result;
    if (!value) return MCPE_WIRE_INVALID_ARGUMENT;
    result = mcpe_wire_read_le32(reader, &value->lo);
    if (result) return result;
    return mcpe_wire_read_le32(reader, &value->hi);
}

int mcpe_wire_read_uvarint(McpeWireReader *reader, mcpe_u32 *value)
{
    mcpe_u32 result = 0;
    mcpe_u32 shift = 0;
    mcpe_u8 byte;
    mcpe_u32 count;
    int status;
    if (!value) return MCPE_WIRE_INVALID_ARGUMENT;
    for (count = 0; count < 5; ++count) {
        status = mcpe_wire_read_u8(reader, &byte);
        if (status) return status;
        if (count == 4 && byte > 0x0f) {
            if (reader) reader->error = MCPE_WIRE_OVERFLOW;
            return MCPE_WIRE_OVERFLOW;
        }
        result |= ((mcpe_u32)(byte & 0x7f)) << shift;
        if (!(byte & 0x80)) {
            *value = result;
            return MCPE_WIRE_OK;
        }
        shift += 7;
    }
    if (reader) reader->error = MCPE_WIRE_OVERFLOW;
    return MCPE_WIRE_OVERFLOW;
}

int mcpe_wire_read_uvarint64(McpeWireReader *reader, McpeWireU64 *value)
{
    mcpe_u64 result = 0;
    mcpe_u8 byte;
    mcpe_u32 count;
    int status;
    if (!value) return MCPE_WIRE_INVALID_ARGUMENT;
    for (count = 0; count < 10; ++count) {
        status = mcpe_wire_read_u8(reader, &byte);
        if (status) return status;
        if (count == 9 && byte > 1) {
            if (reader) reader->error = MCPE_WIRE_OVERFLOW;
            return MCPE_WIRE_OVERFLOW;
        }
        result |= ((mcpe_u64)(byte & 0x7f)) << (count * 7);
        if (!(byte & 0x80)) {
            value->lo = (mcpe_u32)result;
            value->hi = (mcpe_u32)(result >> 32);
            return MCPE_WIRE_OK;
        }
    }
    if (reader) reader->error = MCPE_WIRE_OVERFLOW;
    return MCPE_WIRE_OVERFLOW;
}

int mcpe_wire_read_svarint64(McpeWireReader *reader, McpeWireU64 *value)
{
    McpeWireU64 encoded;
    mcpe_u64 raw;
    mcpe_u64 sign;
    int result;
    if (!value) return MCPE_WIRE_INVALID_ARGUMENT;
    result = mcpe_wire_read_uvarint64(reader, &encoded);
    if (result) return result;
    raw = ((mcpe_u64)encoded.hi << 32) | encoded.lo;
    sign = 0 - (raw & 1u);
    raw = (raw >> 1) ^ sign;
    value->lo = (mcpe_u32)raw;
    value->hi = (mcpe_u32)(raw >> 32);
    return MCPE_WIRE_OK;
}

int mcpe_wire_read_bytes(McpeWireReader *reader, const mcpe_u8 **value, mcpe_u32 size)
{
    if (!value) return MCPE_WIRE_INVALID_ARGUMENT;
    return mcpe_wire_reader_take(reader, size, value);
}

int mcpe_wire_read_string(McpeWireReader *reader, char *value, mcpe_u32 capacity, mcpe_u32 max_bytes, mcpe_u32 *length)
{
    mcpe_u32 size;
    const mcpe_u8 *bytes;
    int result;
    result = mcpe_wire_read_uvarint(reader, &size);
    if (result) return result;
    if (size > max_bytes || (value && size >= capacity)) {
        if (reader) reader->error = MCPE_WIRE_LIMIT;
        return MCPE_WIRE_LIMIT;
    }
    result = mcpe_wire_reader_take(reader, size, &bytes);
    if (result) return result;
    if (size && !value) {
        if (reader) reader->error = MCPE_WIRE_INVALID_ARGUMENT;
        return MCPE_WIRE_INVALID_ARGUMENT;
    }
    if (size) {
        mcpe_u32 i;
        for (i = 0; i < size; ++i) value[i] = (char)bytes[i];
    }
    if (value) value[size] = '\0';
    if (length) *length = size;
    return MCPE_WIRE_OK;
}

int mcpe_wire_write_u8(McpeWireWriter *writer, mcpe_u8 value)
{
    mcpe_u8 *bytes;
    int result = mcpe_wire_writer_take(writer, 1, &bytes);
    if (result) return result;
    bytes[0] = value;
    return MCPE_WIRE_OK;
}

int mcpe_wire_write_bool(McpeWireWriter *writer, int value)
{
    if (value != 0 && value != 1) return MCPE_WIRE_INVALID_VALUE;
    return mcpe_wire_write_u8(writer, (mcpe_u8)value);
}

int mcpe_wire_write_le16(McpeWireWriter *writer, mcpe_u16 value)
{
    mcpe_u8 *bytes;
    int result = mcpe_wire_writer_take(writer, 2, &bytes);
    if (result) return result;
    bytes[0] = (mcpe_u8)value;
    bytes[1] = (mcpe_u8)(value >> 8);
    return MCPE_WIRE_OK;
}

int mcpe_wire_write_le32(McpeWireWriter *writer, mcpe_u32 value)
{
    mcpe_u8 *bytes;
    int result = mcpe_wire_writer_take(writer, 4, &bytes);
    if (result) return result;
    bytes[0] = (mcpe_u8)value;
    bytes[1] = (mcpe_u8)(value >> 8);
    bytes[2] = (mcpe_u8)(value >> 16);
    bytes[3] = (mcpe_u8)(value >> 24);
    return MCPE_WIRE_OK;
}

int mcpe_wire_write_be16(McpeWireWriter *writer, mcpe_u16 value)
{
    mcpe_u8 *bytes;
    int result = mcpe_wire_writer_take(writer, 2, &bytes);
    if (result) return result;
    bytes[0] = (mcpe_u8)(value >> 8);
    bytes[1] = (mcpe_u8)value;
    return MCPE_WIRE_OK;
}

int mcpe_wire_write_be32(McpeWireWriter *writer, mcpe_u32 value)
{
    mcpe_u8 *bytes;
    int result = mcpe_wire_writer_take(writer, 4, &bytes);
    if (result) return result;
    bytes[0] = (mcpe_u8)(value >> 24);
    bytes[1] = (mcpe_u8)(value >> 16);
    bytes[2] = (mcpe_u8)(value >> 8);
    bytes[3] = (mcpe_u8)value;
    return MCPE_WIRE_OK;
}

int mcpe_wire_write_le64(McpeWireWriter *writer, const McpeWireU64 *value)
{
    int result;
    if (!value) return MCPE_WIRE_INVALID_ARGUMENT;
    result = mcpe_wire_write_le32(writer, value->lo);
    if (result) return result;
    return mcpe_wire_write_le32(writer, value->hi);
}

int mcpe_wire_write_uvarint(McpeWireWriter *writer, mcpe_u32 value)
{
    mcpe_u8 byte;
    int result;
    do {
        byte = (mcpe_u8)(value & 0x7f);
        value >>= 7;
        if (value) byte |= 0x80;
        result = mcpe_wire_write_u8(writer, byte);
        if (result) return result;
    } while (value);
    return MCPE_WIRE_OK;
}

int mcpe_wire_write_uvarint64(McpeWireWriter *writer, const McpeWireU64 *value)
{
    mcpe_u64 remaining;
    mcpe_u8 byte;
    int result;
    if (!value) return MCPE_WIRE_INVALID_ARGUMENT;
    remaining = ((mcpe_u64)value->hi << 32) | value->lo;
    do {
        byte = (mcpe_u8)(remaining & 0x7f);
        remaining >>= 7;
        if (remaining) byte |= 0x80;
        result = mcpe_wire_write_u8(writer, byte);
        if (result) return result;
    } while (remaining);
    return MCPE_WIRE_OK;
}

int mcpe_wire_write_svarint64(McpeWireWriter *writer, const McpeWireU64 *value)
{
    McpeWireU64 encoded;
    mcpe_u64 raw;
    mcpe_u64 sign;
    if (!value) return MCPE_WIRE_INVALID_ARGUMENT;
    raw = ((mcpe_u64)value->hi << 32) | value->lo;
    sign = 0 - (raw >> 63);
    raw = (raw << 1) ^ sign;
    encoded.lo = (mcpe_u32)raw;
    encoded.hi = (mcpe_u32)(raw >> 32);
    return mcpe_wire_write_uvarint64(writer, &encoded);
}

int mcpe_wire_write_bytes(McpeWireWriter *writer, const void *value, mcpe_u32 size)
{
    mcpe_u8 *bytes;
    mcpe_u32 i;
    int result;
    if (size && !value) return MCPE_WIRE_INVALID_ARGUMENT;
    result = mcpe_wire_writer_take(writer, size, &bytes);
    if (result) return result;
    for (i = 0; i < size; ++i) bytes[i] = ((const mcpe_u8 *)value)[i];
    return MCPE_WIRE_OK;
}

int mcpe_wire_write_string(McpeWireWriter *writer, const char *value, mcpe_u32 length, mcpe_u32 max_bytes)
{
    int result;
    if (length > max_bytes || (length && !value)) return MCPE_WIRE_LIMIT;
    result = mcpe_wire_write_uvarint(writer, length);
    if (result) return result;
    return mcpe_wire_write_bytes(writer, value, length);
}
