#include "uv_fast.h"
#include "../../state.h"

static NuMC3DS_Hook uv_hooks[4];

__attribute__((naked, used)) void *texture_uv_coordinate_set_copy_fast(void *dst, const void *src)
{
    __asm__ volatile(
        "mov   ip, r0\n"
        /* Copy 28 bytes from src (r1) to dst (r0) */
        "ldmia r1!, {r2, r3}\n"
        "stmia ip!, {r2, r3}\n"
        "ldmia r1!, {r2, r3}\n"
        "stmia ip!, {r2, r3}\n"
        "ldmia r1!, {r2, r3}\n"
        "stmia ip!, {r2, r3}\n"
        "ldr   r2, [r1]\n"
        "str   r2, [ip], #4\n"
        /* Initialize 28-byte subobject at dst+28: 24 zero bytes + 0x00000009 */
        "mov   r1, #0\n"
        "mov   r2, #0\n"
        "mov   r3, #0\n"
        "stmia ip!, {r1, r2, r3}\n"
        "stmia ip!, {r1, r2, r3}\n"
        "mov   r1, #9\n"
        "strb  r1, [ip]\n"
        "bx    lr\n"
    );
}

static void *texture_uv_coordinate_set_copy(void *dst, const void *src)
{
    u32 destination = (u32)dst;
    u32 source = (u32)src;
    if (source < destination + 53u && destination < source + 28u) {
        return ((void *(*)(void *, const void *))uv_hooks[0].trampoline)
            (dst, src);
    }
    return texture_uv_coordinate_set_copy_fast(dst, src);
}

#define ATLAS_UV_INSET_RATIO 0.015625f

static void interpolate_uvs_fast(void *renderer, void *texture,
                                 float *uv0, float *uv1,
                                 float *uv2, float *uv3)
{
    u32 *atlas = *(u32 **)((u8 *)texture + 4);
    u32 current_version;
    u32 cached_version;
    float *bounds;
    float min_u;
    float min_v;
    float delta_u;
    float delta_v;
    float inset_u;
    float inset_v;
    if (!atlas) atlas = *(u32 **)0x0091BA44u;
    if (__builtin_expect(!atlas, 0)) goto slow;
    current_version = atlas[0x88 / 4];
    cached_version = *(u32 *)((u8 *)texture + 0x18);
    if (__builtin_expect(current_version != cached_version, 0)) goto slow;
    bounds = *(float **)((u8 *)texture + 0x14);
    if (__builtin_expect(!bounds, 0)) goto slow;

    min_u = bounds[0];
    min_v = bounds[1];
    delta_u = bounds[2] - min_u;
    delta_v = bounds[3] - min_v;

    inset_u = delta_u * ATLAS_UV_INSET_RATIO;
    inset_v = delta_v * ATLAS_UV_INSET_RATIO;
    min_u += inset_u;
    min_v += inset_v;
    delta_u -= (inset_u * 2.0f);
    delta_v -= (inset_v * 2.0f);

    uv0[0] = min_u + uv0[0] * delta_u;
    uv0[1] = min_v + uv0[1] * delta_v;
    if (uv1) {
        uv1[0] = min_u + uv1[0] * delta_u;
        uv1[1] = min_v + uv1[1] * delta_v;
    }
    if (uv2) {
        uv2[0] = min_u + uv2[0] * delta_u;
        uv2[1] = min_v + uv2[1] * delta_v;
    }
    if (uv3) {
        uv3[0] = min_u + uv3[0] * delta_u;
        uv3[1] = min_v + uv3[1] * delta_v;
    }
    return;

slow:
    ((void (*)(void *, void *, float *, float *, float *, float *))
        uv_hooks[1].trampoline)(renderer, texture, uv0, uv1, uv2, uv3);
}

static float get_interpolated_u_fast(void *texture, float u, float base)
{
    u32 *atlas = *(u32 **)((u8 *)texture + 4);
    u32 current_version;
    u32 cached_version;
    float *bounds;
    float min_u;
    float delta_u;
    float inset_u;
    if (!atlas) atlas = *(u32 **)0x0091BA44u;
    if (__builtin_expect(!atlas, 0)) goto slow;
    current_version = atlas[0x88 / 4];
    cached_version = *(u32 *)((u8 *)texture + 0x18);
    if (__builtin_expect(current_version != cached_version, 0)) goto slow;
    bounds = *(float **)((u8 *)texture + 0x14);
    if (__builtin_expect(!bounds, 0)) goto slow;

    min_u = bounds[0];
    delta_u = bounds[2] - min_u;
    inset_u = delta_u * ATLAS_UV_INSET_RATIO;
    min_u += inset_u;
    delta_u -= (inset_u * 2.0f);

    if (__builtin_expect(base != 0.0f, 0)) return base + u * delta_u;
    return min_u + u * delta_u;

slow:
    return ((float (*)(void *, float, float))uv_hooks[2].trampoline)
        (texture, u, base);
}

static float get_interpolated_v_fast(void *texture, float v, float base)
{
    u32 *atlas = *(u32 **)((u8 *)texture + 4);
    u32 current_version;
    u32 cached_version;
    float *bounds;
    float min_v;
    float delta_v;
    float inset_v;
    if (!atlas) atlas = *(u32 **)0x0091BA44u;
    if (__builtin_expect(!atlas, 0)) goto slow;
    current_version = atlas[0x88 / 4];
    cached_version = *(u32 *)((u8 *)texture + 0x18);
    if (__builtin_expect(current_version != cached_version, 0)) goto slow;
    bounds = *(float **)((u8 *)texture + 0x14);
    if (__builtin_expect(!bounds, 0)) goto slow;

    min_v = bounds[1];
    delta_v = bounds[3] - min_v;
    inset_v = delta_v * ATLAS_UV_INSET_RATIO;
    min_v += inset_v;
    delta_v -= (inset_v * 2.0f);

    if (__builtin_expect(base != 0.0f, 0)) return base + v * delta_v;
    return min_v + v * delta_v;

slow:
    return ((float (*)(void *, float, float))uv_hooks[3].trampoline)
        (texture, v, base);
}

int uv_fast_install_hooks(void)
{
    uv_hooks[0].target = 0x0048509Cu;
    uv_hooks[0].replacement = (u32)texture_uv_coordinate_set_copy;
    uv_hooks[0].expected[0] = 0xE92D4010u;
    uv_hooks[0].expected[1] = 0xE1A04001u;
    if (s->host.install_hook(&uv_hooks[0])) return -62;

    uv_hooks[1].target = 0x0030ACB4u;
    uv_hooks[1].replacement = (u32)interpolate_uvs_fast;
    uv_hooks[1].expected[0] = 0xE92D41F0u;
    uv_hooks[1].expected[1] = 0xE1A04003u;
    if (s->host.install_hook(&uv_hooks[1])) return -63;

    uv_hooks[2].target = 0x00709544u;
    uv_hooks[2].replacement = (u32)get_interpolated_u_fast;
    uv_hooks[2].expected[0] = 0xE92D4070u;
    uv_hooks[2].expected[1] = 0xE1A04000u;
    if (s->host.install_hook(&uv_hooks[2])) return -64;

    uv_hooks[3].target = 0x007095D0u;
    uv_hooks[3].replacement = (u32)get_interpolated_v_fast;
    uv_hooks[3].expected[0] = 0xE92D4070u;
    uv_hooks[3].expected[1] = 0xE1A04000u;
    if (s->host.install_hook(&uv_hooks[3])) return -65;

    return 0;
}
