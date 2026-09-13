#include "../internal.h"
#include "native_command_context.h"
#include "native_property_bag.h"
#include "enchant_registry.h"
int cmd_enchant(void*player,void*level,void*game,const char*p){
    const NativeCommandContext*context=native_command_context_current();
    const char*name;int enchantment,level_value=1,max_level,result;void*targets[SELECTOR_TARGET_CAPACITY],*success[SELECTOR_TARGET_CAPACITY],*no_item[SELECTOR_TARGET_CAPACITY],*failed[SELECTOR_TARGET_CAPACITY];unsigned count,index,success_count=0,no_item_count=0,failed_count=0;
    (void)player;(void)level;(void)game;(void)p;
    if(!context)return fail_syntax();
    count=native_command_context_targets("player",targets,SELECTOR_TARGET_CAPACITY);if(!count)return fail_command_localized("commands.generic.noTargetMatch");
    name=native_bag_get_string(context->input_bag,"enchantmentName");
    if(name)enchantment=native_enchant_id_from_name(name);
    else if(!native_bag_get_int(context->input_bag,"enchantmentId",&enchantment))return fail_syntax();
    if(enchantment<0||enchantment>26){char value[16];unsigned length=0;const char*arguments[1]={value};value[0]=0;append_int(value,&length,enchantment);return fail_command_localized_args("commands.enchant.notFound",arguments,1);}
    max_level=enchant_max_level(enchantment);if(max_level<1)return fail_command_localized("commands.enchant.cantEnchant");
    native_bag_get_int(context->input_bag,"level",&level_value);
    if(level_value<1)return fail_number_too_small(level_value,1);
    if(level_value>max_level)return fail_number_too_big(level_value,max_level);
    for(index=0;index<count;index++){
        result=apply_enchant(level,targets[index],enchantment,level_value);
        if(result==ENCHANT_APPLY_NO_ITEM)no_item[no_item_count++]=targets[index];
        else if(result==ENCHANT_APPLY_CANT_COMBINE)failed[failed_count++]=targets[index];
        else if(result==ENCHANT_APPLY_CANT_ENCHANT||result==ENCHANT_APPLY_INTERNAL)failed[failed_count++]=targets[index];
        else success[success_count++]=targets[index];
    }
    native_command_context_output_entities("playerNames",success,success_count);native_command_context_output_entities("noItemNames",no_item,no_item_count);native_command_context_output_entities("failedNames",failed,failed_count);
    return 1;
}


