#include "map_instance.h"
#include "map_transform.h"

typedef void(__attribute__((pcs("aapcs-vfp")))*VertexFn)(void*,float,float,float,float,float);

static void draw_foreground(MapInstance *instance){
    void *tess=(void*)0x00AC42D8u;
    VertexFn vertex=(VertexFn)0x001B300Cu;
    if(!((int(*)(void*))0x006AC024u)(tess)){
        ((void(*)(void*,void*,void*,int,int))0x00717658u)(instance->foreground,instance->material,instance->texture,0,0);
        return;
    }
    ((void(*)(void*,unsigned))0x001B1BF0u)(tess,0);
    ((void(*)(void*,const void*))0x001B2300u)(tess,(void*)0x00B0E320u);
    vertex(tess,0,128,-0.01f,0,1);
    vertex(tess,128,128,-0.01f,1,1);
    vertex(tess,128,0,-0.01f,1,0);
    vertex(tess,0,0,-0.01f,0,0);
    ((void(*)(void*,void*,void*))0x001B1E68u)(tess,instance->material,instance->texture);
}

void map_instance_draw(MapInstance *instance,const Color *light,int in_frame,int hide_markers){
    MapTransform transform;
    map_transform_push(&transform);
    map_transform_translate(&transform,0,0,-0.02f);
    draw_foreground(instance);
    if(!hide_markers)map_instance_draw_decorations(instance,light,in_frame);
    map_transform_pop(&transform);
}
