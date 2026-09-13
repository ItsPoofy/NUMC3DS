#ifndef NUMC3DS_MCPE_P384_H
#define NUMC3DS_MCPE_P384_H

#include "../protocol/mcpe_types.h"

#define MCPE_P384_PRIVATE_BYTES 48u
#define MCPE_P384_PUBLIC_BYTES 97u
#define MCPE_P384_SHARED_SECRET_BYTES 48u
#define MCPE_P384_SIGNATURE_BYTES 96u
#define MCPE_P384_DIGEST_BYTES 48u

int mcpe_p384_generate_private(mcpe_u8 private_key[MCPE_P384_PRIVATE_BYTES]);
int mcpe_p384_public_from_private(const mcpe_u8 private_key[MCPE_P384_PRIVATE_BYTES],
                                  mcpe_u8 public_key[MCPE_P384_PUBLIC_BYTES]);
int mcpe_p384_shared_secret(const mcpe_u8 private_key[MCPE_P384_PRIVATE_BYTES],
                            const mcpe_u8 peer_public_key[MCPE_P384_PUBLIC_BYTES],
                            mcpe_u8 shared_secret[MCPE_P384_SHARED_SECRET_BYTES]);
int mcpe_p384_public_from_pem(const char *pem, mcpe_u32 pem_length,
                              mcpe_u8 public_key[MCPE_P384_PUBLIC_BYTES]);
int mcpe_p384_sign_sha384(const mcpe_u8 private_key[MCPE_P384_PRIVATE_BYTES],
                          const mcpe_u8 digest[MCPE_P384_DIGEST_BYTES],
                          mcpe_u8 signature[MCPE_P384_SIGNATURE_BYTES]);
int mcpe_p384_verify_sha384(const mcpe_u8 public_key[MCPE_P384_PUBLIC_BYTES],
                            const mcpe_u8 digest[MCPE_P384_DIGEST_BYTES],
                            const mcpe_u8 signature[MCPE_P384_SIGNATURE_BYTES]);
int mcpe_p384_sha384(const void *input, mcpe_u32 input_length,
                     mcpe_u8 digest[MCPE_P384_DIGEST_BYTES]);

#endif
