#include "../internal.h"
#include "native_command_context.h"
#include "native_property_bag.h"
#include "mob_effect_service.h"
#include "effect_registry.h"
#include "entity_classification.h"

int cmd_effect(void*player,void*level,void*game,const char*p){
    const NativeCommandContext*context=native_command_context_current();
    const char*name;int eid,seconds=30,amplifier=0,hide=0;numc3ds_u32 inst[4];
    void*targets[SELECTOR_TARGET_CAPACITY],*successful[SELECTOR_TARGET_CAPACITY];unsigned count,index,success_count=0;
    (void)player;(void)level;(void)game;(void)p;
    if(!context)return fail_syntax();
    count=native_command_context_targets("player",targets,SELECTOR_TARGET_CAPACITY);
    if(!count)return fail_command_localized("commands.generic.noTargetMatch");
    name=native_bag_get_string(context->input_bag,"clear");
    if(!name)name=native_bag_get_string(context->input_bag,"effect");
    if(!name)return fail_syntax();
    if(streq(name,"clear")){
        for(index=0;index<count;index++){
            char pname[32];const char*arguments[1]={pname};entity_name(targets[index],pname,sizeof(pname));
            if(!command_entity_is_mob(targets[index])||!mob_effect_remove_all(targets[index]))return fail_command_localized_args("commands.effect.failure.notActive.all",arguments,1);
            successful[success_count++]=targets[index];
        }
        native_command_context_output_entities("player",successful,success_count);
        return 1;
    }
    eid=native_effect_id_from_name(name);
    if(eid<0){const char*arguments[1]={name};return fail_command_localized_args("commands.effect.notFound",arguments,1);}
    native_bag_get_int(context->input_bag,"seconds",&seconds);
    if(seconds<0)return fail_number_too_small(seconds,0);
    native_bag_get_int(context->input_bag,"amplifier",&amplifier);
    if(amplifier<0)return fail_number_too_small(amplifier,0);
    if(amplifier>255)return fail_number_too_big(amplifier,255);
    native_bag_get_bool(context->input_bag,"hideParticles",&hide);
    for(index=0;index<count;index++){
        if(!command_entity_is_mob(targets[index]))return fail_command_localized("commands.generic.noTargetMatch");zero(inst,sizeof(inst));
        ((EffectCtorFn)SEAM_MobEffectInstance_ctor)(inst,eid,seconds*20,amplifier,0,(unsigned char)(!hide));
        if(seconds<1)mob_effect_remove(targets[index],eid);
        else ((AddEffectFn)SEAM_Mob_addEffect)(targets[index],inst);
        successful[success_count++]=targets[index];
    }
    native_command_context_output_string("effect",name);native_command_context_output_int("amplifier",amplifier+1);native_command_context_output_entities("player",successful,success_count);native_command_context_output_int("seconds",seconds);
    return success_count!=0;
}


