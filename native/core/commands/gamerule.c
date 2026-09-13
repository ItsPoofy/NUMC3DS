#include "../internal.h"
#include "native_command_context.h"
#include "native_property_bag.h"
#include "command_packets.h"

static void list_rule_names(void*node,char*out,unsigned*length){
    char*name;
    if(!node)return;
    list_rule_names(*(void**)((unsigned char*)node+8),out,length);
    name=*(char**)((unsigned char*)node+0x10);
    if(name){if(*length)append(out,length,", ");append(out,length,name);}
    list_rule_names(*(void**)((unsigned char*)node+0xc),out,length);
}

int cmd_gamerule(void*player,void*level,void*game,const char*p){
    const NativeCommandContext*context=native_command_context_current();const char*name;char value[48],display[MAX_TEXT+1];void*rules,*rule;
    (void)player;(void)game;(void)p;if(!context)return fail_syntax();
    rules=((void*(*)(void*))SEAM_Level_getGameRules)(level);
    if(!rules)return fail_command(command_failed);
    name=native_bag_get_string(context->input_bag,"rule");
    if(!name){
        unsigned length=0;display[0]=0;
        list_rule_names(rules_root(rules),display,&length);
        native_command_context_output_string("displayString",display);native_command_context_output_string("details","");
        return 1;
    }
    if(streq(native_schema_text(context->schema->name),"getter")){
        unsigned length=0;const char*arguments[1]={name};
        rule=rule_dfs(rules_root(rules),name);if(!rule)return fail_command_localized_args("commands.gamerule.norule",arguments,1);
        display[0]=0;append(display,&length,name);append(display,&length,": ");rule_print(display,&length,rule);
        native_command_context_output_string("displayString",display);native_command_context_output_string("details","");
        return 1;
    }
    if(streq(native_schema_text(context->schema->name),"setter")){int bool_value;if(!native_bag_get_bool(context->input_bag,"value",&bool_value))return fail_syntax();copy_text(value,bool_value?"true":"false",sizeof(value)-1);}
    else {int int_value;unsigned length=0;if(!native_bag_get_int(context->input_bag,"value",&int_value))return fail_syntax();value[0]=0;append_int(value,&length,int_value);}
    if(!gamerule_set(rules,name,value)){const char*arguments[1]={name};return fail_command_localized_args("commands.gamerule.norule",arguments,1);}
    native_command_context_output_string("name",name);
    if(streq(native_schema_text(context->schema->name),"setter")){
        int bool_value=streq(value,"true");
        native_command_context_output_bool("value",bool_value);
        command_packet_broadcast_gamerule_bool(level,name,bool_value);
    }else{
        int int_value=0;
        native_bag_get_int(context->input_bag,"value",&int_value);
        native_command_context_output_int("value",int_value);
        command_packet_broadcast_gamerule_int(level,name,int_value);
    }
    return 1;
}


