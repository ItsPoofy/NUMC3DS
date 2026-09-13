#include "../internal.h"
#include "native_command_context.h"
#include "native_property_bag.h"
#include "spread_players_service.h"

static void float_text(char *out,float value){
    unsigned length=0;int scaled;
    if(value<0.0f){append(out,&length,"-");value=-value;}
    scaled=(int)(value*100.0f+0.5f);append_int(out,&length,scaled/100);append(out,&length,".");
    if((scaled%100)<10)append(out,&length,"0");append_int(out,&length,scaled%100);
}

int cmd_spreadplayers(void*player,void*level,void*game,const char*p){
    const NativeCommandContext*context=native_command_context_current();
    float cx,cz,spread_distance,max_range;void*src;void*targets[SELECTOR_TARGET_CAPACITY];unsigned target_count;SpreadPlayersResult result;
    (void)player;(void)game;(void)p;
    if(!context)return fail_syntax();
    if(!native_command_context_rotation("x",0,&cx)||!native_command_context_rotation("z",2,&cz)||
       !native_bag_get_float(context->input_bag,"spreadDistance",&spread_distance)||
       !native_bag_get_float(context->input_bag,"maxRange",&max_range))return fail_syntax();
    target_count=native_command_context_targets("victim",targets,SELECTOR_TARGET_CAPACITY);
    if(!target_count)return fail_command_localized("commands.generic.noTargetMatch");
    if(spread_distance<0.0f)return fail_number_too_small((int)spread_distance,0);
    if(max_range<spread_distance+1.0f)return fail_number_too_small((int)max_range,(int)(spread_distance+1.0f));
    src=native_command_context_block_source();
    if(!src)return fail_command(command_failed);
    if(!command_spread_players(level,src,targets,target_count,cx,cz,spread_distance,max_range,&result)){
        char count[16],x[24],z[24],distance[24];unsigned length=0;const char *arguments[4]={count,x,z,distance};
        count[0]=0;append_int(count,&length,(int)target_count);float_text(x,cx);float_text(z,cz);float_text(distance,result.minimum_distance);
        return fail_command_localized_args("commands.spreadplayers.failure.players",arguments,4);
    }
    native_command_context_output_int("count",(int)target_count);native_command_context_output_float("x",cx);native_command_context_output_float("z",cz);
    return 1;
}
