#include "../internal.h"
#include "entity_teleport.h"

static int teleport(void *entity,const float position[3],int item){
    void **vtable;
    EntityTeleport teleport;
    if(!entity||!position)return 0;
    vtable=*(void ***)entity;
    if(!vtable)return 0;
    teleport=(EntityTeleport)vtable[SEAM_Entity_teleportVtableOffset/sizeof(*vtable)];
    if(!teleport)return 0;
    teleport(entity,position,3,item);
    return 1;
}

int native_entity_teleport(void *entity,const float position[3]){return teleport(entity,position,0);}

int native_entity_teleport_to_coordinates(void *entity,const int position[3]){
    float target[3];
    if(!position)return 0;
    target[0]=(float)position[0]+0.5f;target[1]=(float)position[1];target[2]=(float)position[2]+0.5f;
    return teleport(entity,target,1);
}

unsigned native_entity_teleport_multiple(void **entities,unsigned count,const float position[3],void **successful_out){
    unsigned index,success=0;
    if(!entities||!count||!position)return 0;
    for(index=0;index<count;index++){
        if(entities[index]&&native_entity_teleport(entities[index],position)){
            if(successful_out)successful_out[success]=entities[index];
            success++;
        }
    }
    return success;
}

unsigned native_entity_teleport_multiple_to_coordinates(void **entities,unsigned count,const int position[3],void **successful_out){
    unsigned index,success=0;
    if(!entities||!count||!position)return 0;
    for(index=0;index<count;index++){
        if(entities[index]&&native_entity_teleport_to_coordinates(entities[index],position)){
            if(successful_out)successful_out[success]=entities[index];
            success++;
        }
    }
    return success;
}
