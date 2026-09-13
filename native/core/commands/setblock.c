#include "../internal.h"
#include "native_command_context.h"
#include "native_property_bag.h"
int cmd_setblock(void*player,void*level,void*game,const char*p){
    const NativeCommandContext *context=native_command_context_current();
    const char *name,*mode_;int pos[3],data_value=0;unsigned char id,data=0;void*src;
    (void)player;(void)game;(void)p;
    if(!context||!native_command_context_blockpos("position",pos))return fail_syntax();
    name=native_bag_get_string(context->input_bag,"tileName");
    if(!name||!resolve_block_arg((char*)name,&id)){
        {const char*arguments[1]={name?name:""};return fail_command_localized_args("commands.give.block.notFound",arguments,1);}
    }
    if(native_bag_get_int(context->input_bag,"tileData",&data_value)){
        if(data_value<0||data_value>15)return fail_syntax();
        data=(unsigned char)data_value;
    }
    mode_=native_bag_get_string(context->input_bag,"oldBlockHandling");
    if(!mode_)mode_="replace";
    if(pos[1]<0||pos[1]>255)return fail_command_localized("commands.setblock.outOfWorld");
    src=native_command_context_block_source();
    if(!src)return fail_command(command_failed);
    if(streq(mode_,"keep")&&command_block_read_id(src,pos)!=0)return fail_command_localized("commands.setblock.noChange");
    if(streq(mode_,"destroy"))command_block_destroy(src,pos,1);
    if(!command_block_set(src,pos,id,data))return fail_command_localized("commands.setblock.failed");
    return 1;
}


