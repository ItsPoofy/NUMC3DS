#include "../internal.h"
#include "native_command_context.h"
int cmd_testfor(void*player,void*level,void*game,const char*p){
    const NativeCommandContext*context=native_command_context_current();
    void*tv[SELECTOR_TARGET_CAPACITY];unsigned nt,i;
    (void)player;(void)level;(void)game;(void)p;if(!context)return fail_syntax();nt=native_command_context_targets("victim",tv,SELECTOR_TARGET_CAPACITY);if(!nt)return fail_command_localized("commands.generic.noTargetMatch");
    native_command_context_output_entities("victim",tv,nt);
    return 1;
}
