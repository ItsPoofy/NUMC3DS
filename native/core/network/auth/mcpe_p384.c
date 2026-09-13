#include "mcpe_p384.h"
#include "bearssl/inc/bearssl.h"
#include "../platform/ssl_entropy.h"

static int p384_private_valid(const mcpe_u8 private_key[MCPE_P384_PRIVATE_BYTES])
{
    static const mcpe_u8 order[MCPE_P384_PRIVATE_BYTES] = {
        0xffu, 0xffu, 0xffu, 0xffu, 0xffu, 0xffu, 0xffu, 0xffu,
        0xffu, 0xffu, 0xffu, 0xffu, 0xffu, 0xffu, 0xffu, 0xffu,
        0xffu, 0xffu, 0xffu, 0xffu, 0xffu, 0xffu, 0xffu, 0xffu,
        0xc7u, 0x63u, 0x4du, 0x81u, 0xf4u, 0x37u, 0x2du, 0xdfu,
        0x58u, 0x1au, 0x0du, 0xb2u, 0x48u, 0xb0u, 0xa7u, 0x7au,
        0xecu, 0xecu, 0x19u, 0x6au, 0xccu, 0xc5u, 0x29u, 0x73u
    };
    mcpe_u32 index;
    int nonzero = 0;
    for (index = 0; index < MCPE_P384_PRIVATE_BYTES; ++index) {
        if (private_key[index]) nonzero = 1;
    }
    if (!nonzero) return 0;
    for (index = 0; index < MCPE_P384_PRIVATE_BYTES; ++index) {
        if (private_key[index] != order[index]) return private_key[index] < order[index];
    }
    return 0;
}

int mcpe_p384_generate_private(mcpe_u8 private_key[MCPE_P384_PRIVATE_BYTES])
{
#if defined(NUMC3DS_MCPE_TARGET)
    mcpe_u32 attempts;
    if (!private_key) return 0;
    for (attempts = 0; attempts < 32u; ++attempts) {
        if (!mcpe_ssl_entropy_fill(private_key, MCPE_P384_PRIVATE_BYTES)) return 0;
        if (p384_private_valid(private_key)) return 1;
    }
#else
    (void)private_key;
#endif
    return 0;
}

int mcpe_p384_public_from_private(const mcpe_u8 private_key[MCPE_P384_PRIVATE_BYTES],
                                  mcpe_u8 public_key[MCPE_P384_PUBLIC_BYTES])
{
    if (!private_key || !public_key || !p384_private_valid(private_key)) return 0;
    return br_ec_prime_i31.mulgen(public_key, private_key, MCPE_P384_PRIVATE_BYTES,
                                  BR_EC_secp384r1) == MCPE_P384_PUBLIC_BYTES;
}

int mcpe_p384_shared_secret(const mcpe_u8 private_key[MCPE_P384_PRIVATE_BYTES],
                            const mcpe_u8 peer_public_key[MCPE_P384_PUBLIC_BYTES],
                            mcpe_u8 shared_secret[MCPE_P384_SHARED_SECRET_BYTES])
{
    mcpe_u8 point[MCPE_P384_PUBLIC_BYTES];
    mcpe_u32 index;
    if (!private_key || !peer_public_key || !shared_secret || !p384_private_valid(private_key)) return 0;
    for (index = 0; index < MCPE_P384_PUBLIC_BYTES; ++index) point[index] = peer_public_key[index];
    if (!br_ec_prime_i31.mul(point, MCPE_P384_PUBLIC_BYTES, private_key,
                             MCPE_P384_PRIVATE_BYTES, BR_EC_secp384r1)) return 0;
    for (index = 0; index < MCPE_P384_SHARED_SECRET_BYTES; ++index) shared_secret[index] = point[index + 1u];
    return 1;
}

static int p384_base64_value(char value)
{
    if (value >= 'A' && value <= 'Z') return value - 'A';
    if (value >= 'a' && value <= 'z') return value - 'a' + 26;
    if (value >= '0' && value <= '9') return value - '0' + 52;
    if (value == '+') return 62;
    if (value == '/') return 63;
    return -1;
}

static int p384_match(const char *text, mcpe_u32 length, mcpe_u32 *offset,
                      const char *expected)
{
    mcpe_u32 index = 0;
    if (!text || !offset || !expected || *offset > length) return 0;
    while (expected[index]) {
        if (*offset + index >= length || text[*offset + index] != expected[index]) return 0;
        ++index;
    }
    *offset += index;
    return 1;
}

int mcpe_p384_public_from_pem(const char *pem, mcpe_u32 pem_length,
                              mcpe_u8 public_key[MCPE_P384_PUBLIC_BYTES])
{
    static const char begin[] = "-----BEGIN PUBLIC KEY-----";
    static const char end[] = "-----END PUBLIC KEY-----";
    static const mcpe_u8 spki_prefix[] = {
        0x30u, 0x76u, 0x30u, 0x10u, 0x06u, 0x07u, 0x2au, 0x86u, 0x48u,
        0xceu, 0x3du, 0x02u, 0x01u, 0x06u, 0x05u, 0x2bu, 0x81u, 0x04u,
        0x00u, 0x22u, 0x03u, 0x62u, 0x00u
    };
    mcpe_u8 der[120];
    mcpe_u32 offset = 0;
    mcpe_u32 output = 0;
    mcpe_u32 quartet = 0;
    mcpe_u32 values = 0;
    mcpe_u32 index;
    int has_pem_armor = 0;
    int ended = 0;
    if (!pem || !pem_length || !public_key) return 0;
    while (offset < pem_length && (pem[offset] == ' ' || pem[offset] == '\r' || pem[offset] == '\n' || pem[offset] == '\t')) ++offset;
    if (p384_match(pem, pem_length, &offset, begin)) {
        has_pem_armor = 1;
        while (offset < pem_length && (pem[offset] == '\r' || pem[offset] == '\n')) ++offset;
    }
    while (offset < pem_length) {
        char value = pem[offset++];
        int decoded;
        if (value == '\r' || value == '\n' || value == ' ' || value == '\t') continue;
        if (has_pem_armor && value == '-') {
            --offset;
            break;
        }
        if (ended) return 0;
        if (value == '=') {
            if (values < 2u) return 0;
            ended = 1;
            quartet <<= 6;
            ++values;
        } else {
            decoded = p384_base64_value(value);
            if (decoded < 0) return 0;
            quartet = (quartet << 6) | (mcpe_u32)decoded;
            ++values;
        }
        if (values == 4u) {
            if (output >= sizeof(der)) return 0;
            der[output++] = (mcpe_u8)(quartet >> 16);
            if (!ended) {
                if (output >= sizeof(der)) return 0;
                der[output++] = (mcpe_u8)(quartet >> 8);
                if (value != '=') {
                    if (output >= sizeof(der)) return 0;
                    der[output++] = (mcpe_u8)quartet;
                }
            }
            quartet = 0;
            values = 0;
        }
    }
    if (values) return 0;
    if (has_pem_armor && !p384_match(pem, pem_length, &offset, end)) return 0;
    if (output != sizeof(der)) return 0;
    while (offset < pem_length && (pem[offset] == ' ' || pem[offset] == '\r' || pem[offset] == '\n' || pem[offset] == '\t')) ++offset;
    if (offset != pem_length) return 0;
    for (index = 0; index < sizeof(spki_prefix); ++index) {
        if (der[index] != spki_prefix[index]) return 0;
    }
    if (der[sizeof(spki_prefix)] != 0x04u) return 0;
    for (index = 0; index < MCPE_P384_PUBLIC_BYTES; ++index) {
        public_key[index] = der[sizeof(spki_prefix) + index];
    }
    return 1;
}

int mcpe_p384_sign_sha384(const mcpe_u8 private_key[MCPE_P384_PRIVATE_BYTES],
                          const mcpe_u8 digest[MCPE_P384_DIGEST_BYTES],
                          mcpe_u8 signature[MCPE_P384_SIGNATURE_BYTES])
{
    br_ec_private_key key;
    if (!private_key || !digest || !signature) return 0;
    key.curve = BR_EC_secp384r1;
    key.x = (unsigned char *)private_key;
    key.xlen = MCPE_P384_PRIVATE_BYTES;
    return br_ecdsa_i31_sign_raw(&br_ec_prime_i31, &br_sha384_vtable, digest,
                                 &key, signature) == MCPE_P384_SIGNATURE_BYTES;
}

int mcpe_p384_verify_sha384(const mcpe_u8 public_key[MCPE_P384_PUBLIC_BYTES],
                            const mcpe_u8 digest[MCPE_P384_DIGEST_BYTES],
                            const mcpe_u8 signature[MCPE_P384_SIGNATURE_BYTES])
{
    br_ec_public_key key;
    if (!public_key || !digest || !signature) return 0;
    key.curve = BR_EC_secp384r1;
    key.q = (unsigned char *)public_key;
    key.qlen = MCPE_P384_PUBLIC_BYTES;
    return br_ecdsa_i31_vrfy_raw(&br_ec_prime_i31, digest, MCPE_P384_DIGEST_BYTES,
                                 &key, signature, MCPE_P384_SIGNATURE_BYTES) != 0;
}

int mcpe_p384_sha384(const void *input, mcpe_u32 input_length,
                     mcpe_u8 digest[MCPE_P384_DIGEST_BYTES])
{
    br_sha384_context context;
    if ((!input && input_length) || !digest) return 0;
    br_sha384_init(&context);
    br_sha384_update(&context, input, input_length);
    br_sha384_out(&context, digest);
    return 1;
}
