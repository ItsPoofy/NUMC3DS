#include "../internal.h"
#include "native_command_context.h"
#include "locate_feature.h"
#include "native_property_bag.h"

static int block_coordinate(float value){
    int result=(int)value;
    if(value<0.0f&&(float)result!=value)result--;
    return result;
}

int cmd_locate(void*player,void*level,void*game,const char*p){
    const NativeCommandContext *context=native_command_context_current();
    const char *feature;int origin[3],destination[3];void *source;
    (void)game;
    (void)p;
    if(!context||!context->input_bag)return fail_syntax();
    feature=native_bag_get_string(context->input_bag,"feature");
    if(!feature||!feature[0])return fail_syntax();
    if(!player){
        return fail_command_localized("commands.locate.fail.noplayer");
    }
    if(!native_command_context_origin_blockpos(origin)){
        origin[0]=block_coordinate(entity_pos(player,0));
        origin[1]=block_coordinate(entity_pos(player,1));
        origin[2]=block_coordinate(entity_pos(player,2));
    }
    source=native_command_context_block_source();
    if(!locate_feature_find(source,level,origin,feature,destination)){
        return fail_command_localized("commands.locate.fail.nostructurefound");
    }
    native_command_context_output_string("feature",feature);native_command_context_output_blockpos("destination",destination);
    return 1;
}
