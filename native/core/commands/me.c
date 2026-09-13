#include "../internal.h"
#include "native_command_context.h"
#include "native_property_bag.h"
#include "command_origin_access.h"
#include "command_chat_service.h"
int cmd_me(void*player,void*level,void*game,const char*p){
    const NativeCommandContext*context=native_command_context_current();const char*action;char name[32];
    (void)game;(void)p;if(!context)return fail_syntax();action=native_bag_get_string(context->input_bag,"action");
    if(!action||!action[0])return fail_syntax();
    if(!native_command_origin_name(context->origin,name,sizeof(name)))copy_text(name,"Server",sizeof(name));
    if(!command_chat_emote(level,name,action))return fail_command(command_failed);return 1;
}


