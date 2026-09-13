#include "map_instance.h"
#include "../../util/string_util.h"

typedef void *(*ResourceLocationCtorFn)(void *,const char *,int);
typedef void *(*GstdStringCopyCtorFn)(void *,void *);
typedef void *(*GstdStringConcatFn)(void *,const char *,void *);
typedef void (*GstdStringDtorFn)(void *);

static int map_resource_build(u32 resource[5],void *data){
    char suuid[32],comb[32];
    const char *comb_str;
    if(!resource||!data)return 0;
    zero(suuid,sizeof(suuid));
    ((GstdStringCopyCtorFn)0x002FF261u)(suuid,(u8*)data+8);
    zero(comb,sizeof(comb));
    ((GstdStringConcatFn)0x00917B28u)(comb,(const char*)0x00677684u,suuid);
    comb_str=*(const char**)comb;
    if(!comb_str){
        ((GstdStringDtorFn)0x002FEBBDu)(comb);
        ((GstdStringDtorFn)0x002FEBBDu)(suuid);
        return 0;
    }
    zero(resource,5*sizeof(u32));
    ((ResourceLocationCtorFn)0x0033C630u)(resource,comb_str,1);
    ((GstdStringDtorFn)0x002FEBBDu)(comb);
    ((GstdStringDtorFn)0x002FEBBDu)(suuid);
    return 1;
}

int map_instance_resource(u32 resource[5],void *data){
    return map_resource_build(resource,data);
}

int map_instance_update_texture(MapInstance *instance){
    u32 *pixels;
    void *texture,*image;
    u32 *destination,i,color;
    int changed=0;
    if(!instance||!instance->data||!instance->texture_group)return 0;
    pixels=*(u32**)((u8*)instance->data+0x20);
    if(!pixels)return 0;
    texture=((void*(*)(void*,void*))0x004F3F88u)(instance->texture_group,instance->resource);
    if(!texture||texture==instance->texture_group||(u32)texture<0x00100000u)
    texture=((void*(*)(void*,void*,int,int))0x004F4038u)(instance->texture_group,instance->resource,128,128);
    if(!texture||texture==instance->texture_group||(u32)texture<0x00100000u)return 0;
    image=((void*(*)(void*))0x001B340Cu)(texture);
    if(!image||(u32)image<0x00100000u||!*(void**)image)return 0;
    destination=((u32*(*)(void*,int,int))0x004F0C44u)(image,0,0);
    if(!destination||(u32)destination<0x00100000u)return 0;
    for(i=0;i<16384;i++){
        color=pixels[i];
        if(!color)color=0x10000000u|(((i+(i>>7))&1u)<<27);
        if(destination[i]!=color){destination[i]=color;changed=1;}
    }
    if(changed)((void(*)(void*))0x001B34B4u)(texture);
    instance->dirty=0;
    return 1;
}
