#include "../internal.h"
#include "native_command_context.h"
#include "native_property_bag.h"
#include "command_packets.h"

int cmd_difficulty(void*player,void*level,void*game,const char*p){
    const NativeCommandContext*context=native_command_context_current();const char*sub;int gm;void**vtable;void*options;
    const char*diff_name;
    (void)player;(void)p;
    if(!context||!level||!game)return fail_syntax();sub=native_bag_get_string(context->input_bag,"difficulty");
    if(sub){if(streq(sub,"peaceful")||streq(sub,"p"))gm=0;else if(streq(sub,"easy")||streq(sub,"e"))gm=1;else if(streq(sub,"normal")||streq(sub,"n"))gm=2;else if(streq(sub,"hard")||streq(sub,"h"))gm=3;else return fail_syntax();}
    else if(!native_bag_get_int(context->input_bag,"difficulty",&gm)||gm<0||gm>3)return fail_syntax();
    vtable=*(void***)level;
    options=((MinecraftGameGetOptionsFn)SEAM_MinecraftGame_getOptions)(game);
    if(!vtable||!options)return fail_command(command_failed);
    ((LevelSetInt)vtable[SEAM_Level_setDifficultyVtableOffset/4])(level,gm);
    ((OptionsSetIntFn)SEAM_Options_setInt)(options,(const void*)SEAM_Options_difficulty,gm);
    command_packet_broadcast_difficulty(level,gm);
    diff_name=gm==0?"Peaceful":gm==1?"Easy":gm==2?"Normal":"Hard";
    native_command_context_output_string("difficulty",diff_name);
    return 1;
}


