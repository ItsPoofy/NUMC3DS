#include "tessellator_tex_fast.h"
#include "../../state.h"

static NuMC3DS_Hook tex_hook;

static inline float clamp_0_1(float v)
{
    if (v > 1.0f) return 1.0f;
    if (!(v > 0.0f)) return 0.0f;
    return v;
}

static void tessellator_tex_fast(u8 *tess, const float *uv, int unit)
{
    float *dst = (float *)(tess + 0x68 + (unit << 3));
    dst[0] = clamp_0_1(uv[0]);
    dst[1] = clamp_0_1(uv[1]);
    *(u32 *)(tess + 0x34) |= (1u << (unit + 3));
}

int tessellator_tex_fast_install_hook(void)
{
    tex_hook.target = 0x001B17B8u;
    tex_hook.replacement = (u32)tessellator_tex_fast;
    tex_hook.expected[0] = 0xE92D4030u;
    tex_hook.expected[1] = 0xE1A04000u;
    return s->host.install_hook(&tex_hook) ? -72 : 0;
}
