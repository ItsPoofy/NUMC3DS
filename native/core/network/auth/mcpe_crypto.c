#include "mcpe_crypto.h"

static const mcpe_u8 mcpe_aes_sbox[256] = {
    0x63,0x7c,0x77,0x7b,0xf2,0x6b,0x6f,0xc5,0x30,0x01,0x67,0x2b,0xfe,0xd7,0xab,0x76,
    0xca,0x82,0xc9,0x7d,0xfa,0x59,0x47,0xf0,0xad,0xd4,0xa2,0xaf,0x9c,0xa4,0x72,0xc0,
    0xb7,0xfd,0x93,0x26,0x36,0x3f,0xf7,0xcc,0x34,0xa5,0xe5,0xf1,0x71,0xd8,0x31,0x15,
    0x04,0xc7,0x23,0xc3,0x18,0x96,0x05,0x9a,0x07,0x12,0x80,0xe2,0xeb,0x27,0xb2,0x75,
    0x09,0x83,0x2c,0x1a,0x1b,0x6e,0x5a,0xa0,0x52,0x3b,0xd6,0xb3,0x29,0xe3,0x2f,0x84,
    0x53,0xd1,0x00,0xed,0x20,0xfc,0xb1,0x5b,0x6a,0xcb,0xbe,0x39,0x4a,0x4c,0x58,0xcf,
    0xd0,0xef,0xaa,0xfb,0x43,0x4d,0x33,0x85,0x45,0xf9,0x02,0x7f,0x50,0x3c,0x9f,0xa8,
    0x51,0xa3,0x40,0x8f,0x92,0x9d,0x38,0xf5,0xbc,0xb6,0xda,0x21,0x10,0xff,0xf3,0xd2,
    0xcd,0x0c,0x13,0xec,0x5f,0x97,0x44,0x17,0xc4,0xa7,0x7e,0x3d,0x64,0x5d,0x19,0x73,
    0x60,0x81,0x4f,0xdc,0x22,0x2a,0x90,0x88,0x46,0xee,0xb8,0x14,0xde,0x5e,0x0b,0xdb,
    0xe0,0x32,0x3a,0x0a,0x49,0x06,0x24,0x5c,0xc2,0xd3,0xac,0x62,0x91,0x95,0xe4,0x79,
    0xe7,0xc8,0x37,0x6d,0x8d,0xd5,0x4e,0xa9,0x6c,0x56,0xf4,0xea,0x65,0x7a,0xae,0x08,
    0xba,0x78,0x25,0x2e,0x1c,0xa6,0xb4,0xc6,0xe8,0xdd,0x74,0x1f,0x4b,0xbd,0x8b,0x8a,
    0x70,0x3e,0xb5,0x66,0x48,0x03,0xf6,0x0e,0x61,0x35,0x57,0xb9,0x86,0xc1,0x1d,0x9e,
    0xe1,0xf8,0x98,0x11,0x69,0xd9,0x8e,0x94,0x9b,0x1e,0x87,0xe9,0xce,0x55,0x28,0xdf,
    0x8c,0xa1,0x89,0x0d,0xbf,0xe6,0x42,0x68,0x41,0x99,0x2d,0x0f,0xb0,0x54,0xbb,0x16
};

static mcpe_u32 mcpe_rotr32(mcpe_u32 value, mcpe_u32 count)
{
    return (value >> count) | (value << (32u - count));
}

static mcpe_u32 mcpe_sha256_be32(const mcpe_u8 *bytes)
{
    return ((mcpe_u32)bytes[0] << 24) | ((mcpe_u32)bytes[1] << 16) |
           ((mcpe_u32)bytes[2] << 8) | (mcpe_u32)bytes[3];
}

static void mcpe_sha256_store_be32(mcpe_u8 *bytes, mcpe_u32 value)
{
    bytes[0] = (mcpe_u8)(value >> 24);
    bytes[1] = (mcpe_u8)(value >> 16);
    bytes[2] = (mcpe_u8)(value >> 8);
    bytes[3] = (mcpe_u8)value;
}

static void mcpe_sha256_transform(McpeSha256Context *context, const mcpe_u8 *block)
{
    static const mcpe_u32 constants[64] = {
        0x428a2f98u,0x71374491u,0xb5c0fbcfu,0xe9b5dba5u,0x3956c25bu,0x59f111f1u,0x923f82a4u,0xab1c5ed5u,
        0xd807aa98u,0x12835b01u,0x243185beu,0x550c7dc3u,0x72be5d74u,0x80deb1feu,0x9bdc06a7u,0xc19bf174u,
        0xe49b69c1u,0xefbe4786u,0x0fc19dc6u,0x240ca1ccu,0x2de92c6fu,0x4a7484aau,0x5cb0a9dcu,0x76f988dau,
        0x983e5152u,0xa831c66du,0xb00327c8u,0xbf597fc7u,0xc6e00bf3u,0xd5a79147u,0x06ca6351u,0x14292967u,
        0x27b70a85u,0x2e1b2138u,0x4d2c6dfcu,0x53380d13u,0x650a7354u,0x766a0abbu,0x81c2c92eu,0x92722c85u,
        0xa2bfe8a1u,0xa81a664bu,0xc24b8b70u,0xc76c51a3u,0xd192e819u,0xd6990624u,0xf40e3585u,0x106aa070u,
        0x19a4c116u,0x1e376c08u,0x2748774cu,0x34b0bcb5u,0x391c0cb3u,0x4ed8aa4au,0x5b9cca4fu,0x682e6ff3u,
        0x748f82eeu,0x78a5636fu,0x84c87814u,0x8cc70208u,0x90befffau,0xa4506cebu,0xbef9a3f7u,0xc67178f2u
    };
    mcpe_u32 schedule[64];
    mcpe_u32 a, b, c, d, e, f, g, h;
    mcpe_u32 i;
    for (i = 0; i < 16; ++i) schedule[i] = mcpe_sha256_be32(block + i * 4u);
    for (i = 16; i < 64; ++i) {
        mcpe_u32 x = schedule[i - 15u];
        mcpe_u32 y = schedule[i - 2u];
        mcpe_u32 small0 = mcpe_rotr32(x, 7u) ^ mcpe_rotr32(x, 18u) ^ (x >> 3u);
        mcpe_u32 small1 = mcpe_rotr32(y, 17u) ^ mcpe_rotr32(y, 19u) ^ (y >> 10u);
        schedule[i] = schedule[i - 16u] + small0 + schedule[i - 7u] + small1;
    }
    a = context->state[0];
    b = context->state[1];
    c = context->state[2];
    d = context->state[3];
    e = context->state[4];
    f = context->state[5];
    g = context->state[6];
    h = context->state[7];
    for (i = 0; i < 64; ++i) {
        mcpe_u32 big1 = mcpe_rotr32(e, 6u) ^ mcpe_rotr32(e, 11u) ^ mcpe_rotr32(e, 25u);
        mcpe_u32 choose = (e & f) ^ ((~e) & g);
        mcpe_u32 temp1 = h + big1 + choose + constants[i] + schedule[i];
        mcpe_u32 big0 = mcpe_rotr32(a, 2u) ^ mcpe_rotr32(a, 13u) ^ mcpe_rotr32(a, 22u);
        mcpe_u32 majority = (a & b) ^ (a & c) ^ (b & c);
        mcpe_u32 temp2 = big0 + majority;
        h = g;
        g = f;
        f = e;
        e = d + temp1;
        d = c;
        c = b;
        b = a;
        a = temp1 + temp2;
    }
    context->state[0] += a;
    context->state[1] += b;
    context->state[2] += c;
    context->state[3] += d;
    context->state[4] += e;
    context->state[5] += f;
    context->state[6] += g;
    context->state[7] += h;
}

void mcpe_sha256_init(McpeSha256Context *context)
{
    if (!context) return;
    context->state[0] = 0x6a09e667u;
    context->state[1] = 0xbb67ae85u;
    context->state[2] = 0x3c6ef372u;
    context->state[3] = 0xa54ff53au;
    context->state[4] = 0x510e527fu;
    context->state[5] = 0x9b05688cu;
    context->state[6] = 0x1f83d9abu;
    context->state[7] = 0x5be0cd19u;
    context->bit_length = 0;
    context->block_length = 0;
}

void mcpe_sha256_update(McpeSha256Context *context, const void *data, mcpe_u32 length)
{
    const mcpe_u8 *bytes = (const mcpe_u8 *)data;
    mcpe_u32 copy;
    if (!context || (!data && length)) return;
    context->bit_length += (mcpe_u64)length * 8ull;
    while (length) {
        copy = 64u - context->block_length;
        if (copy > length) copy = length;
        {
            mcpe_u32 i;
            for (i = 0; i < copy; ++i) context->block[context->block_length + i] = bytes[i];
        }
        bytes += copy;
        length -= copy;
        context->block_length += copy;
        if (context->block_length == 64u) {
            mcpe_sha256_transform(context, context->block);
            context->block_length = 0;
        }
    }
}

void mcpe_sha256_final(McpeSha256Context *context, mcpe_u8 output[MCPE_CRYPTO_SHA256_BYTES])
{
    mcpe_u32 i;
    if (!context || !output) return;
    context->block[context->block_length++] = 0x80;
    if (context->block_length > 56u) {
        while (context->block_length < 64u) context->block[context->block_length++] = 0;
        mcpe_sha256_transform(context, context->block);
        context->block_length = 0;
    }
    while (context->block_length < 56u) context->block[context->block_length++] = 0;
    for (i = 0; i < 8; ++i) context->block[56u + i] = (mcpe_u8)(context->bit_length >> (56u - i * 8u));
    mcpe_sha256_transform(context, context->block);
    for (i = 0; i < 8; ++i) mcpe_sha256_store_be32(output + i * 4u, context->state[i]);
}

int mcpe_auth_checksum(const McpeWireU64 *counter,
                       const void *packet, mcpe_u32 packet_length,
                       const void *secret, mcpe_u32 secret_length,
                       mcpe_u8 output[MCPE_CRYPTO_CHECKSUM_BYTES])
{
    McpeSha256Context context;
    mcpe_u8 counter_bytes[8];
    mcpe_u32 i;
    if (!counter || (!packet && packet_length) || (!secret && secret_length) || !output) {
        return MCPE_CRYPTO_INVALID_ARGUMENT;
    }
    for (i = 0; i < 4; ++i) counter_bytes[i] = (mcpe_u8)(counter->lo >> (i * 8u));
    for (i = 0; i < 4; ++i) counter_bytes[4u + i] = (mcpe_u8)(counter->hi >> (i * 8u));
    mcpe_sha256_init(&context);
    mcpe_sha256_update(&context, counter_bytes, sizeof(counter_bytes));
    mcpe_sha256_update(&context, packet, packet_length);
    mcpe_sha256_update(&context, secret, secret_length);
    {
        mcpe_u8 digest[MCPE_CRYPTO_SHA256_BYTES];
        mcpe_sha256_final(&context, digest);
        for (i = 0; i < MCPE_CRYPTO_CHECKSUM_BYTES; ++i) output[i] = digest[i];
    }
    return MCPE_CRYPTO_OK;
}

static mcpe_u8 mcpe_aes_xtime(mcpe_u8 value)
{
    return (mcpe_u8)((value << 1) ^ ((value & 0x80u) ? 0x1bu : 0));
}

static void mcpe_aes_key_expand(McpeAes256Cfb8Context *context, const mcpe_u8 *key)
{
    mcpe_u32 generated = 32u;
    mcpe_u8 rcon = 1u;
    mcpe_u8 temp[4];
    mcpe_u32 i;
    for (i = 0; i < 32u; ++i) context->round_keys[i] = key[i];
    while (generated < sizeof(context->round_keys)) {
        for (i = 0; i < 4u; ++i) temp[i] = context->round_keys[generated - 4u + i];
        if ((generated % 32u) == 0u) {
            mcpe_u8 first = temp[0];
            temp[0] = mcpe_aes_sbox[temp[1]] ^ rcon;
            temp[1] = mcpe_aes_sbox[temp[2]];
            temp[2] = mcpe_aes_sbox[temp[3]];
            temp[3] = mcpe_aes_sbox[first];
            rcon = mcpe_aes_xtime(rcon);
        } else if ((generated % 32u) == 16u) {
            for (i = 0; i < 4u; ++i) temp[i] = mcpe_aes_sbox[temp[i]];
        }
        for (i = 0; i < 4u; ++i) {
            context->round_keys[generated] = context->round_keys[generated - 32u] ^ temp[i];
            ++generated;
        }
    }
}

static void mcpe_aes_add_round_key(mcpe_u8 state[16], const mcpe_u8 *round_key)
{
    mcpe_u32 i;
    for (i = 0; i < 16u; ++i) state[i] ^= round_key[i];
}

static void mcpe_aes_sub_bytes(mcpe_u8 state[16])
{
    mcpe_u32 i;
    for (i = 0; i < 16u; ++i) state[i] = mcpe_aes_sbox[state[i]];
}

static void mcpe_aes_shift_rows(mcpe_u8 state[16])
{
    mcpe_u8 shifted[16];
    shifted[0] = state[0];
    shifted[1] = state[5];
    shifted[2] = state[10];
    shifted[3] = state[15];
    shifted[4] = state[4];
    shifted[5] = state[9];
    shifted[6] = state[14];
    shifted[7] = state[3];
    shifted[8] = state[8];
    shifted[9] = state[13];
    shifted[10] = state[2];
    shifted[11] = state[7];
    shifted[12] = state[12];
    shifted[13] = state[1];
    shifted[14] = state[6];
    shifted[15] = state[11];
    {
        mcpe_u32 i;
        for (i = 0; i < 16u; ++i) state[i] = shifted[i];
    }
}

static void mcpe_aes_mix_columns(mcpe_u8 state[16])
{
    mcpe_u32 column;
    for (column = 0; column < 4u; ++column) {
        mcpe_u8 *value = state + column * 4u;
        mcpe_u8 a = value[0];
        mcpe_u8 b = value[1];
        mcpe_u8 c = value[2];
        mcpe_u8 d = value[3];
        mcpe_u8 total = (mcpe_u8)(a ^ b ^ c ^ d);
        value[0] = (mcpe_u8)(a ^ total ^ mcpe_aes_xtime((mcpe_u8)(a ^ b)));
        value[1] = (mcpe_u8)(b ^ total ^ mcpe_aes_xtime((mcpe_u8)(b ^ c)));
        value[2] = (mcpe_u8)(c ^ total ^ mcpe_aes_xtime((mcpe_u8)(c ^ d)));
        value[3] = (mcpe_u8)(d ^ total ^ mcpe_aes_xtime((mcpe_u8)(d ^ a)));
    }
}

static void mcpe_aes_encrypt_block(const McpeAes256Cfb8Context *context,
                                   const mcpe_u8 input[16], mcpe_u8 output[16])
{
    mcpe_u8 state[16];
    mcpe_u32 round;
    mcpe_u32 i;
    for (i = 0; i < 16u; ++i) state[i] = input[i];
    mcpe_aes_add_round_key(state, context->round_keys);
    for (round = 1u; round < 14u; ++round) {
        mcpe_aes_sub_bytes(state);
        mcpe_aes_shift_rows(state);
        mcpe_aes_mix_columns(state);
        mcpe_aes_add_round_key(state, context->round_keys + round * 16u);
    }
    mcpe_aes_sub_bytes(state);
    mcpe_aes_shift_rows(state);
    mcpe_aes_add_round_key(state, context->round_keys + 14u * 16u);
    for (i = 0; i < 16u; ++i) output[i] = state[i];
}

int mcpe_aes256_cfb8_init(McpeAes256Cfb8Context *context,
                          const mcpe_u8 key[MCPE_CRYPTO_AES256_KEY_BYTES],
                          const mcpe_u8 iv[MCPE_CRYPTO_AES_BLOCK_BYTES])
{
    mcpe_u32 i;
    if (!context || !key || !iv) return MCPE_CRYPTO_INVALID_ARGUMENT;
    mcpe_aes_key_expand(context, key);
    for (i = 0; i < MCPE_CRYPTO_AES_BLOCK_BYTES; ++i) context->feedback[i] = iv[i];
    context->initialized = 1;
    return MCPE_CRYPTO_OK;
}

static int mcpe_aes256_cfb8_crypt(McpeAes256Cfb8Context *context,
                                  const void *input, void *output,
                                  mcpe_u32 length, int decrypt)
{
    const mcpe_u8 *source = (const mcpe_u8 *)input;
    mcpe_u8 *destination = (mcpe_u8 *)output;
    mcpe_u32 index;
    if (!context || !context->initialized) return MCPE_CRYPTO_NOT_INITIALIZED;
    if ((!input || !output) && length) return MCPE_CRYPTO_INVALID_ARGUMENT;
    for (index = 0; index < length; ++index) {
        mcpe_u8 stream[16];
        mcpe_u8 input_byte = source[index];
        mcpe_u8 output_byte;
        mcpe_u32 shift;
        mcpe_aes_encrypt_block(context, context->feedback, stream);
        output_byte = (mcpe_u8)(input_byte ^ stream[0]);
        destination[index] = output_byte;
        for (shift = 0; shift + 1u < MCPE_CRYPTO_AES_BLOCK_BYTES; ++shift) {
            context->feedback[shift] = context->feedback[shift + 1u];
        }
        context->feedback[MCPE_CRYPTO_AES_BLOCK_BYTES - 1u] = decrypt ? input_byte : output_byte;
    }
    return MCPE_CRYPTO_OK;
}

int mcpe_aes256_cfb8_encrypt(McpeAes256Cfb8Context *context,
                             const void *input, void *output, mcpe_u32 length)
{
    return mcpe_aes256_cfb8_crypt(context, input, output, length, 0);
}

int mcpe_aes256_cfb8_decrypt(McpeAes256Cfb8Context *context,
                             const void *input, void *output, mcpe_u32 length)
{
    return mcpe_aes256_cfb8_crypt(context, input, output, length, 1);
}
