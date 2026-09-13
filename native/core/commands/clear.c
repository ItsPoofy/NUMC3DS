#include "../internal.h"
#include "native_command_context.h"
#include "native_property_bag.h"
#include "inventory_mutation_service.h"
int cmd_clear(void*player,void*level,void*game,const char*p){
    const NativeCommandContext*context=native_command_context_current();
    const char*name;int data=-1,max_count=-1,has_filter=0,removed=0;
    void*targets[SELECTOR_TARGET_CAPACITY];void*item=0;
    char player_name[40],removed_value[16],test_value[16];
    const char*removed_result[1]={removed_value},*test_result[1]={test_value};
    unsigned count,length=0;
    (void)level;(void)game;(void)p;
    if(!context)return fail_syntax();
    if(native_command_context_has_target("player")){
        count=native_command_context_targets("player",targets,SELECTOR_TARGET_CAPACITY);
        if(!count)return fail_command_localized("commands.generic.player.notFound");
    }else{
        if(!player)return fail_command_localized("commands.generic.player.notFound");
        targets[0]=player;count=1;
    }
    name=native_bag_get_string(context->input_bag,"itemName");
    if(name){
        item=item_from_arg(name);if(!item){const char*arguments[1]={name};return fail_command_localized_args("commands.give.item.notFound",arguments,1);}
        has_filter=1;
        native_bag_get_int(context->input_bag,"data",&data);native_bag_get_int(context->input_bag,"maxCount",&max_count);
        if(data<-1)return fail_number_too_small(data,-1);
        if(max_count<-1)return fail_number_too_small(max_count,-1);
    }
    if(command_inventory_clear_players(targets,count,item,data,max_count,&removed)<0)return fail_command_localized("commands.generic.player.notFound");
    entity_name(targets[0],player_name,sizeof(player_name));
    if(has_filter&&max_count==0){test_value[0]=0;append_int(test_value,&length,removed);native_command_context_output_strings("playerTest",test_result,1);native_command_context_output_entities("player",targets,count);return 1;}
    if(removed<1){const char*arguments[1]={player_name};return fail_command_localized_args("commands.clear.failure.no.items",arguments,1);}
    removed_value[0]=0;append_int(removed_value,&length,removed);
    native_command_context_output_entities("player",targets,count);
    native_command_context_output_strings("itemsRemoved",removed_result,1);
    return 1;
}


