#include "../internal.h"
#include "native_command_context.h"
#include "native_property_bag.h"
#include "block_mutation_service.h"

int cmd_testforblock(void*player,void*level,void*game,const char*p){
    const NativeCommandContext *context=native_command_context_current();
    const char*name;int position[3],data=0,has_data;unsigned char id,actual,actual_data;void*src;
    (void)player;(void)game;(void)p;
    if(!context||!native_command_context_blockpos("position",position))return fail_syntax();
    name=native_bag_get_string(context->input_bag,"tileName");
    if(!name||!resolve_block_arg((char*)name,&id)){
        {const char*arguments[1]={name?name:""};return fail_command_localized_args("commands.setblock.notFound",arguments,1);}
    }
    if(position[1]<0||position[1]>255)return fail_command_localized("commands.testforblock.outOfWorld");
    src=native_command_context_block_source();if(!src)return fail_command(command_failed);
    has_data=native_bag_get_int(context->input_bag,"dataValue",&data);
    if(has_data&&(data<0||data>15))return fail_syntax();
    actual=command_block_read_id(src,position);actual_data=command_block_read_data(src,position);
    if(actual!=id){
        char x[16],y[16],z[16],found[128],expected[128];unsigned length=0;const char*arguments[5]={x,y,z,found,expected};
        x[0]=0;append_int(x,&length,position[0]);length=0;y[0]=0;append_int(y,&length,position[1]);length=0;z[0]=0;append_int(z,&length,position[2]);
        command_block_description_name(actual,actual_data,found,sizeof(found));command_block_description_name(id,(unsigned char)data,expected,sizeof(expected));
        return fail_command_localized_args("commands.testforblock.failed.tile",arguments,5);
    }
    if(has_data&&actual_data!=(unsigned char)data){
        char x[16],y[16],z[16],found[16],expected[16];unsigned length=0;const char*arguments[5]={x,y,z,found,expected};
        x[0]=0;append_int(x,&length,position[0]);length=0;y[0]=0;append_int(y,&length,position[1]);length=0;z[0]=0;append_int(z,&length,position[2]);length=0;found[0]=0;append_int(found,&length,actual_data);length=0;expected[0]=0;append_int(expected,&length,data);
        return fail_command_localized_args("commands.testforblock.failed.data",arguments,5);
    }
    native_command_context_output_blockpos("position",position);native_command_context_output_bool("matches",1);
    return 1;
}


