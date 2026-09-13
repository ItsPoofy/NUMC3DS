#include "world_block_service.h"
#include "../util/string_util.h"
#include "../util/math_util.h"
#include "../rt.h"
#include "../commands/native_command_context.h"
#include "../item/item_registry_service.h"

void* world_source(void* level) {
    void* source = native_command_context_block_source();
    void* dim;
    if (source) return source;
    dim = ((GetDimensionFn)0x00720854u)(level, 0);
    return dim ? *(void**)((unsigned char*)dim + 0x50) : 0;
}

unsigned char read_block_id(void* src, const int* pos) {
    unsigned char id = 0;
    ((GetBlockIdFn)0x00174648u)(&id, src, (void*)pos);
    return id;
}

unsigned char read_block_data(void* src, const int* pos) {
    unsigned char block[2] = {0, 0};
    ((GetBlockIdAndDataFn)SEAM_BlockSource_getBlockIdAndData)(block, src, (void*)pos);
    return block[1];
}

int write_block(void* src, const int* pos, unsigned char id, unsigned char data) {
    return ((SetBlockIdDataFn)0x00176AC0u)(src, (void*)pos, &id, data, 3, 0);
}

void destroy_block(void* level, void* src, const int* pos, int drops) {
    ((LevelDestroyBlockFn)SEAM_Level_destroyBlock)(level, src, pos, drops);
}

int region_bounds(const int a[3], const int b[3], int lo[3], int hi[3]) {
    int i;
    long long cells = 1;
    for (i = 0; i < 3; i++) {
        lo[i] = a[i] < b[i] ? a[i] : b[i];
        hi[i] = a[i] > b[i] ? a[i] : b[i];
        if (hi[i] - lo[i] > 32767) return -1;
    }
    if (lo[1] < 0) lo[1] = 0;
    if (hi[1] > 255) hi[1] = 255;
    for (i = 0; i < 3; i++) cells *= (long long)(hi[i] - lo[i] + 1);
    if (cells > 32768) return -1;
    return (int)cells;
}

int boxes_overlap(const int lo1[3], const int hi1[3], const int lo2[3], const int hi2[3]) {
    int i;
    for (i = 0; i < 3; i++) {
        if (lo2[i] > hi1[i] || hi2[i] < lo1[i]) return 0;
    }
    return 1;
}

int topsolid(void* src, const int* pos) {
    return ((TopSolidFn)0x00178C88u)(src, (void*)pos, 0);
}

int resolve_block_arg(char* name, unsigned char* idOut) {
    void* blk;
    {
        const char* q = name;
        int id;
        if (integer(&q, &id) && !*q && id >= 0 && id <= 255) {
            blk = ((void**)SEAM_BlockRegistry_byNumericId)[id];
            if (!blk) return 0;
            *idOut = *(unsigned char*)((unsigned char*)blk + 4);
            return 1;
        }
    }
    blk = block_by_name(strip_ns(name));
    if (blk) {
        *idOut = *(unsigned char*)((unsigned char*)blk + 4);
        return 1;
    }
    return 0;
}
