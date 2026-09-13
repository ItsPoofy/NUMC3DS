#include "mcpe_map_compat.h"
#include "../../hook_manager.h"
#include "../../state.h"

static NuMC3DS_Hook map_read_hook;

__attribute__((naked)) static void on_map_read_skip_extra_ints(void)
{
    __asm__ volatile (
        "mov r0, #0\n"
        "str r0, [r4, #20]\n"
        "str r0, [r4, #24]\n"
        "ldr pc, =0x00499A04\n"
    );
}

int mcpe_map_compat_install(void)
{
    zero(&map_read_hook, sizeof(map_read_hook));
    map_read_hook.target = 0x004999ECu;
    map_read_hook.replacement = (u32)on_map_read_skip_extra_ints;
    map_read_hook.expected[0] = 0xE59D0048u;
    map_read_hook.expected[1] = 0xEBFD8037u;
    return s->host.install_hook(&map_read_hook);
}
