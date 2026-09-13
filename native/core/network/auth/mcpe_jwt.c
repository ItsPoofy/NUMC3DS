#include "mcpe_jwt.h"

static int base64url_encode(const mcpe_u8 *input, mcpe_u32 input_length,
                            char *output, mcpe_u32 output_capacity, mcpe_u32 *output_length)
{
    static const char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
    mcpe_u32 in = 0;
    mcpe_u32 out = 0;
    if (!input || !output || !output_length) return 0;
    while (in < input_length) {
        mcpe_u32 left = input_length - in;
        mcpe_u32 a = input[in++];
        mcpe_u32 b = left > 1u ? input[in++] : 0u;
        mcpe_u32 c = left > 2u ? input[in++] : 0u;
        if (out + 4u > output_capacity) return 0;
        output[out++] = alphabet[a >> 2];
        output[out++] = alphabet[((a & 3u) << 4) | (b >> 4)];
        if (left > 1u) output[out++] = alphabet[((b & 15u) << 2) | (c >> 6)];
        if (left > 2u) output[out++] = alphabet[c & 63u];
    }
    *output_length = out;
    return 1;
}

int mcpe_jwt_es384(const McpeIdentity *identity, const char *payload,
                   mcpe_u32 payload_length, char *output, mcpe_u32 capacity,
                   mcpe_u32 *output_length)
{
    static const char prefix[] = "{\"alg\":\"ES384\",\"x5u\":\"";
    static const char suffix[] = "\"}";
    char header[256];
    mcpe_u8 digest[MCPE_P384_DIGEST_BYTES];
    mcpe_u8 signature[MCPE_P384_SIGNATURE_BYTES];
    mcpe_u32 header_length;
    mcpe_u32 header_raw_length = 0;
    mcpe_u32 encoded_payload_length;
    mcpe_u32 encoded_signature_length;
    mcpe_u32 message_length;
    mcpe_u32 index;
    if (!identity || !payload || !output || !output_length) return 0;
    if (identity->public_key_pem_length > sizeof(header) - sizeof(prefix) - sizeof(suffix)) return 0;
    for (index = 0; prefix[index]; ++index) header[header_raw_length++] = prefix[index];
    for (index = 0; index < identity->public_key_pem_length; ++index) header[header_raw_length++] = identity->public_key_pem[index];
    for (index = 0; suffix[index]; ++index) header[header_raw_length++] = suffix[index];
    if (!base64url_encode((const mcpe_u8 *)header, header_raw_length,
                          output, capacity, &header_length)) return 0;
    if (header_length >= capacity) return 0;
    output[header_length++] = '.';
    if (!base64url_encode((const mcpe_u8 *)payload, payload_length,
                          output + header_length, capacity - header_length,
                          &encoded_payload_length)) return 0;
    message_length = header_length + encoded_payload_length;
    if (message_length >= capacity ||
        !mcpe_p384_sha384(output, message_length, digest) ||
        !mcpe_p384_sign_sha384(identity->private_key, digest, signature)) return 0;
    output[message_length++] = '.';
    if (!base64url_encode(signature, sizeof(signature), output + message_length,
                          capacity - message_length, &encoded_signature_length)) return 0;
    message_length += encoded_signature_length;
    if (message_length >= capacity) return 0;
    output[message_length] = 0;
    *output_length = message_length;
    return 1;
}
