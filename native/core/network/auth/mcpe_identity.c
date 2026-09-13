#include "mcpe_identity.h"
#include "../platform/ssl_entropy.h"

static McpeIdentity identity;
static int identity_ready;

static int base64_encode(const mcpe_u8 *input, mcpe_u32 input_length,
                         char *output, mcpe_u32 output_capacity, mcpe_u32 *output_length)
{
    static const char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    mcpe_u32 in = 0;
    mcpe_u32 out = 0;
    if (!input || !output || !output_length) return 0;
    while (in < input_length) {
        mcpe_u32 left = input_length - in;
        mcpe_u32 a = input[in++];
        mcpe_u32 b = left > 1u ? input[in++] : 0u;
        mcpe_u32 c = left > 2u ? input[in++] : 0u;
        if (out + 4u >= output_capacity) return 0;
        output[out++] = alphabet[a >> 2];
        output[out++] = alphabet[((a & 3u) << 4) | (b >> 4)];
        output[out++] = left > 1u ? alphabet[((b & 15u) << 2) | (c >> 6)] : '=';
        output[out++] = left > 2u ? alphabet[c & 63u] : '=';
    }
    if (out >= output_capacity) return 0;
    output[out] = 0;
    *output_length = out;
    return 1;
}

static int build_public_key_pem(McpeIdentity *value)
{
    static const mcpe_u8 spki_prefix[] = {
        0x30u, 0x76u, 0x30u, 0x10u, 0x06u, 0x07u, 0x2au, 0x86u, 0x48u,
        0xceu, 0x3du, 0x02u, 0x01u, 0x06u, 0x05u, 0x2bu, 0x81u, 0x04u,
        0x00u, 0x22u, 0x03u, 0x62u, 0x00u
    };
    mcpe_u8 der[120];
    mcpe_u32 encoded_length;
    mcpe_u32 index;
    if (!value) return 0;
    for (index = 0; index < sizeof(spki_prefix); ++index) der[index] = spki_prefix[index];
    for (index = 0; index < sizeof(value->public_key); ++index) {
        der[sizeof(spki_prefix) + index] = value->public_key[index];
    }
    if (!base64_encode(der, sizeof(der), value->public_key_pem,
                       sizeof(value->public_key_pem), &encoded_length)) return 0;
    value->public_key_pem_length = encoded_length;
    value->public_key_pem[encoded_length] = 0;
    return 1;
}

int mcpe_identity_random_bytes(mcpe_u8 *output, mcpe_u32 length)
{
#if defined(NUMC3DS_MCPE_TARGET)
    if (!output || !length) return 0;
    return mcpe_ssl_entropy_fill(output, length);
#else
    (void)output;
    (void)length;
    return 0;
#endif
}

int mcpe_identity_get(McpeIdentity **output)
{
    if (!output) return 0;
    if (!identity_ready) {
        if (!mcpe_p384_generate_private(identity.private_key) ||
            !mcpe_p384_public_from_private(identity.private_key, identity.public_key) ||
            !mcpe_identity_random_bytes(identity.uuid, sizeof(identity.uuid)) ||
            !mcpe_identity_random_bytes(identity.client_random_id, sizeof(identity.client_random_id)) ||
            !build_public_key_pem(&identity)) return 0;
        identity.uuid[6] = (mcpe_u8)((identity.uuid[6] & 0x0fu) | 0x40u);
        identity.uuid[8] = (mcpe_u8)((identity.uuid[8] & 0x3fu) | 0x80u);
        identity_ready = 1;
    }
    *output = &identity;
    return 1;
}
