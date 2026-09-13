#include "../internal.h"
#include "native_command_context.h"
#include "command_packets.h"
#include "native_property_bag.h"
int cmd_title(void*player,void*level,void*game,const char*p){
    const NativeCommandContext*context=native_command_context_current();const char*op,*text;
    void*tv[SELECTOR_TARGET_CAPACITY];unsigned nt,i;
    (void)player;(void)level;(void)game;(void)p;if(!context)return fail_syntax();op=native_schema_text(context->schema->name);nt=native_command_context_targets("player",tv,SELECTOR_TARGET_CAPACITY);if(!nt)return fail_command_localized("commands.generic.player.notFound");
    if(streq(op,"times")){
        int fade_in,stay,fade_out;
        if(!native_bag_get_int(context->input_bag,"fadeIn",&fade_in)||!native_bag_get_int(context->input_bag,"stay",&stay)||!native_bag_get_int(context->input_bag,"fadeOut",&fade_out))return fail_syntax();
        for(i=0;i<nt;i++)if(!command_packet_set_title(tv[i],op,0,fade_in,stay,fade_out))return fail_command(command_failed);
        return 1;
    }
    text=native_bag_get_string(context->input_bag,"titleText");if(!text)text="";
    for(i=0;i<nt;i++){
        if(!command_packet_set_title(tv[i],op,text,0,0,0))return fail_command(command_failed);
    }
    return 1;
}


