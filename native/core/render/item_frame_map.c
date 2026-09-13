#include "item_frame_map.h"
#include "map/map_renderer.h"
#include "map/map_transform.h"
#include "../item/map_item.h"
#include "../state.h"

typedef void(__attribute__((pcs("aapcs-vfp")))*DrawItemFn)(void*,void*,void*,float);
typedef void(__attribute__((pcs("aapcs-vfp")))*DrawFrameFn)(void*,void*,void*,int,float);
static NuMC3DS_Hook item_hook;
static NuMC3DS_Hook frame_hook;

static int has_map(void *frame){
    return ((int(*)(void*))0x006B2924u)((u8*)frame+0x78)==358;
}

static void __attribute__((pcs("aapcs-vfp"))) on_draw_frame(void *renderer,void *source,void *frame,int map_frame,float partial){
    ((DrawFrameFn)frame_hook.trampoline)(renderer,source,frame,map_frame||has_map(frame),partial);
}

static void __attribute__((pcs("aapcs-vfp"))) on_draw_item(void *renderer,void *source,void *frame,float partial){
    void *level,*data,*map_renderer;
    MapTransform transform;
    unsigned rotation;
    if(!has_map(frame)){
        ((DrawItemFn)item_hook.trampoline)(renderer,source,frame,partial);
        return;
    }
    level=((void*(*)(void*))0x0069DF9Cu)(source);
    data=map_savedata_for(level,(u8*)frame+0x78);
    if(!data){
        ((DrawItemFn)item_hook.trampoline)(renderer,source,frame,partial);
        return;
    }
    map_renderer=map_renderer_get();
    if(!map_renderer){
        ((DrawItemFn)item_hook.trampoline)(renderer,source,frame,partial);
        return;
    }
    rotation=*(u8*)((u8*)frame+0xac);
    map_transform_push(&transform);
    map_transform_rotate(&transform,(float)(rotation%4)*-90.0f,0,0,1);
    map_transform_translate(&transform,0,0,0.5625f);
    map_transform_rotate(&transform,180.0f,0,0,1);
    map_transform_rotate(&transform,180.0f,0,1,0);
    map_transform_scale(&transform,0.0078125f);
    map_transform_translate(&transform,-64,-64,0);
    map_renderer_render(map_renderer,(const int*)((u8*)frame+0x20),source,data,partial,1,!is_locator_map((u8*)frame+0x78));
    map_transform_pop(&transform);
}

int item_frame_map_install(void){
    int result;
    zero(&item_hook,sizeof(item_hook));
    item_hook.target=0x00360AD4u;
    item_hook.expected[0]=0xE92D4FF7u;
    item_hook.expected[1]=0xE1A09001u;
    item_hook.replacement=(u32)on_draw_item;
    result=s->host.install_hook(&item_hook);
    if(result)return result;
    zero(&frame_hook,sizeof(frame_hook));
    frame_hook.target=0x0036127Cu;
    frame_hook.expected[0]=0xE92D40F0u;
    frame_hook.expected[1]=0xE1A04000u;
    frame_hook.replacement=(u32)on_draw_frame;
    result=s->host.install_hook(&frame_hook);
    if(result)return result;
    return map_renderer_install();
}
