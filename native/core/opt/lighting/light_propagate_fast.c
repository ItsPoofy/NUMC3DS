#include "light_propagate_fast.h"
#include "../../state.h"

static NuMC3DS_Hook light_pair_hook;

static u32 relight_light_pair_fast(u8 *ctx, u32 pos)
{
    u32 mask_x = *(const u32 *)(ctx + 356);
    u32 shift_x = ctx[360];
    u32 mask_z = *(const u32 *)(ctx + 352);
    u32 mask_y = *(const u32 *)(ctx + 368);
    u32 shift_y = ctx[372];

    u32 block_offset = (pos & mask_z) |
                       (((pos & mask_x) >> shift_x)) |
                       (((pos & mask_y) >> shift_y));

    u32 shift_sc_x = ctx[388];
    u32 shift_sc_z = ctx[396];
    u32 shift_sc_y = ctx[392];
    u32 mask_sc = *(const u32 *)(ctx + 380);

    u32 sc_x = (pos >> shift_sc_x) & mask_sc;
    u32 sc_z = (pos >> shift_sc_z) & mask_sc;
    u32 sc_y = (pos >> shift_sc_y) & mask_sc;

    u32 sc_index = (sc_z << 4) | (sc_y << 2) | sc_x;

    /* Mark touched subchunk flag in ctx */
    ctx[240 + sc_index] = 1;

    const void *subchunk = *(const void * const *)(ctx + (sc_index << 2));
    const u8 *light_array = subchunk ? *(const u8 * const *)((const u8 *)subchunk + 0x1800) : 0;

    if (!subchunk) {
        return ctx[349];
    }
    if (light_array) {
        return light_array[block_offset];
    }
    return *(const u8 *)0x00989D32u;
}

int light_propagate_fast_install_hooks(void)
{
    light_pair_hook.target = 0x00384900u;
    light_pair_hook.replacement = (u32)relight_light_pair_fast;
    light_pair_hook.expected[0] = 0xE92D03F0u;
    light_pair_hook.expected[1] = 0xE5902164u;
    return s->host.install_hook(&light_pair_hook) ? -44 : 0;
}
