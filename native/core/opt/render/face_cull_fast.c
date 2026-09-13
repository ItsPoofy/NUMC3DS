#include "face_cull_fast.h"
#include "../../state.h"

static NuMC3DS_Hook occludes_hook;

__attribute__((naked)) static void occludes_fast(void)
{
    __asm__ volatile(
        "ldr r0, [r0]\n"
        "lsr r0, r0, r1\n"
        "and r0, r0, #1\n"
        "bx lr\n"
    );
}

int face_cull_fast_install_hook(void)
{
    occludes_hook.target = 0x006BB090u;
    occludes_hook.replacement = (u32)occludes_fast;
    occludes_hook.expected[0] = 0xE92D4030u;
    occludes_hook.expected[1] = 0xE1B04001u;
    return s->host.install_hook(&occludes_hook) ? -61 : 0;
}
