#include "../internal.h"
#include "native_command_context.h"
#include "native_property_bag.h"
#include "command_packets.h"
int cmd_daylock(void*player,void*level,void*game,const char*p){
    const NativeCommandContext*context=native_command_context_current();void*rules,*rule;int lock=1;
    (void)player;(void)game;(void)p;if(!context)return fail_syntax();
    rules=((void*(*)(void*))SEAM_Level_getGameRules)(level);if(!rules)return fail_command(command_failed);
    rule=rule_dfs(rules_root(rules),"dodaylightcycle");
    if(!rule)return fail_command(command_failed);
    native_bag_get_bool(context->input_bag,"lock",&lock);
    if(lock){((LevelSetInt)SEAM_Level_setTime)(level,5000);command_packet_broadcast_time(level,5000);}
    ((RuleSetBoolFn)SEAM_GameRule_setBool)(rule,!lock);
    command_packet_broadcast_gamerule_bool(level,"dodaylightcycle",!lock);
    return 1;
}


