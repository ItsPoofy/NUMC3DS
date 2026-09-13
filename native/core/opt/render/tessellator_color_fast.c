#include "tessellator_color_fast.h"
#include "../../state.h"

static NuMC3DS_Hook color_abgr_hook;
static NuMC3DS_Hook color_rgba_hook;
static NuMC3DS_Hook color_float_hook;

__attribute__((naked)) static void tessellator_color_abgr_fast(void)
{
    __asm__ volatile(
        "ldrb r2, [r0, #0xd0]\n"
        "cmp  r2, #0\n"
        "bxne lr\n"
        "str  r1, [r0, #0x80]\n"
        "ldr  r2, [r0, #0x34]\n"
        "orr  r2, r2, #2\n"
        "str  r2, [r0, #0x34]\n"
        "bx   lr\n"
    );
}

static inline u32 pack_floats_abgr(float r, float g, float b, float a)
{
    float scale = 255.0f;
    u32 ir = (u32)(r * scale);
    u32 ig = (u32)(g * scale);
    u32 ib = (u32)(b * scale);
    u32 ia = (u32)(a * scale);
    return (ir & 0xFFu) | ((ig & 0xFFu) << 8) | ((ib & 0xFFu) << 16) | ((ia & 0xFFu) << 24);
}

static void tessellator_color_rgba_fast(u8 *tess, float r, float g, float b, float a)
{
    if (!tess[0xd0]) {
        *(u32 *)(tess + 0x80) = pack_floats_abgr(r, g, b, a);
        *(u32 *)(tess + 0x34) |= 2u;
    }
}

static void tessellator_color_float_fast(u8 *tess, const float *color)
{
    if (!tess[0xd0]) {
        *(u32 *)(tess + 0x80) = pack_floats_abgr(color[0], color[1], color[2], color[3]);
        *(u32 *)(tess + 0x34) |= 2u;
    }
}

int tessellator_color_fast_install_hook(void)
{
    color_abgr_hook.target = 0x001B30A8u;
    color_abgr_hook.replacement = (u32)tessellator_color_abgr_fast;
    color_abgr_hook.expected[0] = 0xE92D4010u;
    color_abgr_hook.expected[1] = 0xE1A04000u;
    if (s->host.install_hook(&color_abgr_hook)) return -73;

    color_rgba_hook.target = 0x001B1FECu;
    color_rgba_hook.replacement = (u32)tessellator_color_rgba_fast;
    color_rgba_hook.expected[0] = 0xED9F2A2Fu;
    color_rgba_hook.expected[1] = 0xE92D4010u;
    if (s->host.install_hook(&color_rgba_hook)) return -74;

    color_float_hook.target = 0x001B1F00u;
    color_float_hook.replacement = (u32)tessellator_color_float_fast;
    color_float_hook.expected[0] = 0xE92D4010u;
    color_float_hook.expected[1] = 0xED9F0A32u;
    if (s->host.install_hook(&color_float_hook)) return -75;

    return 0;
}
