#include "map_instance.h"
#include "../../seams.h"
#include "../../util/string_util.h"

static void material_create(u32 *material,void *group,const char *name){
    u32 text;
    make_str(&text,name);
    ((void(*)(void*,void*,const u32*))0x004F149Cu)(material,group,&text);
    drop_str(&text);
}

MapInstance *map_instance_create(void *renderer,void *data){
    MapInstance *instance;
    void *group;
    if(!renderer||!data)return 0;
    instance=((void *(*)(u32))SEAM_operator_new)(sizeof(MapInstance));
    if(!instance)return 0;
    zero(instance,sizeof(*instance));
    instance->data=data;
    instance->texture_group=*(void**)((u8*)renderer+0xa8);
    instance->icons=(u8*)renderer+0xc8;
    instance->material=(void*)((u8*)renderer+0x64);
    instance->foreground=(u8*)renderer+0x170;
    instance->font=*(void**)((u8*)renderer+0x814);
    if(!instance->texture_group||!instance->material||
       (u32)instance->texture_group<0x00100000u||(u32)instance->material<0x00100000u){
        ((void(*)(void*))SEAM_operator_delete)(instance);
        return 0;
    }
    group=*(void**)((u8*)renderer+0x28);
    if(!group||(u32)group<0x00100000u){
        ((void(*)(void*))SEAM_operator_delete)(instance);
        return 0;
    }
    material_create(instance->marker_material,group,"entity_alphatest");
    material_create(instance->label_material,*(void**)((u8*)renderer+0x40),"name_tag_depth_tested");
    if(!map_instance_resource(instance->resource,data)){
        ((void(*)(void*))SEAM_operator_delete)(instance);
        return 0;
    }
    ((void*(*)(void*,void*,int,int))0x004F4038u)(instance->texture_group,instance->resource,128,128);
    ((void*(*)(void*,void*,void*,int))0x004F06F4u)(instance->texture,instance->texture_group,instance->resource,0);
    instance->dirty=1;
    return instance;
}
