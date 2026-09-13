#include "../internal.h"
#include "native_command_context.h"
#include "command_packets.h"
#include "native_property_bag.h"

int cmd_stopsound(void*player,void*level,void*game,const char*p){
    const NativeCommandContext*context=native_command_context_current();const char*sound;void*targets[EXEC_TARGET_CAPACITY];
    unsigned count=0,i;int all;
    (void)player;(void)level;(void)game;(void)p;if(!context)return fail_syntax();count=native_command_context_targets("player",targets,EXEC_TARGET_CAPACITY);if(!count)return fail_command_localized("commands.generic.player.notFound");sound=native_bag_get_string(context->input_bag,"sound");if(!sound)sound="";
    all=!sound[0];
    for(i=0;i<count;i++)if(!command_packet_stop_sound(targets[i],sound)){
        return fail_command(command_failed);
    }
    native_command_context_output_string("sound",sound);native_command_context_output_entities("player",targets,count);native_command_context_output_bool("one_sound",!all);native_command_context_output_bool("all_sounds",all);
    return 1;
}
