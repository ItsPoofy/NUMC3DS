#include "../internal.h"
#include "native_command_context.h"
#include "native_property_bag.h"
#include "command_origin_access.h"
#include "command_chat_service.h"
int cmd_say(void*player,void*level,void*game,const char*p){
    const NativeCommandContext*context=native_command_context_current();const char*message;char sender[48],expanded[MAX_TEXT+1];
    (void)game;(void)p;if(!context)return fail_syntax();message=native_bag_get_string(context->input_bag,"message");
    if(!message||!message[0])return fail_syntax();
    if(!native_command_origin_name(context->origin,sender,sizeof(sender)))copy_text(sender,"Server",sizeof(sender));
    if(!command_chat_expand_target_names(context->origin,context->input_bag,message,expanded,sizeof(expanded)))return fail_command(command_failed);
    if(!command_chat_announcement(level,sender,expanded))return fail_command(command_failed);
    native_command_context_output_string("message",message);
    return 1;
}


