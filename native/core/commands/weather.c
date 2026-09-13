#include "../internal.h"
#include "native_command_context.h"
#include "native_property_bag.h"
#include "command_random.h"
#include "weather_command_service.h"

int cmd_weather(void*player,void*level,void*game,const char*p){
    const NativeCommandContext*context=native_command_context_current();const char*type_;int dur=0,secs=0,raining,thunder;
    (void)player;(void)game;(void)p;if(!context)return fail_syntax();type_=native_bag_get_string(context->input_bag,"type");if(!type_)return fail_syntax();
    if(streq(type_,"clear")){raining=0;thunder=0;}
    else if(streq(type_,"rain")){raining=1;thunder=0;}
    else if(streq(type_,"thunder")){raining=1;thunder=1;}
    else return fail_syntax();
    if(native_bag_has(context->input_bag,"duration")){
        if(!native_bag_get_int(context->input_bag,"duration",&secs))return fail_syntax();
        if(secs<1)return fail_number_too_small(secs,1);
        if(secs>1000000)return fail_number_too_big(secs,1000000);
    }
    if(secs>0)dur=secs;
    else dur=6000+command_random_next_int(level,600)*20;
    command_weather_apply(level,raining,thunder,dur);
    command_weather_broadcast(level,raining,thunder);
    native_command_context_output_string("format",thunder?"commands.weather.thunder":raining?"commands.weather.rain":"commands.weather.clear");
    return 1;
}


