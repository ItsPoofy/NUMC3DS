#include "mcpe_discovery.h"
#include "../protocol/mcpe_wire.h"
#include "../raknet/raknet_transport.h"

static int mcpe_discovery_append_byte(mcpe_u8 *output, mcpe_u32 capacity, mcpe_u32 *length, mcpe_u8 value)
{
    if (!output || !length || *length >= capacity) return MCPE_WIRE_NO_SPACE;
    output[*length] = value;
    *length += 1;
    return MCPE_WIRE_OK;
}

static int mcpe_discovery_append_text(mcpe_u8 *output, mcpe_u32 capacity, mcpe_u32 *length, const char *text, mcpe_u32 text_length, int escape)
{
    mcpe_u32 i;
    int result;
    if (text_length && !text) return MCPE_WIRE_INVALID_ARGUMENT;
    for (i = 0; i < text_length; ++i) {
        if (escape && (text[i] == ';' || text[i] == '\\')) {
            result = mcpe_discovery_append_byte(output, capacity, length, '\\');
            if (result) return result;
        }
        result = mcpe_discovery_append_byte(output, capacity, length, (mcpe_u8)text[i]);
        if (result) return result;
    }
    return MCPE_WIRE_OK;
}

static int mcpe_discovery_append_u32(mcpe_u8 *output, mcpe_u32 capacity, mcpe_u32 *length, mcpe_u32 value)
{
    char digits[11];
    mcpe_u32 count = 0;
    mcpe_u32 i;
    int result;
    do {
        digits[count++] = (char)('0' + (value % 10));
        value /= 10;
    } while (value);
    for (i = 0; i < count; ++i) {
        result = mcpe_discovery_append_byte(output, capacity, length, (mcpe_u8)digits[count - i - 1]);
        if (result) return result;
    }
    return MCPE_WIRE_OK;
}

static int mcpe_discovery_append_field(mcpe_u8 *output, mcpe_u32 capacity, mcpe_u32 *length, const char *text, mcpe_u32 text_length)
{
    int result = mcpe_discovery_append_byte(output, capacity, length, ';');
    if (result) return result;
    return mcpe_discovery_append_text(output, capacity, length, text, text_length, 1);
}

static int mcpe_discovery_append_u32_field(mcpe_u8 *output, mcpe_u32 capacity, mcpe_u32 *length, mcpe_u32 value)
{
    int result = mcpe_discovery_append_byte(output, capacity, length, ';');
    if (result) return result;
    return mcpe_discovery_append_u32(output, capacity, length, value);
}

static int mcpe_discovery_copy_field(char *destination, mcpe_u32 capacity, const char *source, mcpe_u32 length)
{
    mcpe_u32 i;
    if (!destination || !source || !capacity || length >= capacity) return MCPE_WIRE_LIMIT;
    for (i = 0; i < length; ++i) destination[i] = source[i];
    destination[length] = '\0';
    return MCPE_WIRE_OK;
}

static int mcpe_discovery_magic_equal(const mcpe_u8 *left, const mcpe_u8 *right)
{
    mcpe_u32 index;
    if (!left || !right) return 0;
    for (index = 0; index < MCPE_RAKNET_OFFLINE_MAGIC_SIZE; ++index) {
        if (left[index] != right[index]) return 0;
    }
    return 1;
}

static int mcpe_discovery_parse_u32(const char *text, mcpe_u32 length, mcpe_u32 *value)
{
    mcpe_u32 result = 0;
    mcpe_u32 i;
    if (!text || !length || !value) return MCPE_WIRE_INVALID_VALUE;
    for (i = 0; i < length; ++i) {
        mcpe_u32 digit;
        if (text[i] < '0' || text[i] > '9') return MCPE_WIRE_INVALID_VALUE;
        digit = (mcpe_u32)(text[i] - '0');
        if (result > 429496729u || (result == 429496729u && digit > 5u)) return MCPE_WIRE_OVERFLOW;
        result = result * 10u + digit;
    }
    *value = result;
    return MCPE_WIRE_OK;
}

int mcpe_discovery_build_unconnected_ping(void *output, mcpe_u32 capacity,
                                          mcpe_u64 timestamp, mcpe_u64 client_guid,
                                          mcpe_u32 *length)
{
    McpeWireWriter writer;
    McpeWireU64 value;
    int result;
    if (!length) return MCPE_WIRE_INVALID_ARGUMENT;
    *length = 0;
    mcpe_wire_writer_init(&writer, output, capacity);
    result = mcpe_wire_write_u8(&writer, 0x01);
    if (result) return result;
    value.lo = (mcpe_u32)timestamp;
    value.hi = (mcpe_u32)(timestamp >> 32);
    result = mcpe_wire_write_le64(&writer, &value);
    if (result) return result;
    result = mcpe_wire_write_bytes(&writer, mcpe_raknet_offline_magic, MCPE_RAKNET_OFFLINE_MAGIC_SIZE);
    if (result) return result;
    value.lo = (mcpe_u32)client_guid;
    value.hi = (mcpe_u32)(client_guid >> 32);
    result = mcpe_wire_write_le64(&writer, &value);
    if (result) return result;
    *length = mcpe_wire_writer_size(&writer);
    return mcpe_wire_writer_status(&writer);
}

int mcpe_discovery_encode_announcement(const McpeDiscoveryAdvertisement *advertisement, void *output, mcpe_u32 capacity, mcpe_u32 *length)
{
    mcpe_u8 *bytes = (mcpe_u8 *)output;
    mcpe_u32 used = 0;
    int result;
    if (!advertisement || !output || !length) return MCPE_WIRE_INVALID_ARGUMENT;
    *length = 0;
    result = mcpe_discovery_append_text(bytes, capacity, &used, "MCPE", 4, 0);
    if (result) return result;
    result = mcpe_discovery_append_field(bytes, capacity, &used, advertisement->name, advertisement->name_length);
    if (result) return result;
    result = mcpe_discovery_append_u32_field(bytes, capacity, &used, MCPE_PROTOCOL_VERSION);
    if (result) return result;
    result = mcpe_discovery_append_field(bytes, capacity, &used, advertisement->version, advertisement->version_length);
    if (result) return result;
    result = mcpe_discovery_append_u32_field(bytes, capacity, &used, advertisement->current_players);
    if (result) return result;
    result = mcpe_discovery_append_u32_field(bytes, capacity, &used, advertisement->maximum_players);
    if (result) return result;
    result = mcpe_discovery_append_field(bytes, capacity, &used, advertisement->guid, advertisement->guid_length);
    if (result) return result;
    result = mcpe_discovery_append_field(bytes, capacity, &used, advertisement->world, advertisement->world_length);
    if (result) return result;
    result = mcpe_discovery_append_field(bytes, capacity, &used, advertisement->game_type, advertisement->game_type_length);
    if (result) return result;
    *length = used;
    return MCPE_WIRE_OK;
}

int mcpe_discovery_encode_unconnected_pong(const McpeDiscoveryAdvertisement *advertisement,
                                           mcpe_u64 timestamp, mcpe_u64 server_guid,
                                           void *output, mcpe_u32 capacity, mcpe_u32 *length)
{
    mcpe_u8 announcement[MCPE_MAX_ANNOUNCEMENT_BYTES];
    mcpe_u32 announcement_length;
    McpeWireWriter writer;
    McpeWireU64 value;
    int result;
    if (!advertisement || !output || !length) return MCPE_WIRE_INVALID_ARGUMENT;
    result = mcpe_discovery_encode_announcement(advertisement, announcement,
                                                sizeof(announcement),
                                                &announcement_length);
    if (result) return result;
    if (announcement_length > 0xffffu) return MCPE_WIRE_LIMIT;
    *length = 0;
    mcpe_wire_writer_init(&writer, output, capacity);
    result = mcpe_wire_write_u8(&writer, 0x1cu);
    if (result) return result;
    value.lo = (mcpe_u32)timestamp;
    value.hi = (mcpe_u32)(timestamp >> 32);
    result = mcpe_wire_write_le64(&writer, &value);
    if (result) return result;
    value.lo = (mcpe_u32)server_guid;
    value.hi = (mcpe_u32)(server_guid >> 32);
    result = mcpe_wire_write_le64(&writer, &value);
    if (result) return result;
    result = mcpe_wire_write_bytes(&writer, mcpe_raknet_offline_magic,
                                   MCPE_RAKNET_OFFLINE_MAGIC_SIZE);
    if (result) return result;
    result = mcpe_wire_write_be16(&writer, (mcpe_u16)announcement_length);
    if (result) return result;
    result = mcpe_wire_write_bytes(&writer, announcement, announcement_length);
    if (result) return result;
    *length = mcpe_wire_writer_size(&writer);
    return mcpe_wire_writer_status(&writer);
}

int mcpe_discovery_decode_announcement(const void *payload, mcpe_u32 length, McpeDiscoveryRecord *record)
{
    const mcpe_u8 *bytes = (const mcpe_u8 *)payload;
    char fields[16][MCPE_DISCOVERY_FIELD_BYTES];
    mcpe_u32 field_lengths[16];
    mcpe_u32 field = 0;
    mcpe_u32 field_length = 0;
    mcpe_u32 i;
    int escaped = 0;
    int result;
    if (!bytes || !record || !length) {
        return MCPE_WIRE_INVALID_VALUE;
    }
    for (i = 0; i < 16; ++i) field_lengths[i] = 0;
    for (i = 0; i < length; ++i) {
        mcpe_u8 byte = bytes[i];
        if (escaped) {
            if (field < 16 && field_length + 1 < MCPE_DISCOVERY_FIELD_BYTES) {
                fields[field][field_length++] = (char)byte;
            }
            escaped = 0;
        } else if (byte == '\\') {
            escaped = 1;
        } else if (byte == ';') {
            if (field < 16) {
                fields[field][field_length] = '\0';
                field_lengths[field++] = field_length;
            }
            field_length = 0;
        } else {
            if (field < 16 && field_length + 1 < MCPE_DISCOVERY_FIELD_BYTES) {
                fields[field][field_length++] = (char)byte;
            }
        }
    }
    if (escaped) return MCPE_WIRE_INVALID_VALUE;
    if (field < 16) {
        fields[field][field_length] = '\0';
        field_lengths[field++] = field_length;
    }
    if (field < 6 || field_lengths[0] != 4 || fields[0][0] != 'M' || fields[0][1] != 'C' || fields[0][2] != 'P' || fields[0][3] != 'E' || field_lengths[1] == 0 || field_lengths[2] == 0 || field_lengths[3] == 0 || field_lengths[4] == 0 || field_lengths[5] == 0) return MCPE_WIRE_INVALID_VALUE;
    result = mcpe_discovery_parse_u32(fields[2], field_lengths[2], &record->protocol);
    if (result) return result;
    result = mcpe_discovery_parse_u32(fields[4], field_lengths[4], &record->current_players);
    if (result) return result;
    result = mcpe_discovery_parse_u32(fields[5], field_lengths[5], &record->maximum_players);
    if (result) return result;
    result = mcpe_discovery_copy_field(record->name, sizeof(record->name), fields[1], field_lengths[1]);
    if (result) return result;
    result = mcpe_discovery_copy_field(record->version, sizeof(record->version), fields[3], field_lengths[3]);
    if (result) return result;
    if (field > 6 && field_lengths[6] > 0) {
        (void)mcpe_discovery_copy_field(record->guid, sizeof(record->guid), fields[6], field_lengths[6]);
    } else {
        record->guid[0] = '\0';
    }
    if (field > 7 && field_lengths[7] > 0) {
        (void)mcpe_discovery_copy_field(record->world, sizeof(record->world), fields[7], field_lengths[7]);
    } else {
        record->world[0] = '\0';
    }
    if (field > 8 && field_lengths[8] > 0) {
        (void)mcpe_discovery_copy_field(record->game_type, sizeof(record->game_type), fields[8], field_lengths[8]);
    } else {
        record->game_type[0] = '\0';
    }
    return MCPE_WIRE_OK;
}

int mcpe_discovery_decode_unconnected_pong(const void *payload, mcpe_u32 length,
                                           McpeDiscoveryRecord *record)
{
    const mcpe_u8 *bytes = (const mcpe_u8 *)payload;
    const mcpe_u8 *magic;
    const mcpe_u8 *announcement;
    McpeWireReader reader;
    McpeWireU64 timestamp;
    McpeWireU64 server_guid;
    mcpe_u8 packet_id;
    mcpe_u16 announcement_length;
    int result;
    (void)timestamp;
    (void)server_guid;
    if (!bytes || !record || !length) return MCPE_WIRE_INVALID_VALUE;
    mcpe_wire_reader_init(&reader, bytes, length);
    result = mcpe_wire_read_u8(&reader, &packet_id);
    if (result || packet_id != 0x1cu) return MCPE_WIRE_INVALID_VALUE;
    result = mcpe_wire_read_le64(&reader, &timestamp);
    if (result) return result;
    result = mcpe_wire_read_le64(&reader, &server_guid);
    if (result) return result;
    result = mcpe_wire_read_bytes(&reader, &magic, MCPE_RAKNET_OFFLINE_MAGIC_SIZE);
    if (result) return result;
    if (!mcpe_discovery_magic_equal(magic, mcpe_raknet_offline_magic)) {
        return MCPE_WIRE_INVALID_VALUE;
    }
    result = mcpe_wire_read_be16(&reader, &announcement_length);
    if (result) return result;
    result = mcpe_wire_read_bytes(&reader, &announcement, announcement_length);
    if (result) return result;
    if (mcpe_wire_reader_remaining(&reader) != 0) return MCPE_WIRE_INVALID_VALUE;
    return mcpe_discovery_decode_announcement(announcement, announcement_length, record);
}

int mcpe_discovery_record_is_compatible(const McpeDiscoveryRecord *record)
{
    return record && record->protocol == MCPE_PROTOCOL_VERSION;
}

static int mcpe_discovery_text_equal(const char *left, const char *right, mcpe_u32 capacity)
{
    mcpe_u32 i;
    if (!left || !right) return 0;
    for (i = 0; i < capacity; ++i) {
        if (left[i] != right[i]) return 0;
        if (!left[i]) return 1;
    }
    return 0;
}

int mcpe_discovery_record_same_identity(const McpeDiscoveryRecord *left, const McpeDiscoveryRecord *right)
{
    if (!left || !right) return 0;
    if (left->guid[0] && right->guid[0] && mcpe_discovery_text_equal(left->guid, right->guid, MCPE_DISCOVERY_GUID_BYTES)) return 1;
    return left->endpoint.address_be == right->endpoint.address_be && left->endpoint.port_be == right->endpoint.port_be;
}
