#ifndef NUMC3DS_MCPE_PEER_ENCRYPTION_H
#define NUMC3DS_MCPE_PEER_ENCRYPTION_H

#include "../protocol/mcpe_types.h"

int mcpe_peer_encryption_enable(void *encrypted_peer,
                                const mcpe_u8 key[32]);

#endif
