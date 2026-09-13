#include "resource_lifecycle.h"
#include "internal.h"
#include "ui/progress_tips.h"
#include "ui/ui_widgets.h"
#include "ui/xp_progress_bar.h"
#include "ui/map_preview_texture.h"

typedef void (*LoadClientResourcesFn)(void *,int);

static const char resources_ready[]="NuMC3DS resources ready\n";

static void invalidate_caches(void){
    map_preview_texture_reset();
    s->resources_ready=0;
    s->resource_epoch++;
    xp_progress_bar_reset(&s->progress_bar);
    progress_tips_reset(&s->progress_tips);
    ui_mesh_cache_reset(&s->keyboard.keyboard_background);
    ui_mesh_cache_reset(&s->keyboard.keyboard_focus_background);
    s->keyboard.keyboard_cache_font=0;
    s->history_cache_font=0;
    s->history_cache_generation=~s->history_generation;
}

static void on_load_client_resources(void *game,int reload_existing_atlases){
    invalidate_caches();
    ((LoadClientResourcesFn)s->resource_reload.trampoline)(game,reload_existing_atlases);
    s->resources_ready=1;
    s->host.debug_string(resources_ready,sizeof(resources_ready)-1);
}

int resource_lifecycle_install_hook(void){
    int result;
    s->resources_ready=1;
    s->resource_epoch=1;
    s->resource_reload.target=SEAM_MinecraftGame_loadClientResources;
    s->resource_reload.replacement=(u32)on_load_client_resources;
    s->resource_reload.expected[0]=0xE92D4FF0u;
    s->resource_reload.expected[1]=0xE24DD03Cu;
    result=s->host.install_hook(&s->resource_reload);
    return result?-26:0;
}
