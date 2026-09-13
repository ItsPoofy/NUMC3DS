#include "../native_command_context.h"
#include "../native_property_bag.h"
#include "../command_origin_access.h"
#include "../../internal.h"

static const NativeCommandContext *active_context;

static float context_base(const NativeCommandContext*context,int axis){
    float position[3];
    if(native_command_origin_world_position(context->origin,position))return position[axis];
    return 0.0f;
}

static int context_block_base(const NativeCommandContext*context,int position[3]){
    if(!context||!context->origin||!position)return 0;
    return native_command_origin_block_position(context->origin,position);
}

const NativeCommandContext *native_command_context_current(void){return active_context;}

const NativeCommandContext *native_command_context_push(const NativeCommandContext *context){
    const NativeCommandContext *previous=active_context;
    active_context=context;
    return previous;
}

void native_command_context_restore(const NativeCommandContext *previous){active_context=previous;}

int native_command_context_blockpos(const char*name,int position[3]){
    int value[3],relative[3],base[3];unsigned index;
    if(!active_context||!active_context->input_bag||!position)return 0;
    if(!native_bag_get_blockpos(active_context->input_bag,name,value,relative))return 0;
    if((relative[0]||relative[1]||relative[2])&&!context_block_base(active_context,base))return 0;
    for(index=0;index<3;index++){
        position[index]=value[index]+(relative[index]?base[index]:0);
    }
    return 1;
}

int native_command_context_origin_blockpos(int position[3]){
    return context_block_base(active_context,position);
}

int native_command_context_rotation(const char*name,int axis,float*value){
    int relative;
    if(!active_context||!active_context->input_bag||!value||axis<0||axis>2)return 0;
    if(!native_bag_get_rotation(active_context->input_bag,name,value,&relative))return 0;
    if(relative)*value+=context_base(active_context,axis);
    return 1;
}

unsigned native_command_context_targets(const char*name,void**out,unsigned max){
    if(!active_context||!active_context->origin||!active_context->input_bag)return 0;
    return native_bag_collect_targets(active_context->origin,active_context->input_bag,name,out,max);
}

int native_command_context_has_target(const char*name){
    return active_context&&native_bag_has(active_context->input_bag,name);
}

void *native_command_context_block_source(void){
    if(!active_context||!active_context->origin)return 0;
    return native_command_origin_block_source(active_context->origin);
}

void native_command_context_output_string(const char*name,const char*value){if(active_context)native_bag_set_string(active_context->output_bag,name,value);}
void native_command_context_output_int(const char*name,int value){if(active_context)native_bag_set_int(active_context->output_bag,name,value);}
void native_command_context_output_float(const char*name,float value){if(active_context)native_bag_set_float(active_context->output_bag,name,value);}
void native_command_context_output_bool(const char*name,int value){if(active_context)native_bag_set_bool(active_context->output_bag,name,value);}
void native_command_context_output_blockpos(const char*name,const int value[3]){if(active_context)native_bag_set_blockpos(active_context->output_bag,name,value);}
void native_command_context_output_strings(const char*name,const char*const*values,unsigned count){if(active_context)native_bag_set_result_list(active_context->output_bag,name,values,count);}
void native_command_context_output_entities(const char*name,void*const*entities,unsigned count){char names[SELECTOR_TARGET_CAPACITY][40];const char*pointers[SELECTOR_TARGET_CAPACITY];unsigned index;if(count>SELECTOR_TARGET_CAPACITY)count=SELECTOR_TARGET_CAPACITY;for(index=0;index<count;index++){entity_name(entities[index],names[index],sizeof(names[index]));pointers[index]=names[index];}native_command_context_output_strings(name,pointers,count);}
