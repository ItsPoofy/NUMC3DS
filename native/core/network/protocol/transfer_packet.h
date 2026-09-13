#ifndef NUMC3DS_MCPE_TRANSFER_PACKET_H
#define NUMC3DS_MCPE_TRANSFER_PACKET_H

#include "mcpe_protocol.h"
#include "mcpe_wire.h"

typedef struct {
    const char *address;
    mcpe_u32 address_length;
    mcpe_u16 port;
} McpeTransferPacketView;

typedef struct {
    char *address;
    mcpe_u32 address_length;
    mcpe_u32 address_capacity;
    mcpe_u16 port;
} McpeTransferPacketTarget;

int mcpe_transfer_packet_encode(
    const McpeTransferPacketView *packet,
    void *output, mcpe_u32 capacity, mcpe_u32 *length);
int mcpe_transfer_packet_decode(
    const void *payload, mcpe_u32 length,
    McpeTransferPacketTarget *packet);

#endif
