#include "map_renderer.h"

void *map_renderer_get(void){
    void *dispatcher=((void*(*)(void))0x00444CA0u)();
    return dispatcher?((void*(*)(void*,unsigned))0x00444CB0u)(dispatcher,0x3b):0;
}

MapInstance *map_renderer_get_instance(void *renderer,void *data){
    MapInstance **slot=((MapInstance**(*)(void*,void*))0x008DFAA4u)((u8*)renderer+0xac,(u8*)data+8);
    if(!*slot)*slot=map_instance_create(renderer,data);
    return *slot;
}

static MapInstance *map_renderer_find_instance(void *renderer,void *data){
    u8 *registry;
    u8 *header;
    u8 *node;
    u8 *candidate;
    void *key;
    int compare;

    if(!renderer||!data)return 0;
    registry=(u8*)renderer+0xac;
    header=*(u8**)(registry+0x10);
    if(!header)return 0;
    node=*(u8**)(header+4);
    candidate=header;
    key=(u8*)data+8;
    while(node){
        compare=((int(*)(void*,void*))0x00137850u)(node+0x10,key);
        if(compare<0)
            node=*(u8**)(node+0xc);
        else{
            candidate=node;
            node=*(u8**)(node+8);
        }
    }
    if(candidate==header)return 0;
    if(((int(*)(void*,void*))0x00137850u)(key,candidate+0x10)<0)return 0;
    return *(MapInstance**)(candidate+0x14);
}

void map_renderer_update(void *data){
    void *renderer=map_renderer_get();
    MapInstance *instance;
    if(!renderer||!data)return;
    instance=map_renderer_find_instance(renderer,data);
    if(instance)instance->dirty=1;
}
