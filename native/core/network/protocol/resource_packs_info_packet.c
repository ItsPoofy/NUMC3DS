#include "resource_packs_info_packet.h"

static int mcpe_resource_pack_info_write_entry(McpeWireWriter *writer,
                                                const McpeResourcePackInfoEntry *entry)
{
    int result;
    if (!entry) return MCPE_WIRE_INVALID_ARGUMENT;
    result = mcpe_wire_write_string(writer, entry->uuid, entry->uuid_length, MCPE_MAX_STRING_BYTES);
    if (result) return result;
    result = mcpe_wire_write_string(writer, entry->version, entry->version_length, MCPE_MAX_STRING_BYTES);
    if (result) return result;
    result = mcpe_wire_write_le64(writer, &entry->content_size);
    if (result) return result;
    return mcpe_wire_write_string(writer, entry->content_key, entry->content_key_length, MCPE_MAX_STRING_BYTES);
}

static int mcpe_resource_pack_info_read_string(McpeWireReader *reader,
                                                const mcpe_u8 **value,
                                                mcpe_u32 *length,
                                                int allow_empty)
{
    mcpe_u32 size;
    int result;
    if (!reader || !value || !length) return MCPE_WIRE_INVALID_ARGUMENT;
    result = mcpe_wire_read_uvarint(reader, &size);
    if (result) return result;
    if (size > MCPE_MAX_STRING_BYTES || (!allow_empty && !size)) {
        reader->error = MCPE_WIRE_INVALID_VALUE;
        return MCPE_WIRE_INVALID_VALUE;
    }
    if (size) {
        result = mcpe_wire_read_bytes(reader, value, size);
        if (result) return result;
    } else {
        *value = 0;
    }
    *length = size;
    return MCPE_WIRE_OK;
}

static int mcpe_resource_pack_info_read_entry(McpeWireReader *reader,
                                               McpeResourcePackInfoEntry *entry)
{
    const mcpe_u8 *uuid = 0;
    const mcpe_u8 *version = 0;
    const mcpe_u8 *content_key = 0;
    mcpe_u32 uuid_length = 0;
    mcpe_u32 version_length = 0;
    mcpe_u32 content_key_length = 0;
    int result;
    if (!reader || !entry) return MCPE_WIRE_INVALID_ARGUMENT;
    result = mcpe_resource_pack_info_read_string(reader, &uuid, &uuid_length, 0);
    if (result) return result;
    result = mcpe_resource_pack_info_read_string(reader, &version, &version_length, 0);
    if (result) return result;
    result = mcpe_wire_read_le64(reader, &entry->content_size);
    if (result) return result;
    result = mcpe_resource_pack_info_read_string(reader, &content_key, &content_key_length, 1);
    if (result) return result;
    entry->uuid = (const char *)uuid;
    entry->uuid_length = uuid_length;
    entry->version = (const char *)version;
    entry->version_length = version_length;
    entry->content_key = (const char *)content_key;
    entry->content_key_length = content_key_length;
    return MCPE_WIRE_OK;
}

static int mcpe_resource_pack_info_write_entries(McpeWireWriter *writer,
                                                 const McpeResourcePackInfoEntry *entries,
                                                 mcpe_u16 count)
{
    mcpe_u32 index;
    int result;
    for (index = 0; index < count; ++index) {
        result = mcpe_resource_pack_info_write_entry(writer, &entries[index]);
        if (result) return result;
    }
    return MCPE_WIRE_OK;
}

static int mcpe_resource_pack_info_read_entries(McpeWireReader *reader,
                                                McpeResourcePackInfoEntry *entries,
                                                mcpe_u16 capacity,
                                                mcpe_u16 *count)
{
    mcpe_u16 decoded_count;
    mcpe_u32 index;
    int result;
    if (!reader || !count) return MCPE_WIRE_INVALID_ARGUMENT;
    result = mcpe_wire_read_le16(reader, &decoded_count);
    if (result) return result;
    if (decoded_count > capacity || (decoded_count && !entries)) {
        reader->error = MCPE_WIRE_LIMIT;
        return MCPE_WIRE_LIMIT;
    }
    for (index = 0; index < decoded_count; ++index) {
        result = mcpe_resource_pack_info_read_entry(reader, &entries[index]);
        if (result) return result;
    }
    *count = decoded_count;
    return MCPE_WIRE_OK;
}

int mcpe_resource_packs_info_encode(const McpeResourcePacksInfoView *packet,
                                    void *output, mcpe_u32 capacity,
                                    mcpe_u32 *length)
{
    McpeWireWriter writer;
    int result;
    if (!packet || !length) return MCPE_WIRE_INVALID_ARGUMENT;
    if (packet->behavior_count && !packet->behavior_packs) return MCPE_WIRE_INVALID_ARGUMENT;
    if (packet->resource_count && !packet->resource_packs) return MCPE_WIRE_INVALID_ARGUMENT;
    *length = 0;
    mcpe_wire_writer_init(&writer, output, capacity);
    result = mcpe_wire_write_bool(&writer, packet->must_accept);
    if (result) return result;
    result = mcpe_wire_write_le16(&writer, packet->behavior_count);
    if (result) return result;
    result = mcpe_resource_pack_info_write_entries(&writer, packet->behavior_packs, packet->behavior_count);
    if (result) return result;
    result = mcpe_wire_write_le16(&writer, packet->resource_count);
    if (result) return result;
    result = mcpe_resource_pack_info_write_entries(&writer, packet->resource_packs, packet->resource_count);
    if (result) return result;
    *length = mcpe_wire_writer_size(&writer);
    return mcpe_wire_writer_status(&writer);
}

int mcpe_resource_packs_info_decode(const void *payload, mcpe_u32 length,
                                    McpeResourcePacksInfoView *packet)
{
    McpeWireReader reader;
    int result;
    if (!packet || (!payload && length)) return MCPE_WIRE_INVALID_ARGUMENT;
    packet->behavior_count = 0;
    packet->resource_count = 0;
    mcpe_wire_reader_init(&reader, payload, length);
    result = mcpe_wire_read_bool(&reader, &packet->must_accept);
    if (result) return result;
    result = mcpe_resource_pack_info_read_entries(&reader, packet->behavior_packs,
                                                  packet->behavior_capacity,
                                                  &packet->behavior_count);
    if (result) return result;
    result = mcpe_resource_pack_info_read_entries(&reader, packet->resource_packs,
                                                  packet->resource_capacity,
                                                  &packet->resource_count);
    if (result) return result;
    if (mcpe_wire_reader_remaining(&reader) != 0) {
        reader.error = MCPE_WIRE_INVALID_VALUE;
        return MCPE_WIRE_INVALID_VALUE;
    }
    return mcpe_wire_reader_status(&reader);
}
