#include "matrix_transform_fast.h"
#include "../../state.h"

static NuMC3DS_Hook translate_hook;
static NuMC3DS_Hook scale_hook;

__attribute__((naked)) static void translate_fast(void)
{
    __asm__ volatile(
        "vldmia r1, {s0, s1, s2}\n"
        "add    r1, r0, #0x10\n"
        "vldmia r0, {s4, s5, s6, s7}\n"
        "add    r2, r0, #0x20\n"
        "vldmia r1, {s8, s9, s10, s11}\n"
        "add    r3, r0, #0x30\n"
        "vldmia r2, {s12, s13, s14, s15}\n"

        "vmul.f32 s4, s4, s0\n"
        "vmul.f32 s5, s5, s0\n"
        "vmul.f32 s6, s6, s0\n"
        "vmul.f32 s7, s7, s0\n"

        "vmul.f32 s8, s8, s1\n"
        "vmul.f32 s9, s9, s1\n"
        "vmul.f32 s10, s10, s1\n"
        "vmul.f32 s11, s11, s1\n"

        "vadd.f32 s4, s4, s8\n"
        "vadd.f32 s5, s5, s9\n"
        "vadd.f32 s6, s6, s10\n"
        "vadd.f32 s7, s7, s11\n"

        "vmul.f32 s12, s12, s2\n"
        "vmul.f32 s13, s13, s2\n"
        "vmul.f32 s14, s14, s2\n"
        "vmul.f32 s15, s15, s2\n"

        "vadd.f32 s4, s4, s12\n"
        "vadd.f32 s5, s5, s13\n"
        "vadd.f32 s6, s6, s14\n"
        "vadd.f32 s7, s7, s15\n"

        "vldmia r3, {s8, s9, s10, s11}\n"
        "vadd.f32 s4, s4, s8\n"
        "vadd.f32 s5, s5, s9\n"
        "vadd.f32 s6, s6, s10\n"
        "vadd.f32 s7, s7, s11\n"

        "vstmia r3, {s4, s5, s6, s7}\n"
        "bx lr\n"
    );
}

__attribute__((naked)) static void scale_fast(void)
{
    __asm__ volatile(
        "vldmia r0, {s4, s5, s6, s7}\n"
        "vmul.f32 s4, s4, s0\n"
        "vmul.f32 s5, s5, s0\n"
        "vmul.f32 s6, s6, s0\n"
        "vmul.f32 s7, s7, s0\n"
        "vstmia r0, {s4, s5, s6, s7}\n"

        "add    r1, r0, #0x10\n"
        "vldmia r1, {s8, s9, s10, s11}\n"
        "vmul.f32 s8, s8, s1\n"
        "vmul.f32 s9, s9, s1\n"
        "vmul.f32 s10, s10, s1\n"
        "vmul.f32 s11, s11, s1\n"
        "vstmia r1, {s8, s9, s10, s11}\n"

        "add    r2, r0, #0x20\n"
        "vldmia r2, {s12, s13, s14, s15}\n"
        "vmul.f32 s12, s12, s2\n"
        "vmul.f32 s13, s13, s2\n"
        "vmul.f32 s14, s14, s2\n"
        "vmul.f32 s15, s15, s2\n"
        "vstmia r2, {s12, s13, s14, s15}\n"

        "bx lr\n"
    );
}

int matrix_transform_fast_install_hooks(void)
{
    translate_hook.target = 0x005FC40Cu;
    translate_hook.replacement = (u32)translate_fast;
    translate_hook.expected[0] = 0xE92D40F0u;
    translate_hook.expected[1] = 0xE24DD0B4u;
    if (s->host.install_hook(&translate_hook)) return -66;

    scale_hook.target = 0x005FC1F8u;
    scale_hook.replacement = (u32)scale_fast;
    scale_hook.expected[0] = 0xE92D40F0u;
    scale_hook.expected[1] = 0xE24DD094u;
    if (s->host.install_hook(&scale_hook)) return -67;

    return 0;
}
