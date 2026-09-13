#include "../internal.h"
#include "native_command_context.h"
#include "command_origin_access.h"
#include "entity_classification.h"

static int block_coordinate(float value){
    int result=(int)value;
    if(value<0.0f&&value!=(float)result)result--;
    return result;
}

int cmd_spawnpoint(void*player,void*level,void*game,const char*p){
    const NativeCommandContext*context=native_command_context_current();
    void *targets[SELECTOR_TARGET_CAPACITY],*successful[SELECTOR_TARGET_CAPACITY];
    int position[3],has_position,target_specified;
    unsigned count,success_count=0,i;
    (void)player;(void)level;(void)game;(void)p;
    if(!context)return fail_syntax();
    target_specified=native_command_context_has_target("player");
    if(target_specified){
        count=native_command_context_targets("player",targets,SELECTOR_TARGET_CAPACITY);
        if(!count)return fail_command_localized("commands.generic.player.notFound");
    }else{
        void *origin_ent=native_command_origin_entity(context->origin);
        if(!origin_ent||!command_entity_is_player(origin_ent))return fail_command_localized("commands.generic.player.notFound");
        targets[0]=origin_ent;count=1;
    }
    has_position=native_command_context_blockpos("spawnPos",position);
    for(i=0;i<count;i++){
        int player_pos[3];
        if(!targets[i]||!command_entity_is_player(targets[i]))continue;
        if(has_position){
            player_pos[0]=position[0];player_pos[1]=position[1];player_pos[2]=position[2];
        }else{
            player_pos[0]=block_coordinate(entity_pos(targets[i],0));
            player_pos[1]=block_coordinate(entity_pos(targets[i],1));
            player_pos[2]=block_coordinate(entity_pos(targets[i],2));
            if(!success_count){
                position[0]=player_pos[0];position[1]=player_pos[1];position[2]=player_pos[2];
            }
        }
        ((PlayerSetRespawnPositionFn)SEAM_Player_setRespawnPosition)(targets[i],player_pos,1);
        successful[success_count++]=targets[i];
    }
    if(!success_count)return fail_command_localized("commands.generic.player.notFound");
    native_command_context_output_entities("player",successful,success_count);
    native_command_context_output_blockpos("spawnPos",position);
    if(!target_specified)native_command_context_output_string("format","commands.spawnpoint.success.single");
    else if(has_position)native_command_context_output_string("format","commands.spawnpoint.success.multiple.specific");
    else native_command_context_output_string("format","commands.spawnpoint.success.multiple.generic");
    return 1;
}
