#include "../internal.h"
#include "native_command_context.h"
#include "replace_item_service.h"
#include "block_position_service.h"
#include "native_property_bag.h"

int cmd_replaceitem(void*player,void*level,void*game,const char*p){
    const NativeCommandContext*context=native_command_context_current();const char*slot,*item,*components=0;int position[3],slot_id,amount=1,data=0,maximum_slot=-1;void*targets[2];unsigned count;ReplaceItemResult result;
    (void)player;(void)game;(void)p;if(!context)return fail_syntax();slot=native_bag_get_string(context->input_bag,"slotType");item=native_bag_get_string(context->input_bag,"itemName");
    if(!slot||!item||!native_bag_get_int(context->input_bag,"slotId",&slot_id))return fail_syntax();native_bag_get_int(context->input_bag,"amount",&amount);native_bag_get_int(context->input_bag,"data",&data);
    if(amount<1)return fail_number_too_small(amount,1);if(amount>32767)return fail_number_too_big(amount,32767);if(data<0)return fail_number_too_small(data,0);if(data>32767)return fail_number_too_big(data,32767);if(native_bag_has(context->input_bag,"components"))components=native_bag_get_string(context->input_bag,"components");
    if(!item_from_arg(item)){const char*arguments[1]={item};return fail_command_localized_args("commands.give.item.notFound",arguments,1);}
    if(streq(native_schema_text(context->schema->name),"block")){
        void *source=native_command_context_block_source();
        if(!native_command_context_blockpos("position",position))return fail_syntax();
        result=replace_item_block(source,position,slot,slot_id,item,amount,data,components,&maximum_slot);
        if(result==REPLACE_ITEM_NO_CONTAINER){char formatted[48];const char*arguments[1]={formatted};command_blockpos_to_string(position,formatted,sizeof(formatted));return fail_command_localized_args("commands.replaceitem.noContainer",arguments,1);}
        if(result==REPLACE_ITEM_BAD_SLOT){char requested[16],maximum[16];unsigned length=0;const char*arguments[3]={requested,"0",maximum};requested[0]=0;append_int(requested,&length,slot_id);length=0;maximum[0]=0;append_int(maximum,&length,maximum_slot);return fail_command_localized_args("commands.replaceitem.badSlotNumber",arguments,3);}
        if(result!=REPLACE_ITEM_OK){char requested[16],count_text[16];unsigned length=0;const char*arguments[4]={slot,requested,count_text,strip_ns(item)};requested[0]=0;append_int(requested,&length,slot_id);length=0;count_text[0]=0;append_int(count_text,&length,amount);return fail_command_localized_args("commands.replaceitem.failed",arguments,4);}
        native_command_context_output_string("slotType",slot);native_command_context_output_int("slotId",slot_id);native_command_context_output_int("count",amount);native_command_context_output_string("itemName",strip_ns(item));return 1;
    }
    count=native_command_context_targets("target",targets,SELECTOR_TARGET_CAPACITY);if(!count)return fail_command_localized("commands.generic.entity.notFound");
    {
        unsigned success_count=0,i;ReplaceItemResult last_error=REPLACE_ITEM_OK;
        for(i=0;i<count;i++){
            if(!targets[i])continue;
            result=replace_item_entity(targets[i],slot,slot_id,item,amount,data,components,&maximum_slot);
            if(result==REPLACE_ITEM_OK)success_count++;
            else last_error=result;
        }
        if(!success_count){
            char requested[16],count_text[16],maximum[16];unsigned length=0;
            requested[0]=0;append_int(requested,&length,slot_id);
            if(last_error==REPLACE_ITEM_BAD_SLOT){
                length=0;maximum[0]=0;append_int(maximum,&length,maximum_slot);
                {const char*arguments[3]={requested,"0",maximum};return fail_command_localized_args("commands.replaceitem.badSlotNumber",arguments,3);}
            }
            length=0;count_text[0]=0;append_int(count_text,&length,amount);
            {const char*arguments[4]={slot,requested,count_text,strip_ns(item)};return fail_command_localized_args("commands.replaceitem.failed",arguments,4);}
        }
    }
    native_command_context_output_string("slotType",slot);native_command_context_output_int("slotId",slot_id);native_command_context_output_int("count",amount);native_command_context_output_string("itemName",strip_ns(item));return 1;
}
