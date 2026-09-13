#include "../internal.h"
#include "entity_teleport.h"
#include "entity_rotation.h"
#include "command_origin_access.h"
#include "native_command_context.h"
#include "native_property_bag.h"

static int tp_rotation_input(const NativeCommandContext*context,const char*name,float*out,int*relative){
    int integer_value;
    if(native_bag_get_int(context->input_bag,name,&integer_value)){
        *out=(float)integer_value;*relative=0;return 1;
    }
    return native_bag_get_rotation(context->input_bag,name,out,relative);
}

int cmd_tp(void*player,void*level,void*game,const char*p){
    const NativeCommandContext*context=native_command_context_current();float pos[3];int block_pos[3],coordinate_destination=0;
    void*victims[SELECTOR_TARGET_CAPACITY],*successful[SELECTOR_TARGET_CAPACITY];void*destination_entity=0;void*dest_resolved[2];
    unsigned count,success_count=0,i;
    float y_rot=0.0f,x_rot=0.0f;int has_y_rot,has_x_rot,y_relative=0,x_relative=0;
    (void)player;(void)level;(void)game;(void)p;
    if(!context)return fail_syntax();
    if(native_command_context_has_target("victim")){
        count=native_command_context_targets("victim",victims,SELECTOR_TARGET_CAPACITY);
        if(!count)return fail_command_localized("commands.generic.noTargetMatch");
    }else{
        victims[0]=native_command_origin_entity(context->origin);
        if(!victims[0])return fail_command_localized("commands.generic.noTargetMatch");
        count=1;
    }
    if(native_command_context_blockpos("destination",block_pos)){
        coordinate_destination=1;
    }else{
        unsigned dest_count=native_command_context_targets("destination",dest_resolved,2);
        int dest_dimension;
        if(dest_count!=1)return fail_command_localized("commands.generic.noTargetMatch");
        destination_entity=dest_resolved[0];
        pos[0]=entity_pos(destination_entity,0);pos[1]=entity_pos(destination_entity,1);pos[2]=entity_pos(destination_entity,2);
        dest_dimension=((int(*)(void*))SEAM_Entity_getDimensionId)(destination_entity);
        for(i=0;i<count;i++){
            if(victims[i]&&((int(*)(void*))SEAM_Entity_getDimensionId)(victims[i])!=dest_dimension)
                return fail_command_localized("commands.tp.notSameDimension");
        }
    }
    has_y_rot=tp_rotation_input(context,"y-rot",&y_rot,&y_relative);
    has_x_rot=tp_rotation_input(context,"x-rot",&x_rot,&x_relative);
    for(i=0;i<count;i++){
        void*victim=victims[i];
        float rotation[2],requested[2];int relative[2],provided[2];
        if(!victim)continue;
        if(destination_entity){
            if(command_entity_get_rotation(destination_entity,rotation))
                ((void(*)(void*,const float*))SEAM_Entity_setRot)(victim,rotation);
        }else if(has_x_rot||has_y_rot){
            requested[0]=has_x_rot?x_rot:0.0f;requested[1]=has_y_rot?y_rot:0.0f;
            relative[0]=has_x_rot&&x_relative;relative[1]=has_y_rot&&y_relative;
            provided[0]=has_x_rot;provided[1]=has_y_rot;
            if(command_entity_resolve_rotation(victim,requested,relative,provided,rotation))
                ((void(*)(void*,const float*))SEAM_Entity_setRot)(victim,rotation);
        }
        if(coordinate_destination){
            if(native_entity_teleport_to_coordinates(victim,block_pos))
                successful[success_count++]=victim;
        }else{
            if(native_entity_teleport(victim,pos))
                successful[success_count++]=victim;
        }
    }
    if(!success_count)return fail_command(command_failed);
    native_command_context_output_entities("victim",successful,success_count);
    if(destination_entity){char destination_name[40];entity_name(destination_entity,destination_name,sizeof(destination_name));native_command_context_output_string("destination",destination_name);}else native_command_context_output_blockpos("destination",block_pos);
    return 1;
}


