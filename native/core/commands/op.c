#include "../internal.h"
#include "native_command_context.h"
#include "entity_classification.h"
int cmd_op(void*player,void*level,void*game,const char*p){
    const NativeCommandContext*context=native_command_context_current();char pname[32],success[MAX_TEXT+1],failed[MAX_TEXT+1];void*tv[SELECTOR_TARGET_CAPACITY];unsigned nt,i,success_length=0,failed_length=0;
    GetPermissionsLevel get_perm=(GetPermissionsLevel)SEAM_Player_getPermissionsLevel;
    (void)level;(void)game;
    (void)p;if(!context)return fail_syntax();success[0]=failed[0]=0;nt=native_command_context_targets("player",tv,SELECTOR_TARGET_CAPACITY);if(!nt)return fail_command_localized("commands.generic.player.notFound");
    for(i=0;i<nt;i++){
        if(!command_entity_is_player(tv[i]))return fail_command_localized("commands.generic.player.notFound");
        entity_name(tv[i],pname,sizeof(pname));
        if(get_perm(tv[i])>=2){if(failed_length)append(failed,&failed_length,", ");append(failed,&failed_length,pname);continue;}
        ((SetPermissionsLevel)SEAM_Player_setPermissionsLevel)(tv[i],2);
        if(success_length)append(success,&success_length,", ");append(success,&success_length,pname);
    }
    native_command_context_output_string("playersSuccess",success);native_command_context_output_string("playersFailed",failed);
    return 1;
}


