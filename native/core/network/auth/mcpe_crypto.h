#ifndef NUMC3DS_MCPE_CRYPTO_H
#define NUMC3DS_MCPE_CRYPTO_H

#include "../protocol/mcpe_types.h"

#define MCPE_CRYPTO_SHA256_BYTES 32u
#define MCPE_CRYPTO_AES256_KEY_BYTES 32u
#define MCPE_CRYPTO_AES_BLOCK_BYTES 16u
#define MCPE_CRYPTO_CHECKSUM_BYTES 8u

enum McpeCryptoResult {
    MCPE_CRYPTO_OK = 0,
    MCPE_CRYPTO_INVALID_ARGUMENT = -1,
    MCPE_CRYPTO_NOT_INITIALIZED = -2
};

typedef struct {
    mcpe_u32 state[8];
    mcpe_u64 bit_length;
    mcpe_u8 block[64];
    mcpe_u32 block_length;
} McpeSha256Context;

typedef struct {
    mcpe_u8 round_keys[240];
    mcpe_u8 feedback[MCPE_CRYPTO_AES_BLOCK_BYTES];
    mcpe_u8 initialized;
} McpeAes256Cfb8Context;

void mcpe_sha256_init(McpeSha256Context *context);
void mcpe_sha256_update(McpeSha256Context *context, const void *data, mcpe_u32 length);
void mcpe_sha256_final(McpeSha256Context *context, mcpe_u8 output[MCPE_CRYPTO_SHA256_BYTES]);

int mcpe_auth_checksum(const McpeWireU64 *counter,
                       const void *packet, mcpe_u32 packet_length,
                       const void *secret, mcpe_u32 secret_length,
                       mcpe_u8 output[MCPE_CRYPTO_CHECKSUM_BYTES]);

int mcpe_aes256_cfb8_init(McpeAes256Cfb8Context *context,
                          const mcpe_u8 key[MCPE_CRYPTO_AES256_KEY_BYTES],
                          const mcpe_u8 iv[MCPE_CRYPTO_AES_BLOCK_BYTES]);
int mcpe_aes256_cfb8_encrypt(McpeAes256Cfb8Context *context,
                             const void *input, void *output, mcpe_u32 length);
int mcpe_aes256_cfb8_decrypt(McpeAes256Cfb8Context *context,
                             const void *input, void *output, mcpe_u32 length);

#endif
