#include "../internal.h"
#include "native_command_context.h"
#include "native_property_bag.h"
int cmd_xp(void*player,void*level,void*game,const char*p){
    const NativeCommandContext*context=native_command_context_current();int amount,is_levels;void*tv[SELECTOR_TARGET_CAPACITY];unsigned nt,i;
    (void)level;(void)game;(void)p;if(!context||!native_bag_get_int(context->input_bag,"amount",&amount))return fail_syntax();
    if(native_command_context_has_target("player")){
        nt=native_command_context_targets("player",tv,SELECTOR_TARGET_CAPACITY);
        if(!nt)return fail_command_localized("commands.generic.player.notFound");
    }else{
        if(!player)return fail_command_localized("commands.generic.player.notFound");
        tv[0]=player;nt=1;
    }
    is_levels=streq(native_schema_text(context->schema->name),"byLevel");
    if(!is_levels&&amount<0)return fail_command_localized("commands.xp.failure.widthdrawXp");
    for(i=0;i<nt;i++){
        if(is_levels)((XpFn)SEAM_Player_addLevels)(tv[i],amount);
        else ((XpFn)SEAM_Player_addExperience)(tv[i],amount);
    }
    if(is_levels)native_command_context_output_string("message",amount<0?"commands.xp.success.negative.levels":"commands.xp.success.levels");
    native_command_context_output_int("amount",amount);native_command_context_output_entities("player",tv,nt);
    return 1;
}


