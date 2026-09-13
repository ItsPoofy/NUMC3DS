#ifndef NUMC3DS_HOOK_MANAGER_H
#define NUMC3DS_HOOK_MANAGER_H

#include "../include/numc3ds_abi.h"

void hook_manager_reset(void);
numc3ds_s32 hook_manager_install(NuMC3DS_Hook *hook);
numc3ds_s32 hook_manager_remove(NuMC3DS_Hook *hook);
numc3ds_s32 hook_manager_patch_call(numc3ds_u32 target, numc3ds_u32 expected,
                                    numc3ds_u32 replacement);
numc3ds_s32 hook_manager_remove_call(numc3ds_u32 target);
void hook_manager_rollback_all(void);

#endif
