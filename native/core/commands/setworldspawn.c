#include "../internal.h"
#include "native_command_context.h"
#include "command_packets.h"

int cmd_setworldspawn(void*player,void*level,void*game,const char*p){
    const NativeCommandContext*context=native_command_context_current();int position[3];void*level_data,*source;
    (void)player;(void)game;(void)p;if(!context)return fail_syntax();
    source=native_command_context_block_source();if(!source)return fail_command(command_failed);
    if(((int(*)(void*))SEAM_BlockSource_getDimensionId)(source)!=0)return fail_command_localized("commands.setworldspawn.wrongDimension");
    if(!native_command_context_blockpos("spawnPoint",position)&&!native_command_context_origin_blockpos(position))return fail_command(command_failed);
    position[1]=((int(*)(void*,const int*,int,int))SEAM_BlockSource_getAboveTopSolidBlock)(source,position,0,0);
    level_data=(unsigned char*)level+SEAM_Level_levelDataOffset;
    ((void(*)(void*,const int*))SEAM_LevelData_setSpawn)(level_data,position);
    command_packet_broadcast_world_spawn(source,position);
    native_command_context_output_blockpos("spawnPoint",position);
    return 1;
}


