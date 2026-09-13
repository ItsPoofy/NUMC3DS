#ifndef NUMC3DS_EXTENSION_H
#define NUMC3DS_EXTENSION_H

#include "rt.h"

#ifndef NUMC3DS_EXTENSION_ACTIVE
#define NUMC3DS_EXTENSION_ACTIVE 0
#endif

enum {
    NUMC3DS_EXTENSION_NOT_HANDLED = 0,
    NUMC3DS_EXTENSION_HANDLED = 1,
    NUMC3DS_EXTENSION_REJECTED = 3
};

int numc3ds_extension_install_early_hooks(void);
int numc3ds_extension_install_hooks(void);
void numc3ds_extension_frame_begin(void);
void numc3ds_extension_hud_tick(void *screen);
void numc3ds_extension_profiler_flush(void);
int numc3ds_extension_owns_profiler_hook(u32 index);
int numc3ds_extension_install_profiler_probes(void);
int numc3ds_extension_try_block(const void *source, const int *position,
                                u8 *id, u8 *data, u32 accessor);
int numc3ds_extension_try_light(const void *source, const int *position,
                                u8 ambient, u8 *sky, u8 *block);
void numc3ds_extension_note_block_caller(const void *source, u32 caller);

#if !NUMC3DS_EXTENSION_ACTIVE && !defined(NUMC3DS_EXTENSION_IMPLEMENTATION)
#define numc3ds_extension_install_early_hooks() 0
#define numc3ds_extension_install_hooks() 0
#define numc3ds_extension_frame_begin() ((void)0)
#define numc3ds_extension_hud_tick(screen) ((void)(screen))
#define numc3ds_extension_profiler_flush() ((void)0)
#define numc3ds_extension_owns_profiler_hook(index) 0
#define numc3ds_extension_install_profiler_probes() 0
#endif

#endif
