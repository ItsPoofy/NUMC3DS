#include "fishing_hook_collision_fix.h"
#include "../state.h"

enum { FISHING_HOOK_HIT_RESULT_SIZE = 0x45 };

typedef void (*FishingHookHitCheckFn)(void *, void *);
typedef unsigned long long (*SynchedEntityDataGetDoubleFn)(void *, int);
typedef void *(*EntityGetLevelFn)(void *);
typedef void *(*LevelFetchEntityFn)(void *, unsigned long long, int);

static int fishing_hook_has_attached_target(void *hook) {
    unsigned long long id;
    void *level;

    id = ((SynchedEntityDataGetDoubleFn)SEAM_SynchedEntityData_getDouble)(
        (u8 *)hook + SEAM_Entity_synchedDataOffset, 6);
    if (id == 0xffffffffffffffffULL) return 0;
    level = ((EntityGetLevelFn)SEAM_Entity_getLevel)(hook);
    return level && ((LevelFetchEntityFn)SEAM_Level_fetchEntity)(level, id, 0) != 0;
}

static void on_fishing_hook_hit_check(void *result, void *hook) {
    if (!result || !hook) {
        ((FishingHookHitCheckFn)s->fishing_hook_hit_check.trampoline)(result, hook);
        return;
    }
    if (fishing_hook_has_attached_target(hook)) {
        zero(result, FISHING_HOOK_HIT_RESULT_SIZE);
        return;
    }
    ((FishingHookHitCheckFn)s->fishing_hook_hit_check.trampoline)(result, hook);
}

int fishing_hook_collision_fix_install_hook(void) {
    NuMC3DS_Hook *hook = &s->fishing_hook_hit_check;

    hook->target = SEAM_FishingHook_hitCheck;
    hook->replacement = (u32)on_fishing_hook_hit_check;
    hook->expected[0] = 0xE92D4FF0u;
    hook->expected[1] = 0xE1A04000u;
    return s->host.install_hook(hook) ? -42 : 0;
}
