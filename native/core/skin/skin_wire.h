#ifndef NUMC3DS_SKIN_WIRE_H
#define NUMC3DS_SKIN_WIRE_H

#include "skin_types.h"

typedef struct {
    numc3ds_u8 *bytes;
    numc3ds_u32 length;
    const char *skin_id;
    numc3ds_u8 slim;
    numc3ds_u8 reserved[3];
} McpeSkinWireAsset;

int skin_wire_build(McpeSkinWireAsset *asset);
void skin_wire_release(McpeSkinWireAsset *asset);
int skin_wire_encode_3dst(const numc3ds_u8 *src_rgba, numc3ds_u32 width, numc3ds_u32 height,
                          numc3ds_u8 *dst_3dst, numc3ds_u32 dst_capacity, numc3ds_u32 *out_length);

#endif
