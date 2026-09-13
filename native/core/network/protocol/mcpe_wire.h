#ifndef NUMC3DS_MCPE_WIRE_H
#define NUMC3DS_MCPE_WIRE_H

#include "mcpe_types.h"

enum McpeWireResult {
    MCPE_WIRE_OK = 0,
    MCPE_WIRE_INVALID_ARGUMENT = -1,
    MCPE_WIRE_TRUNCATED = -2,
    MCPE_WIRE_OVERFLOW = -3,
    MCPE_WIRE_LIMIT = -4,
    MCPE_WIRE_INVALID_VALUE = -5,
    MCPE_WIRE_NO_SPACE = -6
};

typedef struct {
    const mcpe_u8 *data;
    mcpe_u32 size;
    mcpe_u32 offset;
    int error;
} McpeWireReader;

typedef struct {
    mcpe_u8 *data;
    mcpe_u32 capacity;
    mcpe_u32 offset;
    int error;
} McpeWireWriter;

void mcpe_wire_reader_init(McpeWireReader *reader, const void *data, mcpe_u32 size);
void mcpe_wire_writer_init(McpeWireWriter *writer, void *data, mcpe_u32 capacity);
mcpe_u32 mcpe_wire_reader_remaining(const McpeWireReader *reader);
mcpe_u32 mcpe_wire_writer_size(const McpeWireWriter *writer);
int mcpe_wire_reader_status(const McpeWireReader *reader);
int mcpe_wire_writer_status(const McpeWireWriter *writer);

int mcpe_wire_read_u8(McpeWireReader *reader, mcpe_u8 *value);
int mcpe_wire_read_bool(McpeWireReader *reader, int *value);
int mcpe_wire_read_le16(McpeWireReader *reader, mcpe_u16 *value);
int mcpe_wire_read_le32(McpeWireReader *reader, mcpe_u32 *value);
int mcpe_wire_read_be16(McpeWireReader *reader, mcpe_u16 *value);
int mcpe_wire_read_be32(McpeWireReader *reader, mcpe_u32 *value);
int mcpe_wire_read_le64(McpeWireReader *reader, McpeWireU64 *value);
int mcpe_wire_read_uvarint(McpeWireReader *reader, mcpe_u32 *value);
int mcpe_wire_read_uvarint64(McpeWireReader *reader, McpeWireU64 *value);
int mcpe_wire_read_svarint64(McpeWireReader *reader, McpeWireU64 *value);
int mcpe_wire_read_bytes(McpeWireReader *reader, const mcpe_u8 **value, mcpe_u32 size);
int mcpe_wire_read_string(McpeWireReader *reader, char *value, mcpe_u32 capacity, mcpe_u32 max_bytes, mcpe_u32 *length);

int mcpe_wire_write_u8(McpeWireWriter *writer, mcpe_u8 value);
int mcpe_wire_write_bool(McpeWireWriter *writer, int value);
int mcpe_wire_write_le16(McpeWireWriter *writer, mcpe_u16 value);
int mcpe_wire_write_le32(McpeWireWriter *writer, mcpe_u32 value);
int mcpe_wire_write_be16(McpeWireWriter *writer, mcpe_u16 value);
int mcpe_wire_write_be32(McpeWireWriter *writer, mcpe_u32 value);
int mcpe_wire_write_le64(McpeWireWriter *writer, const McpeWireU64 *value);
int mcpe_wire_write_uvarint(McpeWireWriter *writer, mcpe_u32 value);
int mcpe_wire_write_uvarint64(McpeWireWriter *writer, const McpeWireU64 *value);
int mcpe_wire_write_svarint64(McpeWireWriter *writer, const McpeWireU64 *value);
int mcpe_wire_write_bytes(McpeWireWriter *writer, const void *value, mcpe_u32 size);
int mcpe_wire_write_string(McpeWireWriter *writer, const char *value, mcpe_u32 length, mcpe_u32 max_bytes);

#endif
