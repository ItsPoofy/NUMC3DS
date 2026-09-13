#include "datagram_backend.h"

void mcpe_datagram_backend_init(McpeDatagramBackend *backend, const McpeDatagramBackendOps *ops, void *context, mcpe_u32 payload_limit)
{
    if (!backend) return;
    backend->ops = ops;
    backend->context = context;
    backend->payload_limit = payload_limit;
    backend->state = MCPE_DATAGRAM_STOPPED;
}

int mcpe_datagram_backend_start(McpeDatagramBackend *backend, mcpe_u16 port, int allow_broadcast)
{
    int result;
    if (!backend || !backend->ops || !backend->ops->start) return MCPE_DATAGRAM_INVALID_ARGUMENT;
    if (backend->state == MCPE_DATAGRAM_STARTED) return MCPE_DATAGRAM_OK;
    result = backend->ops->start(backend->context, port, allow_broadcast);
    if (result == MCPE_DATAGRAM_OK) backend->state = MCPE_DATAGRAM_STARTED;
    return result;
}

void mcpe_datagram_backend_stop(McpeDatagramBackend *backend)
{
    if (!backend) return;
    if (backend->state == MCPE_DATAGRAM_STARTED && backend->ops && backend->ops->stop) {
        backend->ops->stop(backend->context);
    }
    backend->state = MCPE_DATAGRAM_STOPPED;
}

int mcpe_datagram_backend_send(McpeDatagramBackend *backend, const McpeDatagramEndpoint *destination, const void *payload, mcpe_u32 length)
{
    if (!backend || backend->state != MCPE_DATAGRAM_STARTED || !backend->ops || !backend->ops->send) return MCPE_DATAGRAM_NOT_STARTED;
    if (!destination || (length && !payload)) return MCPE_DATAGRAM_INVALID_ARGUMENT;
    if (backend->payload_limit && length > backend->payload_limit) return MCPE_DATAGRAM_TOO_LARGE;
    return backend->ops->send(backend->context, destination, payload, length);
}

int mcpe_datagram_backend_receive(McpeDatagramBackend *backend, McpeDatagramEndpoint *source, void *payload, mcpe_u32 capacity, mcpe_u32 *length)
{
    if (length) *length = 0;
    if (!backend || backend->state != MCPE_DATAGRAM_STARTED || !backend->ops || !backend->ops->receive) return MCPE_DATAGRAM_NOT_STARTED;
    if (!source || (capacity && !payload)) return MCPE_DATAGRAM_INVALID_ARGUMENT;
    return backend->ops->receive(backend->context, source, payload, capacity, length);
}

int mcpe_datagram_backend_poll(McpeDatagramBackend *backend, mcpe_u32 timeout_ms)
{
    if (!backend || backend->state != MCPE_DATAGRAM_STARTED || !backend->ops || !backend->ops->poll) return MCPE_DATAGRAM_NOT_STARTED;
    return backend->ops->poll(backend->context, timeout_ms);
}

void mcpe_datagram_backend_wakeup(McpeDatagramBackend *backend)
{
    if (!backend || backend->state != MCPE_DATAGRAM_STARTED || !backend->ops || !backend->ops->wakeup) return;
    backend->ops->wakeup(backend->context);
}
