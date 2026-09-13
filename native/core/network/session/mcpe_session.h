#ifndef NUMC3DS_MCPE_SESSION_H
#define NUMC3DS_MCPE_SESSION_H

#include "../protocol/mcpe_protocol.h"

enum McpeSessionState {
    MCPE_SESSION_IDLE = 0,
    MCPE_SESSION_CONNECTING = 1,
    MCPE_SESSION_LOGIN = 2,
    MCPE_SESSION_ENCRYPTION = 3,
    MCPE_SESSION_RESOURCES = 4,
    MCPE_SESSION_STARTING = 5,
    MCPE_SESSION_GAMEPLAY = 6,
    MCPE_SESSION_DISCONNECTING = 7,
    MCPE_SESSION_CLOSED = 8,
    MCPE_SESSION_FAILED = 9
};

enum McpeSessionResult {
    MCPE_SESSION_OK = 0,
    MCPE_SESSION_INVALID_ARGUMENT = -1,
    MCPE_SESSION_INVALID_STATE = -2,
    MCPE_SESSION_PROTOCOL_MISMATCH = -3
};

typedef struct {
    enum McpeSessionState state;
    mcpe_u32 protocol;
    mcpe_u8 hosting;
    mcpe_u8 reserved[3];
} McpeSession;

void mcpe_session_init(McpeSession *session);
int mcpe_session_begin(McpeSession *session, int hosting, mcpe_u32 protocol);
int mcpe_session_advance(McpeSession *session, enum McpeSessionState next_state);
int mcpe_session_disconnect(McpeSession *session);
int mcpe_session_fail(McpeSession *session);

#endif
