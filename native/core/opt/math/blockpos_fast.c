#include "blockpos_fast.h"
#include "../../state.h"

static NuMC3DS_Hook blockpos_hooks[3];

static inline int fast_floor(float val)
{
    int i = (int)val;
    return val < (float)i ? i - 1 : i;
}

void *blockpos_from_vec3(int *result, const float *vec)
{
    result[0] = fast_floor(vec[0]);
    result[1] = fast_floor(vec[1]);
    result[2] = fast_floor(vec[2]);
    return result;
}

void *blockpos_from_floats(int *result, float x, float y, float z)
{
    result[0] = fast_floor(x);
    result[1] = fast_floor(y);
    result[2] = fast_floor(z);
    return result;
}

void *chunkpos_from_blockpos(int *result, const int *block_pos)
{
    result[0] = block_pos[0] >> 4;
    result[1] = block_pos[2] >> 4;
    return result;
}

int blockpos_fast_install_hooks(void)
{
    static const u32 targets[3] = {
        0x0064A984u,
        0x0064AAB8u,
        0x0064CA6Cu
    };
    static const u32 expected[3][2] = {
        { 0xE92D4010u, 0xE1A04000u },
        { 0xE92D4010u, 0xE1A04000u },
        { 0xE5912000u, 0xE1A02242u }
    };
    u32 replacements[3];
    u32 i;
    replacements[0] = (u32)blockpos_from_vec3;
    replacements[1] = (u32)blockpos_from_floats;
    replacements[2] = (u32)chunkpos_from_blockpos;

    for (i = 0; i < 3; i++) {
        blockpos_hooks[i].target = targets[i];
        blockpos_hooks[i].replacement = replacements[i];
        blockpos_hooks[i].expected[0] = expected[i][0];
        blockpos_hooks[i].expected[1] = expected[i][1];
        if (s->host.install_hook(&blockpos_hooks[i])) return -80 - (int)i;
    }
    return 0;
}
