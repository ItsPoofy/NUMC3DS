#include "resource_pack_stack_packet.h"

static int mcpe_resource_pack_stack_write_entry(McpeWireWriter *writer,
                                                 const McpePackIdVersion *entry)
{
    int result;
    if (!writer || !entry) return MCPE_WIRE_INVALID_ARGUMENT;
    result = mcpe_wire_write_string(writer, entry->pack_id, entry->pack_id_length,
                                    MCPE_MAX_STRING_BYTES);
    if (result) return result;
    return mcpe_wire_write_string(writer, entry->version, entry->version_length,
                                  MCPE_MAX_STRING_BYTES);
}

static int mcpe_resource_pack_stack_read_string(McpeWireReader *reader,
                                                 const char **value,
                                                 mcpe_u32 *length)
{
    mcpe_u32 size;
    const mcpe_u8 *bytes;
    int result;
    if (!reader || !value || !length) return MCPE_WIRE_INVALID_ARGUMENT;
    result = mcpe_wire_read_uvarint(reader, &size);
    if (result) return result;
    if (size > MCPE_MAX_STRING_BYTES) {
        reader->error = MCPE_WIRE_LIMIT;
        return MCPE_WIRE_LIMIT;
    }
    result = mcpe_wire_read_bytes(reader, &bytes, size);
    if (result) return result;
    *value = (const char *)bytes;
    *length = size;
    return MCPE_WIRE_OK;
}

static int mcpe_resource_pack_stack_read_entry(McpeWireReader *reader,
                                                McpePackIdVersion *entry)
{
    int result;
    if (!reader || !entry) return MCPE_WIRE_INVALID_ARGUMENT;
    result = mcpe_resource_pack_stack_read_string(reader, &entry->pack_id,
                                                  &entry->pack_id_length);
    if (result) return result;
    return mcpe_resource_pack_stack_read_string(reader, &entry->version,
                                                &entry->version_length);
}

static int mcpe_resource_pack_stack_write_entries(McpeWireWriter *writer,
                                                  const McpePackIdVersion *entries,
                                                  mcpe_u32 count)
{
    mcpe_u32 index;
    int result;
    if (count && !entries) return MCPE_WIRE_INVALID_ARGUMENT;
    result = mcpe_wire_write_uvarint(writer, count);
    if (result) return result;
    for (index = 0; index < count; ++index) {
        result = mcpe_resource_pack_stack_write_entry(writer, &entries[index]);
        if (result) return result;
    }
    return MCPE_WIRE_OK;
}

static int mcpe_resource_pack_stack_read_entries(McpeWireReader *reader,
                                                 McpePackIdVersion *entries,
                                                 mcpe_u32 capacity,
                                                 mcpe_u32 *count)
{
    mcpe_u32 decoded_count;
    mcpe_u32 index;
    int result;
    if (!reader || !count) return MCPE_WIRE_INVALID_ARGUMENT;
    result = mcpe_wire_read_uvarint(reader, &decoded_count);
    if (result) return result;
    if (decoded_count > capacity || (decoded_count && !entries)) {
        reader->error = MCPE_WIRE_LIMIT;
        return MCPE_WIRE_LIMIT;
    }
    for (index = 0; index < decoded_count; ++index) {
        result = mcpe_resource_pack_stack_read_entry(reader, &entries[index]);
        if (result) return result;
    }
    *count = decoded_count;
    return MCPE_WIRE_OK;
}

int mcpe_resource_pack_stack_encode(const McpeResourcePackStackView *packet,
                                    void *output, mcpe_u32 capacity,
                                    mcpe_u32 *length)
{
    McpeWireWriter writer;
    int result;
    if (!packet || !length) return MCPE_WIRE_INVALID_ARGUMENT;
    *length = 0;
    mcpe_wire_writer_init(&writer, output, capacity);
    result = mcpe_wire_write_bool(&writer, packet->must_accept);
    if (result) return result;
    result = mcpe_resource_pack_stack_write_entries(&writer, packet->behavior_packs,
                                                    packet->behavior_count);
    if (result) return result;
    result = mcpe_resource_pack_stack_write_entries(&writer, packet->resource_packs,
                                                    packet->resource_count);
    if (result) return result;
    *length = mcpe_wire_writer_size(&writer);
    return mcpe_wire_writer_status(&writer);
}

int mcpe_resource_pack_stack_decode(const void *payload, mcpe_u32 length,
                                    McpeResourcePackStackView *packet)
{
    McpeWireReader reader;
    int result;
    if (!packet || (!payload && length)) return MCPE_WIRE_INVALID_ARGUMENT;
    packet->behavior_count = 0;
    packet->resource_count = 0;
    mcpe_wire_reader_init(&reader, payload, length);
    result = mcpe_wire_read_bool(&reader, &packet->must_accept);
    if (result) return result;
    result = mcpe_resource_pack_stack_read_entries(&reader, packet->behavior_packs,
                                                   packet->behavior_capacity,
                                                   &packet->behavior_count);
    if (result) return result;
    result = mcpe_resource_pack_stack_read_entries(&reader, packet->resource_packs,
                                                   packet->resource_capacity,
                                                   &packet->resource_count);
    if (result) return result;
    if (mcpe_wire_reader_remaining(&reader) != 0) {
        reader.error = MCPE_WIRE_INVALID_VALUE;
        return MCPE_WIRE_INVALID_VALUE;
    }
    return mcpe_wire_reader_status(&reader);
}
