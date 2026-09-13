#include "mcpe_handshake_crypto.h"

int mcpe_handshake_derive_key(const McpeIdentity *identity,
                              const char *server_public_key_pem,
                              mcpe_u32 server_public_key_pem_length,
                              const mcpe_u8 salt[16],
                              mcpe_u8 key[MCPE_CRYPTO_AES256_KEY_BYTES])
{
    mcpe_u8 public_key[MCPE_P384_PUBLIC_BYTES];
    mcpe_u8 shared_secret[MCPE_P384_SHARED_SECRET_BYTES];
    McpeSha256Context hash;
    if (!identity || !server_public_key_pem || !server_public_key_pem_length || !salt || !key ||
        !mcpe_p384_public_from_pem(server_public_key_pem, server_public_key_pem_length, public_key) ||
        !mcpe_p384_shared_secret(identity->private_key, public_key, shared_secret)) return 0;
    mcpe_sha256_init(&hash);
    mcpe_sha256_update(&hash, salt, 16u);
    mcpe_sha256_update(&hash, shared_secret, sizeof(shared_secret));
    mcpe_sha256_final(&hash, key);
    return 1;
}
