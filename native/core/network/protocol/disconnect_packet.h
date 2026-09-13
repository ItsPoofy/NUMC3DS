#ifndef NUMC3DS_MCPE_DISCONNECT_PACKET_H
#define NUMC3DS_MCPE_DISCONNECT_PACKET_H

#include "mcpe_protocol.h"
#include "mcpe_wire.h"

typedef struct {
    int hide_disconnection_screen;
    const char *message;
    mcpe_u32 message_length;
} McpeDisconnectPacketView;

typedef struct {
    int hide_disconnection_screen;
    char *message;
    mcpe_u32 message_length;
    mcpe_u32 message_capacity;
} McpeDisconnectPacketTarget;

int mcpe_disconnect_packet_encode(
    const McpeDisconnectPacketView *packet,
    void *output, mcpe_u32 capacity, mcpe_u32 *length);
int mcpe_disconnect_packet_decode(
    const void *payload, mcpe_u32 length,
    McpeDisconnectPacketTarget *packet);

#endif
