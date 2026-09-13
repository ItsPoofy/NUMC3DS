#include "ao_blend_fast.h"
#include "../../state.h"

typedef struct {
    u8 sky;
    u8 block;
} BrightnessPair;

static NuMC3DS_Hook ao_blend_hook;

static void ao_blend_fast(
    BrightnessPair *result,
    void *unused,
    BrightnessPair *a,
    BrightnessPair *b,
    BrightnessPair *c,
    const BrightnessPair *center
) {
    u16 c_val = *(const u16 *)center;
    u16 a_val = *(const u16 *)a;
    u16 b_val = *(const u16 *)b;
    u16 d_val = *(const u16 *)c;

    if (a_val == 0) { *(u16 *)a = c_val; a_val = c_val; }
    if (b_val == 0) { *(u16 *)b = c_val; b_val = c_val; }
    if (d_val == 0) { *(u16 *)c = c_val; d_val = c_val; }

    u32 sky = (a_val & 0xFFu) + (b_val & 0xFFu) + (d_val & 0xFFu) + (c_val & 0xFFu);
    u32 block = (a_val >> 8) + (b_val >> 8) + (d_val >> 8) + (c_val >> 8);

    result->sky = (u8)(sky >> 2);
    result->block = (u8)(block >> 2);
}

int ao_blend_fast_install_hook(void)
{
    ao_blend_hook.target = 0x004861D4u;
    ao_blend_hook.replacement = (u32)ao_blend_fast;
    ao_blend_hook.expected[0] = 0xE92D0070u;
    ao_blend_hook.expected[1] = 0xE28D400Cu;
    return s->host.install_hook(&ao_blend_hook) ? -50 : 0;
}
