#ifndef NUMC3DS_MCPE_BATCH_H
#define NUMC3DS_MCPE_BATCH_H

#include "mcpe_protocol.h"
#include "mcpe_wire.h"

#define MCPE_BATCH_END 1

int mcpe_batch_write_packet(McpeWireWriter *writer,
                            const void *payload, mcpe_u32 length);
int mcpe_batch_read_packet(McpeWireReader *reader,
                           const mcpe_u8 **payload, mcpe_u32 *length);

#endif
