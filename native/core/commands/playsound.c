#include "../internal.h"
#include "native_command_context.h"
#include "command_packets.h"
#include "native_property_bag.h"
int cmd_playsound(void*player,void*level,void*game,const char*p){
    const NativeCommandContext*context=native_command_context_current();const char*name;float vol=1.0f,pitch=1.0f,minvol=0.0f;
    void*tv[SELECTOR_TARGET_CAPACITY];unsigned nt,i;
    (void)level;(void)p;if(!context)return fail_syntax();name=native_bag_get_string(context->input_bag,"sound");if(!name)return fail_syntax();nt=native_command_context_targets("player",tv,SELECTOR_TARGET_CAPACITY);if(!nt)return fail_command_localized("commands.generic.player.notFound");
    native_bag_get_float(context->input_bag,"volume",&vol);native_bag_get_float(context->input_bag,"pitch",&pitch);native_bag_get_float(context->input_bag,"minimumVolume",&minvol);
    for(i=0;i<nt;i++){
        float position[3],listener[3],distance,radius,play_volume=vol,dx,dy,dz;
        if(native_command_context_has_target("position")){
            int block_position[3];
            if(!native_command_context_blockpos("position",block_position))return fail_syntax();
            position[0]=(float)block_position[0];position[1]=(float)block_position[1];position[2]=(float)block_position[2];
        }else{
            position[0]=entity_pos(tv[i],0);position[1]=entity_pos(tv[i],1);position[2]=entity_pos(tv[i],2);
        }
        listener[0]=entity_pos(tv[i],0);listener[1]=entity_pos(tv[i],1);listener[2]=entity_pos(tv[i],2);
        distance=((EntityDistanceToVec3Fn)SEAM_Entity_distanceToVec3)(tv[i],position);
        radius=play_volume>1.0f?play_volume*16.0f:16.0f;
        if(radius<distance){
            if(minvol<=0.0f){char name_out[40];const char*arguments[1]={name_out};entity_name(tv[i],name_out,sizeof(name_out));return fail_command_localized_args("commands.playsound.playerTooFar",arguments,1);}
            dx=position[0]-listener[0];dy=position[1]-listener[1];dz=position[2]-listener[2];
            if(distance>0.0f){position[0]=listener[0]+dx/distance+dx/distance;position[1]=listener[1]+dy/distance+dy/distance;position[2]=listener[2]+dz/distance+dz/distance;}
            play_volume=minvol;
        }
        if(!command_packet_play_sound(tv[i],name,position,play_volume,pitch))return fail_command(command_failed);
    }
    native_command_context_output_string("sound",name);native_command_context_output_entities("player",tv,nt);
    return 1;
}


