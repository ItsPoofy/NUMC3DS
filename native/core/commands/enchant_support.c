#include "../internal.h"
#include "../seams.h"
#include "inventory_action_service.h"

typedef void* (*SelectedItemFn)(void*);

static void* enchant_by_id(int id){
    void**table;
    if(id<0||id>=27)return 0;
    table=*(void***)SEAM_Enchant_registry;
    return table?table[id]:0;
}

int enchant_max_level(int id){
    void*enchant=enchant_by_id(id);
    return enchant?((int(*)(void*))(*(void***)enchant)[5])(enchant):0;
}

int apply_enchant(void*level,void*target,int eid,int enchantment_level){
    void*enchant=enchant_by_id(eid);void*item;
    if(!enchant)return ENCHANT_APPLY_INTERNAL;
    item=((SelectedItemFn)SEAM_Player_getMainhandItem)(target);
    if(!item||((IsNullFn)SEAM_ItemInstance_isNull)(item))return ENCHANT_APPLY_NO_ITEM;
    return command_inventory_action_enchant(level,target,item,eid,enchantment_level)?ENCHANT_APPLY_SUCCESS:ENCHANT_APPLY_CANT_ENCHANT;
}
