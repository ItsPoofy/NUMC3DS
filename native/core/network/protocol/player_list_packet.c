#include "player_list_packet.h"
#include <string.h>

static int mcpe_player_list_write_entry(McpeWireWriter *writer,
                                        const McpePlayerListEntry *entry)
{
    int result;
    if (!writer || !entry) return MCPE_WIRE_INVALID_ARGUMENT;
    result = mcpe_wire_write_bytes(writer, entry->uuid, MCPE_PLAYER_UUID_BYTES);
    if (result) return result;
    result = mcpe_wire_write_svarint64(writer, &entry->entity_unique_id);
    if (result) return result;
    result = mcpe_wire_write_string(writer, entry->name, entry->name_length,
                                    MCPE_MAX_STRING_BYTES);
    if (result) return result;
    result = mcpe_wire_write_string(writer, entry->skin_id, entry->skin_id_length,
                                    MCPE_MAX_STRING_BYTES);
    if (result) return result;
    return mcpe_wire_write_string(writer, (const char *)entry->skin_data,
                                  entry->skin_data_length, MCPE_MAX_BATCH_BYTES);
}

static int mcpe_player_list_read_string(McpeWireReader *reader,
                                        const mcpe_u8 **value,
                                        mcpe_u32 *length,
                                        mcpe_u32 maximum)
{
    mcpe_u32 size;
    int result;
    if (!reader || !value || !length) return MCPE_WIRE_INVALID_ARGUMENT;
    result = mcpe_wire_read_uvarint(reader, &size);
    if (result) return result;
    if (size > maximum) {
        reader->error = MCPE_WIRE_LIMIT;
        return MCPE_WIRE_LIMIT;
    }
    result = mcpe_wire_read_bytes(reader, value, size);
    if (result) return result;
    *length = size;
    return MCPE_WIRE_OK;
}

static int mcpe_player_list_read_entry(McpeWireReader *reader,
                                       McpePlayerListEntry *entry)
{
    const mcpe_u8 *value;
    int result;
    if (!reader || !entry) return MCPE_WIRE_INVALID_ARGUMENT;
    result = mcpe_wire_read_bytes(reader, &value, MCPE_PLAYER_UUID_BYTES);
    if (result) return result;
    memcpy(entry->uuid, value, MCPE_PLAYER_UUID_BYTES);
    result = mcpe_wire_read_svarint64(reader, &entry->entity_unique_id);
    if (result) return result;
    result = mcpe_player_list_read_string(reader, (const mcpe_u8 **)&entry->name,
                                          &entry->name_length, MCPE_MAX_STRING_BYTES);
    if (result) return result;
    result = mcpe_player_list_read_string(reader, (const mcpe_u8 **)&entry->skin_id,
                                          &entry->skin_id_length, MCPE_MAX_STRING_BYTES);
    if (result) return result;
    return mcpe_player_list_read_string(reader, &entry->skin_data,
                                        &entry->skin_data_length, MCPE_MAX_BATCH_BYTES);
}

int mcpe_player_list_encode(const McpePlayerListView *packet,
                            void *output, mcpe_u32 capacity,
                            mcpe_u32 *length)
{
    McpeWireWriter writer;
    mcpe_u32 index;
    int result;
    if (!packet || !length) return MCPE_WIRE_INVALID_ARGUMENT;
    if (packet->action != MCPE_PLAYER_LIST_ADD && packet->action != MCPE_PLAYER_LIST_REMOVE) {
        return MCPE_WIRE_INVALID_VALUE;
    }
    if (packet->count && !packet->entries) return MCPE_WIRE_INVALID_ARGUMENT;
    *length = 0;
    mcpe_wire_writer_init(&writer, output, capacity);
    result = mcpe_wire_write_u8(&writer, packet->action);
    if (result) return result;
    result = mcpe_wire_write_uvarint(&writer, packet->count);
    if (result) return result;
    for (index = 0; index < packet->count; ++index) {
        if (packet->action == MCPE_PLAYER_LIST_ADD) {
            result = mcpe_player_list_write_entry(&writer, &packet->entries[index]);
        } else {
            result = mcpe_wire_write_bytes(&writer, packet->entries[index].uuid,
                                           MCPE_PLAYER_UUID_BYTES);
        }
        if (result) return result;
    }
    *length = mcpe_wire_writer_size(&writer);
    return mcpe_wire_writer_status(&writer);
}

int mcpe_player_list_decode(const void *payload, mcpe_u32 length,
                            McpePlayerListView *packet)
{
    McpeWireReader reader;
    mcpe_u8 action;
    mcpe_u32 count;
    mcpe_u32 index;
    int result;
    if (!packet || (!payload && length)) return MCPE_WIRE_INVALID_ARGUMENT;
    packet->count = 0;
    mcpe_wire_reader_init(&reader, payload, length);
    result = mcpe_wire_read_u8(&reader, &action);
    if (result) return result;
    if (action != MCPE_PLAYER_LIST_ADD && action != MCPE_PLAYER_LIST_REMOVE) {
        reader.error = MCPE_WIRE_INVALID_VALUE;
        return MCPE_WIRE_INVALID_VALUE;
    }
    result = mcpe_wire_read_uvarint(&reader, &count);
    if (result) return result;
    if (count > packet->capacity || (count && !packet->entries)) {
        reader.error = MCPE_WIRE_LIMIT;
        return MCPE_WIRE_LIMIT;
    }
    packet->action = action;
    for (index = 0; index < count; ++index) {
        if (action == MCPE_PLAYER_LIST_ADD) {
            result = mcpe_player_list_read_entry(&reader, &packet->entries[index]);
        } else {
            const mcpe_u8 *uuid;
            result = mcpe_wire_read_bytes(&reader, &uuid, MCPE_PLAYER_UUID_BYTES);
            if (!result) memcpy(packet->entries[index].uuid, uuid, MCPE_PLAYER_UUID_BYTES);
        }
        if (result) return result;
    }
    packet->count = count;
    if (mcpe_wire_reader_remaining(&reader) != 0) {
        reader.error = MCPE_WIRE_INVALID_VALUE;
        return MCPE_WIRE_INVALID_VALUE;
    }
    return mcpe_wire_reader_status(&reader);
}
