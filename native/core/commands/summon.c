#include "../internal.h"
#include "native_command_context.h"
#include "../entity_type_enum.h"
#include "native_property_bag.h"
#include "entity_spawn_service.h"
int cmd_summon(void*player,void*level,void*game,const char*p){
    const NativeCommandContext*context=native_command_context_current();
    const char*name,*canonical_name;
    int eid,block_pos[3];
    float pos[3];
    void *source;

    (void)game;(void)p;(void)player;
    if(!context)return fail_syntax();
    name=native_bag_get_string(context->input_bag,"entityType");
    if(!name)return fail_syntax();
    eid=entity_type_resolve_name(name);

    if(native_command_context_blockpos("spawnPos",block_pos)){
        pos[0]=(float)block_pos[0]+0.5f;
        pos[1]=(float)block_pos[1];
        pos[2]=(float)block_pos[2]+0.5f;
    } else if(!native_command_context_origin_blockpos(block_pos)) {
        return fail_command_localized("commands.summon.failed");
    } else {
        block_pos[1]++;
        pos[0]=(float)block_pos[0]+0.5f;
        pos[1]=(float)block_pos[1];
        pos[2]=(float)block_pos[2]+0.5f;
    }

    native_command_context_output_bool("wasSpawned",0);
    if(eid<0)return fail_command_localized("commands.summon.failed");
    canonical_name=entity_type_name_from_id(eid);
    if(!canonical_name)return fail_command_localized("commands.summon.failed");

    source=native_command_context_block_source();
    if(!command_spawn_source_has_block(source,block_pos))return fail_command_localized("commands.summon.failed");

    if(!command_spawn_entity_by_id(level,source,eid,pos)){
        return fail_command_localized("commands.summon.failed");
    }

    native_command_context_output_string("entityType",canonical_name);
    native_command_context_output_blockpos("spawnPos",block_pos);
    native_command_context_output_bool("wasSpawned",1);
    return 1;
}
