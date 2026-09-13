#include "../internal.h"
#include "command_origin_access.h"
#include "entity_damage_service.h"
#include "native_command_context.h"
int cmd_kill(void*player,void*level,void*game,const char*p){
    const NativeCommandContext*context=native_command_context_current();
    void *target=0;void *targets[SELECTOR_TARGET_CAPACITY],*successful[SELECTOR_TARGET_CAPACITY];
    unsigned count,killed_count;
    (void)player;(void)level;(void)game;(void)p;
    if(!context)return fail_syntax();
    if(native_command_context_has_target("target")){
        count=native_command_context_targets("target",targets,SELECTOR_TARGET_CAPACITY);
        if(!count)return fail_command_localized("commands.generic.noTargetMatch");
        killed_count=command_entity_kill_multiple(targets,count,successful);
        if(!killed_count)return fail_command_localized("commands.generic.noTargetMatch");
        native_command_context_output_entities("targetname",successful,killed_count);
        return 1;
    }
    target=native_command_origin_entity(context->origin);
    if(!target)target=player;
    if(!target||!command_entity_kill(target))return fail_command_localized("commands.generic.noTargetMatch");
    native_command_context_output_entities("targetname",&target,1);
    return 1;
}


