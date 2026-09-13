#include "mcpe_login_builder.h"
#include "connection_request.h"
#include "mcpe_identity.h"
#include "mcpe_jwt.h"
#include "../../../diagnostics/network_debug.h"
#include "../session/mcpe_transport_selector.h"
#include "../../skin/skin_wire.h"
#include "../../state.h"
#include "../../seams.h"

typedef unsigned int *(*GstdAllocateFn)(unsigned int, unsigned int, unsigned int);
typedef void (*GstdDtorFn)(void *);

typedef struct {
    char *bytes;
    mcpe_u32 capacity;
    mcpe_u32 length;
    int valid;
} McpeLoginText;

static int gstd_allocate(NativeGstdString *value, mcpe_u32 capacity, mcpe_u32 length)
{
    unsigned int *base;
    if (!value || capacity < length || capacity > 0x00fffff0u) return 0;
    value->handle = 0;
    base = ((GstdAllocateFn)SEAM_gstd_string_allocate)(0, capacity, length);
    if (!base) return 0;
    value->handle = (unsigned int)(base + 3);
    return 1;
}

static void gstd_destroy(NativeGstdString *value)
{
    if (!value || !value->handle) return;
    ((GstdDtorFn)SEAM_StrDtor)(value);
    value->handle = 0;
}

static int login_text_allocate(McpeLoginText *text, mcpe_u32 capacity)
{
    if (!text || !s || !s->host.heap_alloc || !capacity) return 0;
    text->bytes = (char *)s->host.heap_alloc(capacity);
    if (!text->bytes) return 0;
    text->capacity = capacity;
    text->length = 0;
    text->valid = 1;
    text->bytes[0] = 0;
    return 1;
}

static void login_text_destroy(McpeLoginText *text)
{
    if (!text) return;
    if (text->bytes && s && s->host.heap_free) s->host.heap_free(text->bytes);
    zero(text, sizeof(*text));
}

static int login_text_append_bytes(McpeLoginText *text, const char *value, mcpe_u32 length)
{
    mcpe_u32 index;
    if (!text || !text->valid || (length && !value) ||
        text->length > text->capacity || length >= text->capacity - text->length) return 0;
    for (index = 0; index < length; ++index) text->bytes[text->length + index] = value[index];
    text->length += length;
    text->bytes[text->length] = 0;
    return 1;
}

static int login_text_append_literal(McpeLoginText *text, const char *value)
{
    mcpe_u32 length = 0;
    if (!value) return 0;
    while (value[length]) ++length;
    return login_text_append_bytes(text, value, length);
}

static int login_text_append_byte(McpeLoginText *text, char value)
{
    return login_text_append_bytes(text, &value, 1u);
}

static int login_text_append_json_string(McpeLoginText *text, const char *value)
{
    mcpe_u32 index = 0;
    if (!text || !value || !login_text_append_byte(text, '"')) return 0;
    while (value[index]) {
        unsigned char character = (unsigned char)value[index++];
        if (character == '"' || character == '\\') {
            if (!login_text_append_byte(text, '\\') || !login_text_append_byte(text, (char)character)) return 0;
        } else if (character == '\n') {
            if (!login_text_append_literal(text, "\\n")) return 0;
        } else if (character == '\r') {
            if (!login_text_append_literal(text, "\\r")) return 0;
        } else if (character == '\t') {
            if (!login_text_append_literal(text, "\\t")) return 0;
        } else if (character < 0x20u) {
            static const char hex[] = "0123456789abcdef";
            if (!login_text_append_literal(text, "\\u00") ||
                !login_text_append_byte(text, hex[character >> 4]) ||
                !login_text_append_byte(text, hex[character & 15u])) return 0;
        } else if (!login_text_append_byte(text, (char)character)) return 0;
    }
    return login_text_append_byte(text, '"');
}

static McpeWireU64 identity_random_id(const McpeIdentity *identity)
{
    McpeWireU64 result;
    mcpe_u32 index;
    result.lo = 0;
    result.hi = 0;
    for (index = 0; index < 4u; ++index) {
        result.lo |= (mcpe_u32)identity->client_random_id[index] << (index * 8u);
        result.hi |= (mcpe_u32)identity->client_random_id[index + 4u] << (index * 8u);
    }
    return result;
}

static mcpe_u32 login_u64_divmod_10(McpeWireU64 *value)
{
    McpeWireU64 quotient;
    mcpe_u32 bit;
    mcpe_u32 remainder = 0;
    quotient.lo = 0;
    quotient.hi = 0;
    for (bit = 0; bit < 64u; ++bit) {
        mcpe_u32 input_bit;
        if (bit < 32u) input_bit = (value->hi >> (31u - bit)) & 1u;
        else input_bit = (value->lo >> (63u - bit)) & 1u;
        remainder = (remainder << 1) | input_bit;
        if (remainder >= 10u) {
            remainder -= 10u;
            if (bit < 32u) quotient.hi |= 1u << (31u - bit);
            else quotient.lo |= 1u << (63u - bit);
        }
    }
    *value = quotient;
    return remainder;
}

static int login_text_append_u64(McpeLoginText *text, McpeWireU64 value)
{
    char digits[20];
    mcpe_u32 count = 0;
    do {
        digits[count++] = (char)('0' + login_u64_divmod_10(&value));
    } while ((value.lo || value.hi) && count < sizeof(digits));
    while (count) {
        if (!login_text_append_byte(text, digits[--count])) return 0;
    }
    return 1;
}

static int login_text_append_uuid(McpeLoginText *text, const mcpe_u8 uuid[16])
{
    static const char hex[] = "0123456789abcdef";
    mcpe_u32 index;
    if (!text || !uuid || !login_text_append_byte(text, '"')) return 0;
    for (index = 0; index < 16u; ++index) {
        if ((index == 4u || index == 6u || index == 8u || index == 10u) &&
            !login_text_append_byte(text, '-')) return 0;
        if (!login_text_append_byte(text, hex[uuid[index] >> 4]) ||
            !login_text_append_byte(text, hex[uuid[index] & 15u])) return 0;
    }
    return login_text_append_byte(text, '"');
}

static int login_base64_encode(const mcpe_u8 *input, mcpe_u32 input_length,
                               McpeLoginText *output)
{
    static const char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    mcpe_u32 index = 0;
    if ((!input && input_length) || !output) return 0;
    while (index < input_length) {
        mcpe_u32 remaining = input_length - index;
        mcpe_u32 a = input[index++];
        mcpe_u32 b = remaining > 1u ? input[index++] : 0u;
        mcpe_u32 c = remaining > 2u ? input[index++] : 0u;
        if (!login_text_append_byte(output, alphabet[a >> 2]) ||
            !login_text_append_byte(output, alphabet[((a & 3u) << 4) | (b >> 4)]) ||
            !login_text_append_byte(output, remaining > 1u ? alphabet[((b & 15u) << 2) | (c >> 6)] : '=') ||
            !login_text_append_byte(output, remaining > 2u ? alphabet[c & 63u] : '=')) return 0;
    }
    return 1;
}

static int build_certificate_payload(McpeLoginText *output, const McpeIdentity *identity,
                                     const char *username)
{
    const char *display_name = (username && username[0]) ? username : "NuMC3DS";
    if (!login_text_append_literal(output, "{\"identityPublicKey\":") ||
        !login_text_append_json_string(output, identity->public_key_pem) ||
        !login_text_append_literal(output, ",\"certificateAuthority\":true,\"extraData\":{\"displayName\":") ||
        !login_text_append_json_string(output, display_name) ||
        !login_text_append_literal(output, ",\"identity\":")) return 0;
    if (!login_text_append_uuid(output, identity->uuid) ||
        !login_text_append_literal(output, ",\"XUID\":\"\"},\"nbf\":1,\"exp\":4102444800}")) return 0;
    return 1;
}

static int build_client_payload(McpeLoginText *output, const McpeIdentity *identity,
                                const McpeSkinWireAsset *skin)
{
    const char *server_address = mcpe_transport_selector_server_address();
    if (!server_address) server_address = "";
    if (!login_text_append_literal(output, "{\"ServerAddress\":") ||
        !login_text_append_json_string(output, server_address) ||
        !login_text_append_literal(output, ",\"ClientRandomId\":") ||
        !login_text_append_u64(output, identity_random_id(identity)) ||
        !login_text_append_literal(output, ",\"SkinId\":") ||
        !login_text_append_json_string(output, skin->skin_id) ||
        !login_text_append_literal(output, ",\"SkinData\":\"") ||
        !login_base64_encode(skin->bytes, skin->length, output) ||
        !login_text_append_literal(output, "\",\"TenantId\":\"\",\"ADRole\":0,\"GameVersion\":\"1.1.5\",\"DeviceModel\":\"Nintendo 3DS\",\"DeviceOS\":-1,\"DefaultInputMode\":2,\"CurrentInputMode\":2,\"UIProfile\":0,\"GuiScale\":0,\"LanguageCode\":\"en_US\"}")) return 0;
    return 1;
}

int mcpe_login_builder_build(NativeGstdString *payload, const char *username)
{
    McpeIdentity *identity = 0;
    McpeSkinWireAsset skin;
    McpeLoginText certificate_payload;
    McpeLoginText certificate_jwt;
    McpeLoginText certificate_chain;
    McpeLoginText client_payload;
    McpeLoginText client_jwt;
    McpeConnectionRequestView request;
    mcpe_u32 certificate_jwt_length;
    mcpe_u32 client_jwt_length;
    mcpe_u32 output_length;
    mcpe_u32 output_capacity;
    mcpe_u32 client_payload_capacity;
    mcpe_u32 client_jwt_capacity;
    int result = 0;
    zero(&skin, sizeof(skin));
    zero(&certificate_payload, sizeof(certificate_payload));
    zero(&certificate_jwt, sizeof(certificate_jwt));
    zero(&certificate_chain, sizeof(certificate_chain));
    zero(&client_payload, sizeof(client_payload));
    zero(&client_jwt, sizeof(client_jwt));
    zero(&request, sizeof(request));
    if (!payload || !s || !s->host.heap_alloc || !s->host.heap_free) return 0;
    zero(payload, sizeof(*payload));
    if (!mcpe_identity_get(&identity) || !skin_wire_build(&skin) ||
        !login_text_allocate(&certificate_payload, 1024u) ||
        !login_text_allocate(&certificate_jwt, 2048u) ||
        !login_text_allocate(&certificate_chain, 2080u)) goto cleanup;
    if (!build_certificate_payload(&certificate_payload, identity, username) ||
        !mcpe_jwt_es384(identity, certificate_payload.bytes, certificate_payload.length,
                         certificate_jwt.bytes, certificate_jwt.capacity, &certificate_jwt_length) ||
        !login_text_append_literal(&certificate_chain, "{\"chain\":[\"") ||
        !login_text_append_bytes(&certificate_chain, certificate_jwt.bytes, certificate_jwt_length) ||
        !login_text_append_literal(&certificate_chain, "\"]}")) goto cleanup;
    if (skin.length > 0x00010000u) goto cleanup;
    client_payload_capacity = 4096u + ((skin.length + 2u) / 3u) * 4u;
    client_jwt_capacity = 1024u + ((client_payload_capacity + 2u) / 3u) * 4u;
    if (!login_text_allocate(&client_payload, client_payload_capacity) ||
        !login_text_allocate(&client_jwt, client_jwt_capacity)) goto cleanup;
    if (!build_client_payload(&client_payload, identity, &skin) ||
        !mcpe_jwt_es384(identity, client_payload.bytes, client_payload.length,
                         client_jwt.bytes, client_jwt.capacity, &client_jwt_length)) goto cleanup;
    request.certificate = (const mcpe_u8 *)certificate_chain.bytes;
    request.certificate_length = certificate_chain.length;
    request.web_token = (const mcpe_u8 *)client_jwt.bytes;
    request.web_token_length = client_jwt_length;
    if (certificate_chain.length > 0x00fffff0u - client_jwt_length - 8u) goto cleanup;
    output_capacity = certificate_chain.length + client_jwt_length + 8u;
    if (!gstd_allocate(payload, output_capacity, output_capacity) ||
        mcpe_connection_request_encode(&request, (void *)payload->handle,
                                       output_capacity, &output_length) ||
        output_length != output_capacity) goto cleanup;
    result = 1;
    net_log_open(NET_LOG_INFO, "login_builder", "built_payload");
    net_log_dec("length", output_length);
    net_log_close();

cleanup:
    if (!result) {
        net_log_open(NET_LOG_INFO, "login_builder", "failed");
        net_log_close();
        gstd_destroy(payload);
    }
    login_text_destroy(&client_jwt);
    login_text_destroy(&client_payload);
    login_text_destroy(&certificate_chain);
    login_text_destroy(&certificate_jwt);
    login_text_destroy(&certificate_payload);
    skin_wire_release(&skin);
    return result;
}
