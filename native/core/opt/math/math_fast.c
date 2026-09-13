#include "math_fast.h"
#include "../../state.h"

static NuMC3DS_Hook math_hooks[6];

__attribute__((naked)) void math_fast_vec4_scale(float *out, const float *in, const float *scalar)
{
    __asm__ volatile (
        "vldr    s0, [r2]\n\t"
        "vldmia  r1, {s1, s2, s3, s4}\n\t"
        "vmul.f32 s1, s1, s0\n\t"
        "vmul.f32 s2, s2, s0\n\t"
        "vmul.f32 s3, s3, s0\n\t"
        "vmul.f32 s4, s4, s0\n\t"
        "vstmia  r0, {s1, s2, s3, s4}\n\t"
        "bx      lr\n\t"
    );
}

__attribute__((naked)) void math_fast_vec4_add(float *out, const float *a, const float *b)
{
    __asm__ volatile (
        "vldmia  r1, {s0, s1, s2, s3}\n\t"
        "vldmia  r2, {s4, s5, s6, s7}\n\t"
        "vadd.f32 s0, s0, s4\n\t"
        "vadd.f32 s1, s1, s5\n\t"
        "vadd.f32 s2, s2, s6\n\t"
        "vadd.f32 s3, s3, s7\n\t"
        "vstmia  r0, {s0, s1, s2, s3}\n\t"
        "bx      lr\n\t"
    );
}

__attribute__((naked)) void math_fast_matrix_copy_4x4(float *dst, const float *src)
{
    __asm__ volatile (
        "vldmia  r1, {s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, s12, s13, s14, s15}\n\t"
        "vstmia  r0, {s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, s12, s13, s14, s15}\n\t"
        "bx      lr\n\t"
    );
}

__attribute__((naked)) float math_fast_floorf(float x)
{
    __asm__ volatile (
        "vmov    r0, s0\n\t"
        "lsls    r1, r0, #1\n\t"
        "bxeq    lr\n\t"
        "lsr     r1, r1, #24\n\t"
        "cmp     r1, #150\n\t"
        "bxge    lr\n\t"
        "cmp     r1, #127\n\t"
        "blt     1f\n\t"
        "vcvt.s32.f32 s1, s0\n\t"
        "vcvt.f32.s32 s1, s1\n\t"
        "vcmpe.f32    s0, s1\n\t"
        "vmrs    apsr_nzcv, fpscr\n\t"
        "vmov.f32 s0, s1\n\t"
        "bxge    lr\n\t"
        "vldr    s1, 2f\n\t"
        "vsub.f32 s0, s0, s1\n\t"
        "bx      lr\n\t"
        "1:\n\t"
        "cmp     r0, #0\n\t"
        "vldrlt  s0, 3f\n\t"
        "vldrge  s0, 4f\n\t"
        "bx      lr\n\t"
        ".align 2\n\t"
        "2: .float 1.0\n\t"
        "3: .float -1.0\n\t"
        "4: .float 0.0\n\t"
    );
}

__attribute__((naked)) float math_fast_ceilf(float x)
{
    __asm__ volatile (
        "vmov    r0, s0\n\t"
        "lsls    r1, r0, #1\n\t"
        "bxeq    lr\n\t"
        "lsr     r1, r1, #24\n\t"
        "cmp     r1, #150\n\t"
        "bxge    lr\n\t"
        "cmp     r1, #127\n\t"
        "blt     1f\n\t"
        "vcvt.s32.f32 s1, s0\n\t"
        "vcvt.f32.s32 s1, s1\n\t"
        "vcmpe.f32    s0, s1\n\t"
        "vmrs    apsr_nzcv, fpscr\n\t"
        "vmov.f32 s0, s1\n\t"
        "bxle    lr\n\t"
        "vldr    s1, 2f\n\t"
        "vadd.f32 s0, s0, s1\n\t"
        "bx      lr\n\t"
        "1:\n\t"
        "cmp     r0, #0\n\t"
        "vldrgt  s0, 2f\n\t"
        "vldrle  s0, 3f\n\t"
        "bx      lr\n\t"
        ".align 2\n\t"
        "2: .float 1.0\n\t"
        "3: .float -0.0\n\t"
    );
}

__attribute__((naked)) int math_fast_spheres_intersect(float x1, float y1, float z1, float r1, float x2, float y2, float z2, float r2)
{
    __asm__ volatile (
        "vadd.f32 s7, s3, s7\n\t"
        "vldr     s3, 1f\n\t"
        "vsub.f32 s0, s0, s4\n\t"
        "vsub.f32 s1, s1, s5\n\t"
        "vsub.f32 s2, s2, s6\n\t"
        "vadd.f32 s7, s7, s3\n\t"
        "vcmpe.f32 s7, #0.0\n\t"
        "vmrs    apsr_nzcv, fpscr\n\t"
        "movlt   r0, #0\n\t"
        "bxlt    lr\n\t"
        "vmul.f32 s4, s0, s0\n\t"
        "vmla.f32 s4, s1, s1\n\t"
        "vmla.f32 s4, s2, s2\n\t"
        "vmul.f32 s7, s7, s7\n\t"
        "vcmpe.f32 s4, s7\n\t"
        "vmrs    apsr_nzcv, fpscr\n\t"
        "movle   r0, #1\n\t"
        "movgt   r0, #0\n\t"
        "bx      lr\n\t"
        ".align 2\n\t"
        "1: .float 0.6\n\t"
    );
}

int math_fast_install_hooks(void)
{
    static const u32 targets[6] = {
        0x0012533Cu, /* vec4_scale (244 callers) */
        0x00125360u, /* vec4_add (102 callers) */
        0x0074F240u, /* floorf (49 callers) */
        0x0074F1B0u, /* ceilf (33 callers) */
        0x0011E5CCu, /* matrix_copy_4x4 (40 callers) */
        0x007374C4u  /* Math_spheresIntersect */
    };
    static const u32 expected[6][2] = {
        { 0xECD10A04u, 0xED920A00u },
        { 0xEDD23A00u, 0xED923A01u },
        { 0xEE100A10u, 0xE1A01080u },
        { 0xEE100A10u, 0xE1A01080u },
        { 0xE5912000u, 0xE5802000u },
        { 0xEE713AA3u, 0xEE701A42u }
    };
    u32 replacements[6];
    u32 i;

    replacements[0] = (u32)math_fast_vec4_scale;
    replacements[1] = (u32)math_fast_vec4_add;
    replacements[2] = (u32)math_fast_floorf;
    replacements[3] = (u32)math_fast_ceilf;
    replacements[4] = (u32)math_fast_matrix_copy_4x4;
    replacements[5] = (u32)math_fast_spheres_intersect;

    for (i = 0; i < 6u; i++) {
        math_hooks[i].target = targets[i];
        math_hooks[i].replacement = replacements[i];
        math_hooks[i].expected[0] = expected[i][0];
        math_hooks[i].expected[1] = expected[i][1];
        if (s->host.install_hook(&math_hooks[i])) {
            return -90 - (int)i;
        }
    }
    return 0;
}
