#include "light_read_fast.h"
#include "../../state.h"
#include "../../extension.h"

#ifndef NUMC3DS_OPTIMIZATIONS
#define NUMC3DS_OPTIMIZATIONS 1
#endif
#ifndef NUMC3DS_EXTENSION_FAST_READS
#define NUMC3DS_EXTENSION_FAST_READS 0
#endif

static NuMC3DS_Hook light_read_hook;

void light_read_stock_color(u8 *result, const void *source, const int *position,
                            const u8 *ambient)
{
    ((void (*)(u8 *, const void *, const int *, const u8 *))light_read_hook.trampoline)
        (result, source, position, ambient);
}

static void read_light_color(u8 *result, const u8 *source, const int *position, const u8 *ambient)
{
    u8 sky;
    u8 block;
    u8 amb = *ambient;
    int y = position[1];

#if NUMC3DS_EXTENSION_FAST_READS
    if (numc3ds_extension_try_light(source, position, amb, &result[0], &result[1])) return;
#endif

#if !NUMC3DS_OPTIMIZATIONS
    light_read_stock_color(result, source, position, ambient);
    return;
#endif

    if (y < 0 || y >= *(const short *)(source + 0x18)) {
        sky = source[60];
        block = source[61];
        if (block < amb) block = amb;
        result[0] = sky;
        result[1] = block;
        return;
    }

    const u8 *chunk = *(const u8 * const *)(source + 0x34);
    if (chunk && *(const int *)(chunk + 0x20) == (position[0] >> 4) &&
        *(const int *)(chunk + 0x24) == (position[2] >> 4)) {
        u32 section = (u32)y >> 4;
        u32 section_count = *(const u32 *)(chunk + 0x7c);
        if (section >= section_count) {
            sky = chunk[0xb44];
            block = chunk[0xb45];
        } else {
            const u8 *subchunk = *(const u8 * const *)(chunk + 0x5c + section * 4u);
            if (subchunk) {
                const u8 *light_array = *(const u8 * const *)(subchunk + 0x1800);
                if (light_array) {
                    u32 idx = (((u32)position[0] & 15u) << 8) |
                              (((u32)position[2] & 15u) << 4) |
                              ((u32)y & 15u);
                    u8 val = light_array[idx];
                    sky = val >> 4;
                    block = val & 15u;
                } else {
                    sky = 0;
                    block = 0;
                }
            } else {
                sky = 0;
                block = 0;
            }
        }
        if (block < amb) block = amb;
        result[0] = sky;
        result[1] = block;
        return;
    }

    light_read_stock_color(result, source, position, ambient);
}

int light_read_fast_install_hooks(void)
{
    light_read_hook.target = 0x00175F28u;
    light_read_hook.replacement = (u32)read_light_color;
    light_read_hook.expected[0] = 0xE92D40F0u;
    light_read_hook.expected[1] = 0xE1A04000u;
    return s->host.install_hook(&light_read_hook) ? -70 : 0;
}
