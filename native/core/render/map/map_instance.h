#ifndef NUMC3DS_MAP_INSTANCE_H
#define NUMC3DS_MAP_INSTANCE_H
#include "../../rt.h"

typedef struct {
    void *data;
    void *texture_group;
    u8 dirty, reserved[3];
    u32 texture[8];
    void *icons;
    void *material;
    u32 marker_material[3];
    u32 label_material[3];
    void *foreground;
    void *font;
    u32 resource[5];
} MapInstance;

_Static_assert(sizeof(MapInstance)==0x68,"MapInstance native size");
_Static_assert(__builtin_offsetof(MapInstance,resource)==0x54,"MapInstance resource offset");

MapInstance *map_instance_create(void *renderer,void *data);
int map_instance_update_texture(MapInstance *instance);
void map_instance_draw(MapInstance *instance,const Color *light,int in_frame,int hide_markers);
void map_instance_draw_decorations(MapInstance *instance,const Color *light,int in_frame);
int map_instance_resource(u32 resource[5],void *data);
#endif
