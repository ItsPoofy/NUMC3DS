#include "../internal.h"
#include "native_command_context.h"
#include "item_components.h"
#include "item_validation_service.h"
#include "inventory_action_service.h"
#include "native_property_bag.h"
int cmd_give(void*player,void*level,void*game,const char*p){
    void map_log3(char,unsigned,unsigned);
    const NativeCommandContext*context=native_command_context_current();const char*name,*components=0;int count=1,data=0;void*item;numc3ds_u32 inst[48];void*tv[SELECTOR_TARGET_CAPACITY];unsigned nt,i;
    (void)level;(void)game;(void)p;if(!context)return fail_syntax();
    nt=native_command_context_targets("player",tv,SELECTOR_TARGET_CAPACITY);
    name=native_bag_get_string(context->input_bag,"itemName");
    if(!nt)return fail_command_localized("commands.generic.player.notFound");
    if(!name)return fail_syntax();
    item=item_from_arg(name);
    map_log3('G', (unsigned)item, nt ? (unsigned)tv[0] : 0);
    map_log3('g', nt, (unsigned)player);
    if(!item){const char*arguments[1]={name};return fail_command_localized_args("commands.give.item.notFound",arguments,1);}
    native_bag_get_int(context->input_bag,"amount",&count);if(count<1)return fail_number_too_small(count,1);if(count>32767)return fail_number_too_big(count,32767);
    if(native_bag_has(context->input_bag,"data"))native_bag_get_int(context->input_bag,"data",&data);
    else if(streq(strip_ns(name),"locator_map")||streq(strip_ns(name),"empty_locator_map"))data=2;
    if(native_bag_has(context->input_bag,"components"))components=native_bag_get_string(context->input_bag,"components");
    {
        CommandItemValidation validation;
        zero(inst,sizeof(inst));
        ((ItemCountAuxCtorFn)SEAM_ItemInstance_itemCountAuxCtor)(inst,item,1,data);
        validation=command_item_validate_instance(inst);
        ((ItemInstanceDtorFn)SEAM_ItemInstance_dtor)(inst);
        if(validation!=COMMAND_ITEM_VALID){
            const char*arguments[1]={name};
            return fail_command_localized_args(validation==COMMAND_ITEM_NOT_FOUND?"commands.give.item.notFound":"commands.give.item.invalid",arguments,1);
        }
    }
    {int output_amount=0;for(i=0;i<nt;i++){
        int maxStack,remaining=count;
        if(!tv[i])continue;
        zero(inst,sizeof(inst));
        ((ItemCountAuxCtorFn)SEAM_ItemInstance_itemCountAuxCtor)(inst,item,1,data);
        if(!command_item_components_apply(inst,components)){((ItemInstanceDtorFn)SEAM_ItemInstance_dtor)(inst);return fail_command_localized("commands.generic.componentError");}
        maxStack=((ItemInstanceGetMaxStackFn)SEAM_ItemInstance_getMaxStackSize)(inst);
        ((ItemInstanceDtorFn)SEAM_ItemInstance_dtor)(inst);
        if(maxStack<1)maxStack=1;
        if(maxStack>255)maxStack=255;
        while(remaining>0){
            int chunk=remaining<maxStack?remaining:maxStack;
            zero(inst,sizeof(inst));
            ((ItemCountAuxCtorFn)SEAM_ItemInstance_itemCountAuxCtor)(inst,item,chunk,data);
            if(!command_item_components_apply(inst,components)){((ItemInstanceDtorFn)SEAM_ItemInstance_dtor)(inst);return fail_command_localized("commands.generic.componentError");}
            if(!command_inventory_action_give(level,tv[i],inst)){((ItemInstanceDtorFn)SEAM_ItemInstance_dtor)(inst);return fail_command(command_failed);}
            ((ItemInstanceDtorFn)SEAM_ItemInstance_dtor)(inst);
            remaining-=chunk;
        }
        output_amount=count;
    }native_command_context_output_string("itemName",strip_ns(name));native_command_context_output_int("itemAmount",output_amount);native_command_context_output_entities("playerName",tv,nt);}
    return 1;
}


