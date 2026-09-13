#include "../internal.h"
#include "native_command_context.h"
#include "native_property_bag.h"
#include "command_region.h"
#include "clone_mutation_service.h"

static int clone_unknown_block(const char *name){
    const char *arguments[1]={name?name:""};
    return fail_command_localized_args("commands.give.block.notFound",arguments,1);
}

int cmd_clone(void *player,void *level,void *game,const char *p){
    const NativeCommandContext *context=native_command_context_current();
    const char *mask,*mode,*filter_name;int begin[3],end[3],destination[3],dlo[3],dhi[3],filter_data=0,copied;
    unsigned char filter_id=0;void *source;CommandRegion region;CloneMutationOptions options;int axis;
    (void)player;(void)level;(void)game;(void)p;
    options.mask=CLONE_MUTATION_REPLACE;options.move=0;options.filter_id=0;options.filter_data=0;options.has_filter_data=0;
    if(!context||!native_command_context_blockpos("begin",begin)||!native_command_context_blockpos("end",end)||!native_command_context_blockpos("destination",destination))return fail_syntax();
    if(begin[1]<0||begin[1]>255||end[1]<0||end[1]>255)return fail_command_localized("commands.clone.outOfWorld");
    if(!command_region_from_bounds(begin,end,&region)){char actual[16],maximum[16];unsigned length=0;const char *arguments[2]={actual,maximum};actual[0]=0;append_int(actual,&length,region.volume);length=0;maximum[0]=0;append_int(maximum,&length,32768);return fail_command_localized_args("commands.clone.tooManyBlocks",arguments,2);}
    for(axis=0;axis<3;axis++){dlo[axis]=destination[axis];dhi[axis]=destination[axis]+region.maximum[axis]-region.minimum[axis];}
    if(dlo[1]<0||dhi[1]>255)return fail_command_localized("commands.clone.outOfWorld");
    mask=native_bag_get_string(context->input_bag,"maskMode");if(!mask)mask="replace";
    mode=native_bag_get_string(context->input_bag,"cloneMode");if(!mode)mode="normal";
    if(streq(mask,"masked"))options.mask=CLONE_MUTATION_MASKED;
    else if(streq(mask,"filtered"))options.mask=CLONE_MUTATION_FILTERED;
    options.move=streq(mode,"move");
    filter_name=native_bag_get_string(context->input_bag,"tileName");
    if(options.mask==CLONE_MUTATION_FILTERED){
        if(!filter_name||!resolve_block_arg((char *)filter_name,&filter_id)){if(!filter_name)return fail_command_localized("commands.clone.filtered.error");return clone_unknown_block(filter_name);}
        options.filter_id=filter_id;options.has_filter_data=native_bag_get_int(context->input_bag,"tileData",&filter_data);
        if(options.has_filter_data&&(filter_data<0||filter_data>15))return fail_syntax();
        options.filter_data=(unsigned char)filter_data;
    }
    if(!streq(mode,"force")&&boxes_overlap(region.minimum,region.maximum,dlo,dhi))return fail_command_localized("commands.clone.noOverlap");
    source=native_command_context_block_source();if(!source)return fail_command(command_failed);
    copied=command_clone_apply(source,&region,destination,&options,&copied);
    if(copied<0)return fail_command(command_failed);
    if(!copied)return fail_command_localized("commands.clone.failed");
    native_command_context_output_int("count",copied);
    return 1;
}
