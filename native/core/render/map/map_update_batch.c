#include "map_update_batch.h"
#include "map_renderer.h"
#include "../../state.h"
#include "../../ui/map_preview_texture.h"

static NuMC3DS_Hook sample_hook;
static void *active_data;
static int active_changed;

void map_update_batch_notify(void *data){
    if(data==active_data){
        active_changed=1;
        return;
    }
    map_preview_texture_dirty(data);
    map_renderer_update(data);
}

static void on_sample(void *item,void *level,void *entity,void *data){
    void *previous_data=active_data;
    int previous_changed=active_changed;
    int changed;
    active_data=data;
    active_changed=0;
    ((void(*)(void*,void*,void*,void*))sample_hook.trampoline)(item,level,entity,data);
    changed=active_changed;
    active_data=previous_data;
    active_changed=previous_changed;
    if(changed)map_update_batch_notify(data);
}

int map_update_batch_install(void){
    sample_hook.target=0x0072C384u;
    sample_hook.expected[0]=0xE92D4FFFu;
    sample_hook.expected[1]=0xE1A05002u;
    sample_hook.replacement=(u32)on_sample;
    return s->host.install_hook(&sample_hook);
}
