#include "map_instance.h"
#include "map_transform.h"

typedef void(__attribute__((pcs("aapcs-vfp")))*VertexFn)(void*,float,float,float,float,float);

static int rendered_on_frame(unsigned type){
    unsigned bit=(type-1u)&255u;
    return bit<=14u&&((0x6001u>>bit)&1u);
}

static void decoration_draw(MapInstance *instance,const u8 *decoration,const Color *light,unsigned index){
    MapTransform transform;
    void *tess=(void*)0x00AC42D8u;
    const Color *tint=(const Color*)(decoration+8);
    Color color={light->r*tint->r,light->g*tint->g,light->b*tint->b,light->a*tint->a};
    unsigned type=decoration[4];
    float u=(float)(type%4)*0.25f,v=(float)(type/4)*0.25f;
    VertexFn vertex=(VertexFn)0x001B300Cu;
    map_transform_push(&transform);
    map_transform_translate(&transform,(float)(signed char)decoration[5]*0.5f+64.0f,
        (float)(signed char)decoration[6]*0.5f+64.0f,(float)index*-0.02f);
    map_transform_rotate(&transform,(float)(signed char)decoration[7]*22.5f,0,0,1);
    map_transform_scale(&transform,4.0f);
    map_transform_translate(&transform,-0.125f,0.125f,0);
    ((void(*)(void*,unsigned))0x001B1BF0u)(tess,4);
    ((void(*)(void*,const void*))0x001B2300u)(tess,(void*)0x00B0E320u);
    ((void(*)(void*,const Color*))0x001B1F00u)(tess,&color);
    vertex(tess,-1,1,0,u,v);
    vertex(tess,1,1,0,u+0.25f,v);
    vertex(tess,1,-1,0,u+0.25f,v+0.25f);
    vertex(tess,-1,-1,0,u,v+0.25f);
    if(((int(*)(void*))0x006AC024u)(tess))
        ((void(*)(void*,void*,void*))0x001B1E68u)(tess,instance->marker_material,instance->icons);
    else
        ((void(*)(void*,void*,void*))0x001B1984u)(tess,instance->marker_material,instance->icons);
    map_transform_pop(&transform);
}

void map_instance_draw_decorations(MapInstance *instance,const Color *light,int in_frame){
    u8 *entry=*(u8**)((u8*)instance->data+0x3c);
    u8 *end=*(u8**)((u8*)instance->data+0x40);
    unsigned index=1;
    for(;entry!=end;entry+=0x28){
        void *control=*(void**)(entry+0x24);
        const u8 *decoration=*(const u8**)(entry+0x20);
        if(!control||!decoration||!((int(*)(void*))0x00119D4Cu)(control))continue;
        if(in_frame&&!rendered_on_frame(decoration[4]))continue;
        decoration_draw(instance,decoration,light,index++);
    }
}
