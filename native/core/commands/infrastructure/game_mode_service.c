#include "../game_mode_service.h"
#include "../entity_classification.h"
#include "../native_types.h"
#include "../../internal.h"

int command_game_mode_label(char *out,unsigned capacity,int game_mode){
    NativeGstdString value;const char *text=0;
    if(!out||!capacity)return 0;
    out[0]=0;zero(&value,sizeof(value));
    ((void(*)(void*,int))SEAM_GameMode_toString)(&value,game_mode);
    if(value.handle)text=(const char*)value.handle;
    if(text&&text[0])copy_text(out,text,capacity-1);
    if(value.handle)((StrDtor)SEAM_StrDtor)(&value);
    return out[0]!=0;
}

int command_game_mode_apply(void *player,int mode){
    if(!player||!command_entity_is_player(player))return 0;
    ((PlayerSetGameType)SEAM_Player_setGameType)(player,mode);
    return 1;
}

unsigned command_game_mode_apply_multiple(void **players,unsigned count,int mode,void **applied_out){
    unsigned index,applied=0;
    if(!players||!count)return 0;
    for(index=0;index<count;index++){
        if(command_game_mode_apply(players[index],mode)){
            if(applied_out)applied_out[applied]=players[index];
            applied++;
        }
    }
    return applied;
}
