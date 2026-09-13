#include "ui_sprite.h"

enum { TEXTURE_IMAGE_BUFFER_OFFSET=0x40,TEXTURE_LOCATION_OFFSET=0x84 };

static int ui_sprite_texture_loaded(void*texture){
    void*entries;
    if(!texture)return 0;
    entries=*(void**)((u8*)texture+TEXTURE_IMAGE_BUFFER_OFFSET);
    return entries&&((u32*)entries)[2]>0;
}

typedef void(*TextureLazyLoadFn)(void*location,void*container);

static int ui_sprite_texture_ready(void*sprite){
    void*texture;
    if(!sprite)return 0;
    texture=((TexturePtrDerefFn)SEAM_TexturePtr_deref)((u8*)sprite+0x10);
    return ui_sprite_texture_loaded(texture);
}

void *ui_sprite_create(void*screen,const char*resource,int x,int y,int width,int height,int source_x,int source_y,int source_width,int source_height){
    u32 location[5];void*memory,*sprite,*texture;
    if(!ui_resources_ready()||!screen||!resource)return 0;
    zero(location,sizeof(location));
    ((ResourceLocationCtor)SEAM_ResourceLocation_ctor)(location,resource,0);
    memory=((GameAllocWithSelector)SEAM_Heap_allocWithSelector)(0xA8,(void*)SEAM_game_alloc_selector);
    if(!memory)return 0;
    sprite=((SpriteCtorFn)SEAM_Sprite_ctor)(memory,ui_screen_game(screen),x,y,width,height,location,source_x,source_y,source_width,source_height);
    if(!sprite)((void(*)(void*))SEAM_operator_delete)(memory);
    else{
        texture=((TexturePtrDerefFn)SEAM_TexturePtr_deref)((u8*)sprite+0x10);
        if(texture&&!ui_sprite_texture_loaded(texture)){
            ((TextureLazyLoadFn)SEAM_TextureLazyLoad)((u8*)texture+TEXTURE_LOCATION_OFFSET,(u8*)texture+TEXTURE_IMAGE_BUFFER_OFFSET);
        }
        if(!ui_sprite_texture_loaded(texture)){ui_sprite_destroy(sprite);sprite=0;}
    }
    return sprite;
}

void ui_sprite_draw(void*sprite){if(ui_sprite_texture_ready(sprite))((void(*)(void*))SEAM_Sprite_render)(sprite);}

void ui_sprite_destroy(void*sprite){
    if(!sprite)return;
    ((MeshDtorFn)SEAM_Mesh_dtor)((u8*)sprite+0x30);
    ((TexturePtrDtor)SEAM_TexturePtrDtor)((u8*)sprite+0x10);
    ((void(*)(void*))SEAM_MaterialPtr_dtor)((u8*)sprite+4);
    ((void(*)(void*))SEAM_operator_delete)(sprite);
}
