#define NUMC3DS_EXTENSION_IMPLEMENTATION 1
#include "extension.h"

__attribute__((weak)) int numc3ds_extension_install_early_hooks(void) { return 0; }
__attribute__((weak)) int numc3ds_extension_install_hooks(void) { return 0; }
__attribute__((weak)) void numc3ds_extension_frame_begin(void) {}
__attribute__((weak)) void numc3ds_extension_hud_tick(void *screen) { (void)screen; }
__attribute__((weak)) void numc3ds_extension_profiler_flush(void) {}
__attribute__((weak)) int numc3ds_extension_owns_profiler_hook(u32 index)
{
    (void)index;
    return 0;
}
__attribute__((weak)) int numc3ds_extension_install_profiler_probes(void) { return 0; }
__attribute__((weak)) int numc3ds_extension_try_block(const void *source,
                                                       const int *position,
                                                       u8 *id, u8 *data,
                                                       u32 accessor)
{
    (void)source;
    (void)position;
    (void)id;
    (void)data;
    (void)accessor;
    return NUMC3DS_EXTENSION_NOT_HANDLED;
}
__attribute__((weak)) int numc3ds_extension_try_light(const void *source,
                                                       const int *position,
                                                       u8 ambient, u8 *sky,
                                                       u8 *block)
{
    (void)source;
    (void)position;
    (void)ambient;
    (void)sky;
    (void)block;
    return 0;
}
__attribute__((weak)) void numc3ds_extension_note_block_caller(const void *source,
                                                                u32 caller)
{
    (void)source;
    (void)caller;
}
