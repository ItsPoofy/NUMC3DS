#ifndef NUMC3DS_MCPE_IDENTITY_H
#define NUMC3DS_MCPE_IDENTITY_H

#include "mcpe_p384.h"

#define MCPE_IDENTITY_PUBLIC_KEY_PEM_CAPACITY 224u

typedef struct {
    mcpe_u8 private_key[MCPE_P384_PRIVATE_BYTES];
    mcpe_u8 public_key[MCPE_P384_PUBLIC_BYTES];
    mcpe_u8 uuid[16];
    mcpe_u8 client_random_id[8];
    char public_key_pem[MCPE_IDENTITY_PUBLIC_KEY_PEM_CAPACITY];
    mcpe_u32 public_key_pem_length;
} McpeIdentity;

int mcpe_identity_get(McpeIdentity **identity);
int mcpe_identity_random_bytes(mcpe_u8 *output, mcpe_u32 length);

#endif
