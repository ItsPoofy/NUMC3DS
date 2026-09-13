#ifndef NUMC3DS_MAP_RENDERER_H
#define NUMC3DS_MAP_RENDERER_H
#include "map_instance.h"

void *map_renderer_get(void);
MapInstance *map_renderer_get_instance(void *renderer,void *data);
void map_renderer_update(void *data);
void map_renderer_render(void *renderer,const int *position,void *source,void *data,float partial,int in_frame,int hide_markers);
int map_renderer_install(void);
#endif
