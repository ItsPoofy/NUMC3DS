#ifndef NUMC3DS_MCPE_CONNECTION_REQUEST_H
#define NUMC3DS_MCPE_CONNECTION_REQUEST_H

#include "../protocol/mcpe_protocol.h"
#include "../protocol/mcpe_wire.h"

typedef struct {
    const mcpe_u8 *certificate;
    mcpe_u32 certificate_length;
    const mcpe_u8 *web_token;
    mcpe_u32 web_token_length;
} McpeConnectionRequestView;

int mcpe_connection_request_encode(const McpeConnectionRequestView *request,
                                   void *output, mcpe_u32 capacity,
                                   mcpe_u32 *length);
int mcpe_connection_request_decode(const void *payload, mcpe_u32 length,
                                   McpeConnectionRequestView *request);

#endif
