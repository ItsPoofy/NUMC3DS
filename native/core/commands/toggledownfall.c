#include "../internal.h"
#include "native_command_context.h"
#include "weather_command_service.h"

int cmd_toggledownfall(void*player,void*level,void*game,const char*p){
    const NativeCommandContext*context=native_command_context_current();
    void*source;int rain_level;
    (void)player;(void)game;(void)p;
    if(!context)return fail_syntax();
    source=native_command_context_block_source();
    if(!source)return fail_command_localized("commands.generic.invalidcontext");
    rain_level=command_weather_toggle_downfall(level);
    native_command_context_output_int("rainLevel",rain_level);
    return 1;
}
