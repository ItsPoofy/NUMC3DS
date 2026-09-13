#include "mcpe_session.h"

void mcpe_session_init(McpeSession *session)
{
    if (!session) return;
    session->state = MCPE_SESSION_IDLE;
    session->protocol = 0;
    session->hosting = 0;
    session->reserved[0] = 0;
    session->reserved[1] = 0;
    session->reserved[2] = 0;
}

int mcpe_session_begin(McpeSession *session, int hosting, mcpe_u32 protocol)
{
    if (!session) return MCPE_SESSION_INVALID_ARGUMENT;
    if (session->state != MCPE_SESSION_IDLE && session->state != MCPE_SESSION_CLOSED && session->state != MCPE_SESSION_FAILED) return MCPE_SESSION_INVALID_STATE;
    if (protocol != MCPE_PROTOCOL_VERSION) return MCPE_SESSION_PROTOCOL_MISMATCH;
    session->state = MCPE_SESSION_CONNECTING;
    session->protocol = protocol;
    session->hosting = hosting ? 1 : 0;
    return MCPE_SESSION_OK;
}

static int mcpe_session_valid_next(enum McpeSessionState current, enum McpeSessionState next_state)
{
    if (current == MCPE_SESSION_CONNECTING && next_state == MCPE_SESSION_LOGIN) return 1;
    if (current == MCPE_SESSION_LOGIN && next_state == MCPE_SESSION_ENCRYPTION) return 1;
    if (current == MCPE_SESSION_ENCRYPTION && next_state == MCPE_SESSION_RESOURCES) return 1;
    if (current == MCPE_SESSION_RESOURCES && next_state == MCPE_SESSION_STARTING) return 1;
    if (current == MCPE_SESSION_STARTING && next_state == MCPE_SESSION_GAMEPLAY) return 1;
    if (current == MCPE_SESSION_GAMEPLAY && next_state == MCPE_SESSION_DISCONNECTING) return 1;
    if (current == MCPE_SESSION_CONNECTING && next_state == MCPE_SESSION_DISCONNECTING) return 1;
    if (current == MCPE_SESSION_LOGIN && next_state == MCPE_SESSION_DISCONNECTING) return 1;
    if (current == MCPE_SESSION_ENCRYPTION && next_state == MCPE_SESSION_DISCONNECTING) return 1;
    if (current == MCPE_SESSION_RESOURCES && next_state == MCPE_SESSION_DISCONNECTING) return 1;
    if (current == MCPE_SESSION_STARTING && next_state == MCPE_SESSION_DISCONNECTING) return 1;
    if (next_state == MCPE_SESSION_FAILED && current != MCPE_SESSION_IDLE && current != MCPE_SESSION_CLOSED) return 1;
    return 0;
}

int mcpe_session_advance(McpeSession *session, enum McpeSessionState next_state)
{
    if (!session) return MCPE_SESSION_INVALID_ARGUMENT;
    if (!mcpe_session_valid_next(session->state, next_state)) return MCPE_SESSION_INVALID_STATE;
    session->state = next_state;
    return MCPE_SESSION_OK;
}

int mcpe_session_disconnect(McpeSession *session)
{
    if (!session) return MCPE_SESSION_INVALID_ARGUMENT;
    if (session->state == MCPE_SESSION_IDLE || session->state == MCPE_SESSION_CLOSED) return MCPE_SESSION_INVALID_STATE;
    session->state = MCPE_SESSION_DISCONNECTING;
    session->state = MCPE_SESSION_CLOSED;
    return MCPE_SESSION_OK;
}

int mcpe_session_fail(McpeSession *session)
{
    if (!session) return MCPE_SESSION_INVALID_ARGUMENT;
    if (session->state == MCPE_SESSION_IDLE || session->state == MCPE_SESSION_CLOSED) return MCPE_SESSION_INVALID_STATE;
    session->state = MCPE_SESSION_FAILED;
    return MCPE_SESSION_OK;
}
