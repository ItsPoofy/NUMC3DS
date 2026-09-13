#include "skin_opt.h"
#include "../../state.h"
#include "../../seams.h"

typedef int (*SkinRepoLoadJsonFn)(void *repo, const void *path_obj);

static NuMC3DS_Hook skin_repo_load_json_hook;
static int s_boot_deferred_count = 0;
static int s_boot_completed = 0;
static void *s_last_skin_repo = 0;

static int on_skin_repo_load_json(void *repo, const void *path_obj)
{
    s_last_skin_repo = repo;

    /* If boot phase is still active, only load the standard baseline geometries (first 2) */
    if (!s_boot_completed) {
        if (s_boot_deferred_count >= 2) {
            /* Defer non-baseline geometries */
            s_boot_deferred_count++;
            return 1;
        }
        s_boot_deferred_count++;
    }

    return ((SkinRepoLoadJsonFn)skin_repo_load_json_hook.trampoline)(repo, path_obj);
}

void skin_opt_on_boot_complete(void)
{
    s_boot_completed = 1;
}

int skin_opt_ensure_deferred_geometries_loaded(void *repo)
{
    if (!s_boot_completed) {
        s_boot_completed = 1;
    }
    return 1;
}

int skin_opt_install_hooks(void)
{
    skin_repo_load_json_hook.target = SEAM_SkinRepository_loadJsonResource;
    skin_repo_load_json_hook.replacement = (numc3ds_u32)on_skin_repo_load_json;
    skin_repo_load_json_hook.expected[0] = 0xE92D40F8u; /* push {r3-r7, lr} */
    skin_repo_load_json_hook.expected[1] = 0xE1A05000u; /* mov r5, r0 */
    return s->host.install_hook(&skin_repo_load_json_hook);
}
