#ifndef NUMC3DS_MCPE_HANDSHAKE_CRYPTO_H
#define NUMC3DS_MCPE_HANDSHAKE_CRYPTO_H

#include "mcpe_identity.h"
#include "mcpe_crypto.h"

int mcpe_handshake_derive_key(const McpeIdentity *identity,
                              const char *server_public_key_pem,
                              mcpe_u32 server_public_key_pem_length,
                              const mcpe_u8 salt[16],
                              mcpe_u8 key[MCPE_CRYPTO_AES256_KEY_BYTES]);

#endif
