#include "skin_texture_loader.h"
#include "skin_types.h"
#include "../util/string_util.h"
#include "../../state.h"
#include "../../seams.h"

typedef u32 (*ResourceLoaderLoadTextureFn)(void *loader, void *location, void *texture_data);
typedef void *(*AppPlatformSingletonFn)(void);
typedef u32 (*Texture3DSLoadFn)(void *platform, const void *path, void *texture_data);
typedef void (*StrCtorFn)(void *out, const char *text, void *scratch);
typedef void (*StrDtorFn)(void *out);

static NuMC3DS_Hook s_load_texture_hook;

static int path_has_prefix(const char *path, const char *prefix)
{
    unsigned i;
    if (!path || !prefix) return 0;
    for (i = 0; prefix[i]; i++) {
        if (path[i] != prefix[i]) return 0;
    }
    return path[i] == '/';
}

static u32 on_resource_loader_load_texture(void *loader, void *location, void *texture_data)
{
    const char *path = location ? *(const char **)location : 0;
    if (path_has_prefix(path, NUMC3DS_SKINS_DIR)) {
        void *platform = ((AppPlatformSingletonFn)SEAM_AppPlatform_singleton)();
        if (platform) {
            u32 path_obj = 0;
            u32 scratch = 0;
            u32 result;
            ((StrCtorFn)SEAM_StrCtor)(&path_obj, path, &scratch);
            result = ((Texture3DSLoadFn)SEAM_Texture3DS_load3DST)(platform, &path_obj,
                                                                 texture_data);
            ((StrDtorFn)SEAM_StrDtor)(&path_obj);
            return result;
        }
        return 0;
    }
    return ((ResourceLoaderLoadTextureFn)s_load_texture_hook.trampoline)(loader, location,
                                                                         texture_data);
}

int skin_texture_loader_install_hook(void)
{
    s_load_texture_hook.target = SEAM_ResourceLoader_loadTexture;
    s_load_texture_hook.replacement = (numc3ds_u32)on_resource_loader_load_texture;
    s_load_texture_hook.expected[0] = 0xE92D43F0u;
    s_load_texture_hook.expected[1] = 0xE24DD014u;
    return s->host.install_hook(&s_load_texture_hook);
}
