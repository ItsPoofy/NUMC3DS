#include "map_preview_texture.h"
#include "../internal.h"
#include "../item/map_item.h"

static struct {
    u32 resource[5],reference[8];
    void *group,*texture,*data,*pixels;
    int dirty;
} preview;

void map_preview_texture_reset(void){zero(&preview,sizeof(preview));}

void map_preview_texture_dirty(void *data){
    if(data==preview.data)preview.dirty=1;
}

int map_preview_texture_begin(MapPreviewTexture *binding,void *game,void *sprite,void *data){
    void *group,*pixels;
    binding->sprite=0;
    if(!game||!sprite)return 0;
    group=*(void**)((u8*)game+0x58);if(!group)return 0;
    if(preview.group!=group)map_preview_texture_reset();
    if(!preview.texture){
        ((ResourceLocationCtor)SEAM_ResourceLocation_ctor)(preview.resource,"numc3ds/held_map_preview",1);
        preview.texture=((void*(*)(void*,void*))0x004F3F88u)(group,preview.resource);
        if(!preview.texture)
            preview.texture=((void*(*)(void*,void*,int,int))0x004F4038u)(group,preview.resource,128,128);
        if(!preview.texture)return 0;
        preview.group=group;
        ((TexturePathStateInit)SEAM_TexturePathStateInit)(preview.reference,group,preview.resource,0);
        preview.dirty=1;
    }
    pixels=data?*(void**)((u8*)data+0x20):0;
    if(preview.data!=data||preview.pixels!=pixels)preview.dirty=1;
    preview.data=data;preview.pixels=pixels;
    if(preview.dirty){
        void *image=((void*(*)(void*))0x001B340Cu)(preview.texture);
        if(data&&!pixels)return 0;
        if(!image||(u32)image<0x00100000u||!*(void**)image)return 0;
        if((u32)((void*(*)(void*,int,int))0x004F0C44u)(image,0,0)<0x00100000u)return 0;
        if(data)map_copy_pixels_to_texture(preview.texture,data);else map_fill_blank_texture(preview.texture);
        preview.dirty=0;
    }
    cp(binding->saved,(u8*)sprite+0x10,sizeof(binding->saved));
    cp((u8*)sprite+0x10,preview.reference,sizeof(preview.reference));
    binding->sprite=sprite;
    return 1;
}

void map_preview_texture_end(MapPreviewTexture *binding){
    if(!binding->sprite)return;
    cp(preview.reference,(u8*)binding->sprite+0x10,sizeof(preview.reference));
    cp((u8*)binding->sprite+0x10,binding->saved,sizeof(binding->saved));
    binding->sprite=0;
}
