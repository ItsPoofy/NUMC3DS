#ifndef NUMC3DS_MCPE_JWT_H
#define NUMC3DS_MCPE_JWT_H

#include "mcpe_identity.h"

int mcpe_jwt_es384(const McpeIdentity *identity, const char *payload,
                   mcpe_u32 payload_length, char *output, mcpe_u32 capacity,
                   mcpe_u32 *output_length);

#endif
