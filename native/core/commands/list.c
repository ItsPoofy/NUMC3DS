#include "../internal.h"
#include "native_command_context.h"
#include "command_player_list_service.h"
int cmd_list(void*player,void*level,void*game,const char*p){
    char out[MAX_TEXT+1];int cnt=0;(void)player;(void)game;(void)p;
    if(!command_collect_player_names(level,out,sizeof(out),&cnt))return fail_command(command_failed);
    native_command_context_output_int("currentPlayerCount",cnt);native_command_context_output_int("maxPlayerCount",cnt);native_command_context_output_string("players",out);
    return 1;
}


