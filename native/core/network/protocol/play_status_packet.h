#ifndef NUMC3DS_MCPE_PLAY_STATUS_PACKET_H
#define NUMC3DS_MCPE_PLAY_STATUS_PACKET_H

#include "mcpe_protocol.h"
#include "mcpe_wire.h"

typedef struct {
    mcpe_u32 status;
} McpePlayStatusPacketView;

int mcpe_play_status_packet_encode(const McpePlayStatusPacketView *packet,
                                    void *output, mcpe_u32 capacity,
                                    mcpe_u32 *length);
int mcpe_play_status_packet_decode(const void *payload, mcpe_u32 length,
                                    McpePlayStatusPacketView *packet);

#endif
