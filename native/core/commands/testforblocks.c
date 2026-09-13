#include "../internal.h"
#include "native_command_context.h"
#include "native_property_bag.h"
#include "command_region.h"

int cmd_testforblocks(void*player,void*level,void*game,const char*p){
    const NativeCommandContext *context=native_command_context_current();
    const char*mode;int begin[3],end[3],destination[3],x,y,z,compared=0;void*src;
    CommandRegion region;
    (void)player;(void)game;(void)p;
    if(!context||!native_command_context_blockpos("begin",begin)||!native_command_context_blockpos("end",end)||!native_command_context_blockpos("destination",destination))return fail_syntax();
    if(begin[1]<0||begin[1]>255||end[1]<0||end[1]>255||destination[1]<0)return fail_command_localized("commands.compare.outOfWorld");
    if(!command_region_from_bounds(begin,end,&region)){char actual[16],maximum[16];unsigned length=0;const char*arguments[2]={actual,maximum};actual[0]=0;append_int(actual,&length,region.volume);length=0;maximum[0]=0;append_int(maximum,&length,32768);return fail_command_localized_args("commands.compare.tooManyBlocks",arguments,2);}
    if(destination[1]+region.maximum[1]-region.minimum[1]>255)return fail_command_localized("commands.compare.outOfWorld");
    mode=native_bag_get_string(context->input_bag,"mode");if(!mode)mode="all";
    src=native_command_context_block_source();if(!src)return fail_command(command_failed);
    for(x=0;x<=region.maximum[0]-region.minimum[0];x++)for(y=0;y<=region.maximum[1]-region.minimum[1];y++)for(z=0;z<=region.maximum[2]-region.minimum[2];z++){
        int source[3]={region.minimum[0]+x,region.minimum[1]+y,region.minimum[2]+z};int target[3]={destination[0]+x,destination[1]+y,destination[2]+z};
        unsigned char source_id=command_block_read_id(src,source),target_id=command_block_read_id(src,target);
        if(streq(mode,"masked")&&source_id==0)continue;
        if(source_id!=target_id||command_block_read_data(src,source)!=command_block_read_data(src,target))return fail_command_localized("commands.compare.failed");
        compared++;
    }
    native_command_context_output_int("compareCount",compared);native_command_context_output_bool("matches",1);
    return 1;
}


