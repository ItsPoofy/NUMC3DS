#ifndef NUMC3DS_DATAGRAM_BACKEND_H
#define NUMC3DS_DATAGRAM_BACKEND_H

#include "../protocol/mcpe_types.h"

enum McpeDatagramResult {
    MCPE_DATAGRAM_OK = 0,
    MCPE_DATAGRAM_WOULD_BLOCK = 1,
    MCPE_DATAGRAM_DISCONNECTED = 2,
    MCPE_DATAGRAM_PLATFORM_ERROR = -1,
    MCPE_DATAGRAM_INVALID_ARGUMENT = -2,
    MCPE_DATAGRAM_NOT_STARTED = -3,
    MCPE_DATAGRAM_TOO_LARGE = -4
};

enum McpeDatagramState {
    MCPE_DATAGRAM_STOPPED = 0,
    MCPE_DATAGRAM_STARTED = 1
};

typedef struct {
    mcpe_u32 address_be;
    mcpe_u16 port_be;
    mcpe_u16 reserved;
} McpeDatagramEndpoint;

typedef struct {
    int (*start)(void *context, mcpe_u16 port, int allow_broadcast);
    void (*stop)(void *context);
    int (*send)(void *context, const McpeDatagramEndpoint *destination, const void *payload, mcpe_u32 length);
    int (*receive)(void *context, McpeDatagramEndpoint *source, void *payload, mcpe_u32 capacity, mcpe_u32 *length);
    int (*poll)(void *context, mcpe_u32 timeout_ms);
    void (*wakeup)(void *context);
} McpeDatagramBackendOps;

typedef struct {
    const McpeDatagramBackendOps *ops;
    void *context;
    mcpe_u32 payload_limit;
    enum McpeDatagramState state;
} McpeDatagramBackend;

void mcpe_datagram_backend_init(McpeDatagramBackend *backend, const McpeDatagramBackendOps *ops, void *context, mcpe_u32 payload_limit);
int mcpe_datagram_backend_start(McpeDatagramBackend *backend, mcpe_u16 port, int allow_broadcast);
void mcpe_datagram_backend_stop(McpeDatagramBackend *backend);
int mcpe_datagram_backend_send(McpeDatagramBackend *backend, const McpeDatagramEndpoint *destination, const void *payload, mcpe_u32 length);
int mcpe_datagram_backend_receive(McpeDatagramBackend *backend, McpeDatagramEndpoint *source, void *payload, mcpe_u32 capacity, mcpe_u32 *length);
int mcpe_datagram_backend_poll(McpeDatagramBackend *backend, mcpe_u32 timeout_ms);
void mcpe_datagram_backend_wakeup(McpeDatagramBackend *backend);

#endif
