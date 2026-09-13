#include "../internal.h"
#include "native_command_context.h"
#include "native_property_bag.h"
#include "command_region.h"

static int fill_unknown_block(const char*name){
    const char*arguments[1]={name?name:""};
    return fail_command_localized_args("commands.give.block.notFound",arguments,1);
}

int cmd_fill(void*player,void*level,void*game,const char*p){
    const NativeCommandContext *context=native_command_context_current();
    const char *name,*mode,*replace_name;int from[3],to[3],data_value=0,replace_data=0;
    int x,y,z,written=0,has_replace_data=0;int *changed=0;unsigned char id,data=0,replace_id=0;void*src;
    CommandRegion region;
    (void)player;(void)game;(void)p;
    if(!context||!native_command_context_blockpos("from",from)||!native_command_context_blockpos("to",to))return fail_syntax();
    name=native_bag_get_string(context->input_bag,"tileName");
    if(!name||!resolve_block_arg((char*)name,&id))return fill_unknown_block(name);
    if(native_bag_get_int(context->input_bag,"tileData",&data_value)){
        if(data_value<0||data_value>15)return fail_syntax();
        data=(unsigned char)data_value;
    }
    mode=native_bag_get_string(context->input_bag,"oldBlockHandling");
    if(!mode)mode="replace";
    replace_name=native_bag_get_string(context->input_bag,"replaceTileName");
    if(replace_name){
        if(!resolve_block_arg((char*)replace_name,&replace_id))return fill_unknown_block(replace_name);
        has_replace_data=native_bag_get_int(context->input_bag,"replaceDataValue",&replace_data);
        if(has_replace_data&&(replace_data<0||replace_data>15))return fail_syntax();
    }
    if(from[1]<0||from[1]>255||to[1]<0||to[1]>255)return fail_command_localized("commands.fill.outOfWorld");
    if(!command_region_from_bounds(from,to,&region)){char actual[16],maximum[16];unsigned length=0;const char*arguments[2]={actual,maximum};actual[0]=0;append_int(actual,&length,region.volume);length=0;maximum[0]=0;append_int(maximum,&length,32768);return fail_command_localized_args("commands.fill.tooManyBlocks",arguments,2);}
    src=native_command_context_block_source();
    if(!src)return fail_command(command_failed);
    changed=(int *)s->host.heap_alloc((u32)region.volume*3u*sizeof(*changed));
    if(!changed)return fail_command(command_failed);
    for(x=region.minimum[0];x<=region.maximum[0];x++)for(y=region.minimum[1];y<=region.maximum[1];y++)for(z=region.minimum[2];z<=region.maximum[2];z++){
        int position[3]={x,y,z};int shell=(x==region.minimum[0]||x==region.maximum[0]||y==region.minimum[1]||y==region.maximum[1]||z==region.minimum[2]||z==region.maximum[2]);
        unsigned char old_id=command_block_read_id(src,position);
        if(replace_name&&(old_id!=replace_id||(has_replace_data&&command_block_read_data(src,position)!=(unsigned char)replace_data)))continue;
        if(streq(mode,"keep")&&old_id!=0)continue;
        if(streq(mode,"outline")&&!shell)continue;
        if(streq(mode,"hollow")&&!shell){if(command_block_set(src,position,0,0)){changed[written*3]=x;changed[written*3+1]=y;changed[written*3+2]=z;written++;}continue;}
        if(streq(mode,"destroy"))command_block_destroy(src,position,1);
        if(command_block_set(src,position,id,data)){changed[written*3]=x;changed[written*3+1]=y;changed[written*3+2]=z;written++;}
    }
    for(x=0;x<written;x++)command_block_update_neighbors(src,&changed[x*3]);
    s->host.heap_free(changed);
    if(!written)return fail_command_localized("commands.fill.failed");
    native_command_context_output_int("fillCount",written);native_command_context_output_string("blockName",name);
    return 1;
}


