#include "../internal.h"
#include "native_command_context.h"
#include "native_property_bag.h"
#include "entity_classification.h"
#include "game_mode_service.h"

int cmd_gamemode(void*player,void*level,void*game,const char*p){
    const NativeCommandContext *context=native_command_context_current();
    const char*mode_name;char label[64];void*targets[SELECTOR_TARGET_CAPACITY];int mode;unsigned count;
    (void)level;(void)game;(void)p;
    if(!context)return fail_syntax();
    mode_name=native_bag_get_string(context->input_bag,"gameMode");
    if(mode_name){
        if(streq(mode_name,"survival")||streq(mode_name,"s"))mode=0;
        else if(streq(mode_name,"creative")||streq(mode_name,"c"))mode=1;
        else if(streq(mode_name,"adventure")||streq(mode_name,"a"))mode=2;
        else return fail_syntax();
    }else if(!native_bag_get_int(context->input_bag,"gameMode",&mode)||mode<0||mode>2)return fail_syntax();
    if(!command_game_mode_label(label,sizeof(label),mode))return fail_syntax();
    if(native_command_context_has_target("player")){
        void*applied[SELECTOR_TARGET_CAPACITY];unsigned applied_count;
        count=native_command_context_targets("player",targets,SELECTOR_TARGET_CAPACITY);
        if(!count)return fail_command_localized("commands.gamemode.fail.invalidtarget");
        applied_count=command_game_mode_apply_multiple(targets,count,mode,applied);
        if(!applied_count)return fail_command_localized("commands.gamemode.fail.invalidtarget");
        native_command_context_output_string("result", "%commands.gamemode.success.other");
        native_command_context_output_entities("player", applied, applied_count);
        native_command_context_output_string("gameMode", label);
    }else{
        if(!command_game_mode_apply(player,mode))return fail_command_localized("commands.gamemode.fail.invalidtarget");
        native_command_context_output_string("result", "%commands.gamemode.success.self");
        native_command_context_output_entities("player", &player, 1);
        native_command_context_output_string("gameMode", label);
    }
    return 1;
}


