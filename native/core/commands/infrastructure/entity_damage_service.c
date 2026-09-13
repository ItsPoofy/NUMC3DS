#include "../entity_damage_service.h"
#include "../entity_classification.h"
#include "../../internal.h"

int command_entity_kill(void *entity){
    numc3ds_u32 source[2];
    if(!entity)return 0;
    ((void(*)(void*,int))SEAM_DamageSource_ctor)(source,13);
    ((void(*)(void*,const void*,int,int,int))SEAM_Entity_hurt)(entity,source,0x7fff,1,0);
    if(!command_entity_is_player(entity) && !command_entity_is_mob(entity)){
        if(command_entity_is_alive(entity)){
            ((void(*)(void*))0x005F6828u)(entity);
        }
    }
    return 1;
}

unsigned command_entity_kill_multiple(void **entities, unsigned count, void **killed_out){
    unsigned index, killed = 0;
    if(!entities || !count) return 0;
    for(index = 0; index < count; index++){
        if(entities[index] && command_entity_kill(entities[index])){
            if(killed_out) killed_out[killed] = entities[index];
            killed++;
        }
    }
    return killed;
}

int command_entity_is_alive(const void *entity){
    return entity?((int(*)(const void *))(*(void *const *const *)entity)[0x130/4])(entity):0;
}
