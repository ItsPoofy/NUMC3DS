#include "../internal.h"
#include "native_command_context.h"
#include "native_property_bag.h"
#include "command_packets.h"
#include "command_localization.h"

int cmd_time(void*player,void*level,void*game,const char*p){
    const NativeCommandContext*context=native_command_context_current();const char*value_name,*prefix;int value,now,output_time;char output[MAX_TEXT+1],number[16];unsigned length=0;
    (void)player;(void)game;(void)p;if(!context)return fail_syntax();now=((LevelGetInt)SEAM_Level_getTime)(level);
    prefix=native_schema_text(context->schema->prefix);
    if(streq(prefix,"add")&&native_bag_get_int(context->input_bag,"amount",&value)){
        now+=value;
        ((LevelSetInt)SEAM_Level_setTime)(level,now);
        command_packet_broadcast_time(level,now);
        native_command_context_output_int("time",value);
        return 1;
    }
    value_name=native_bag_get_string(context->input_bag,"time");
    if(value_name&&(streq(prefix,"query")||streq(value_name,"daytime")||streq(value_name,"gametime"))){
        const char*key;const char*arguments[1]={number};
        if(streq(value_name,"daytime")){value=divide(now,24000);value=now-value*24000;key="commands.time.query.daytime";}
        else if(streq(value_name,"day")){value=divide(now,24000);key="commands.time.query.day";}
        else if(streq(value_name,"gametime")){value=now;key="commands.time.query.gametime";}
        else return fail_syntax();
        number[0]=0;length=0;append_int(number,&length,value);
        if(!command_localize(output,sizeof(output),key,arguments,1))copy_text(output,key,sizeof(output)-1);
        native_command_context_output_string("body",output);return 1;
    }
    if(value_name){if(streq(value_name,"day"))value=1000;else if(streq(value_name,"night"))value=13000;else return fail_syntax();output_time=value;}
    else if(!native_bag_get_int(context->input_bag,"time",&value))return fail_syntax();
    else output_time=divide(now,24000)*24000+value;
    value=divide(now,24000)*24000+value;
    ((LevelSetInt)SEAM_Level_setTime)(level,value);
    command_packet_broadcast_time(level,value);
    native_command_context_output_int("time",output_time);
    return 1;
}


