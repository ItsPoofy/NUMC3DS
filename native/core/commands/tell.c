#include "../internal.h"
#include "native_command_context.h"
#include "native_property_bag.h"
#include "command_origin_access.h"
#include "entity_message_service.h"
int cmd_tell(void*player,void*level,void*game,const char*p){
    const NativeCommandContext*context=native_command_context_current();char sender[32];
    void*targets[SELECTOR_TARGET_CAPACITY],*successful[SELECTOR_TARGET_CAPACITY];const char*message;unsigned count,success_count=0,i;
    (void)player;(void)level;(void)game;(void)p;if(!context)return fail_syntax();
    count=native_command_context_targets("recipient",targets,SELECTOR_TARGET_CAPACITY);
    if(!count)return fail_command_localized("commands.generic.noTargetMatch");
    message=native_bag_get_string(context->input_bag,"message");if(!message||!message[0])return fail_syntax();
    if(!native_command_origin_name(context->origin,sender,sizeof(sender)))copy_text(sender,"Server",sizeof(sender));
    for(i=0;i<count;i++){
        if(targets[i]&&command_entity_display_message(targets[i],sender,message))successful[success_count++]=targets[i];
    }
    if(!success_count)return fail_command(command_failed);
    native_command_context_output_entities("recipient",successful,success_count);native_command_context_output_string("message",message);
    return 1;
}


