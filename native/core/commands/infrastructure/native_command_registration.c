#include "../native_command_registration.h"
#include "../native_command_loader.h"
#include "../native_callbacks.h"
#include "../../internal.h"

typedef struct { const char *name; NativeCommandInvokerFn invoker; } NativeRegistration;
typedef void *(*VectorAllocateFn)(u32,u32);

#define NATIVE_COMMAND_INVOKER(name) static u32 invoke_##name(void*storage,void*origin,void*input_bag,void*output_bag){return native_callback_invoke_handler(storage,origin,input_bag,output_bag,cmd_##name);}
NATIVE_COMMAND_INVOKER(help) NATIVE_COMMAND_INVOKER(time) NATIVE_COMMAND_INVOKER(gamemode) NATIVE_COMMAND_INVOKER(tp) NATIVE_COMMAND_INVOKER(kill) NATIVE_COMMAND_INVOKER(difficulty)
NATIVE_COMMAND_INVOKER(setworldspawn) NATIVE_COMMAND_INVOKER(say) NATIVE_COMMAND_INVOKER(me) NATIVE_COMMAND_INVOKER(list) NATIVE_COMMAND_INVOKER(give) NATIVE_COMMAND_INVOKER(clear)
NATIVE_COMMAND_INVOKER(effect) NATIVE_COMMAND_INVOKER(enchant) NATIVE_COMMAND_INVOKER(xp) NATIVE_COMMAND_INVOKER(gamerule) NATIVE_COMMAND_INVOKER(daylock) NATIVE_COMMAND_INVOKER(tell)
NATIVE_COMMAND_INVOKER(title) NATIVE_COMMAND_INVOKER(playsound) NATIVE_COMMAND_INVOKER(testfor) NATIVE_COMMAND_INVOKER(summon) NATIVE_COMMAND_INVOKER(spawnpoint) NATIVE_COMMAND_INVOKER(op)
NATIVE_COMMAND_INVOKER(deop) NATIVE_COMMAND_INVOKER(weather) NATIVE_COMMAND_INVOKER(toggledownfall) NATIVE_COMMAND_INVOKER(setblock) NATIVE_COMMAND_INVOKER(fill) NATIVE_COMMAND_INVOKER(clone)
NATIVE_COMMAND_INVOKER(testforblock) NATIVE_COMMAND_INVOKER(testforblocks) NATIVE_COMMAND_INVOKER(spreadplayers) NATIVE_COMMAND_INVOKER(execute) NATIVE_COMMAND_INVOKER(locate)
NATIVE_COMMAND_INVOKER(stopsound) NATIVE_COMMAND_INVOKER(replaceitem) NATIVE_COMMAND_INVOKER(testall)
#undef NATIVE_COMMAND_INVOKER

static const NativeRegistration registrations[]={
    {"help",invoke_help},{"time",invoke_time},{"gamemode",invoke_gamemode},{"tp",invoke_tp},{"kill",invoke_kill},{"difficulty",invoke_difficulty},
    {"setworldspawn",invoke_setworldspawn},{"say",invoke_say},{"me",invoke_me},{"list",invoke_list},{"give",invoke_give},{"clear",invoke_clear},
    {"effect",invoke_effect},{"enchant",invoke_enchant},{"xp",invoke_xp},{"gamerule",invoke_gamerule},{"daylock",invoke_daylock},{"tell",invoke_tell},
    {"title",invoke_title},{"playsound",invoke_playsound},{"testfor",invoke_testfor},{"summon",invoke_summon},{"spawnpoint",invoke_spawnpoint},{"op",invoke_op},
    {"deop",invoke_deop},{"weather",invoke_weather},{"toggledownfall",invoke_toggledownfall},{"setblock",invoke_setblock},{"fill",invoke_fill},{"clone",invoke_clone},
    {"testforblock",invoke_testforblock},{"testforblocks",invoke_testforblocks},{"spreadplayers",invoke_spreadplayers},{"execute",invoke_execute},{"locate",invoke_locate},
    {"stopsound",invoke_stopsound},{"replaceitem",invoke_replaceitem},{"testall",invoke_testall}
};

static NativeCommandInvokerFn registration_invoker(const char *name){unsigned index;for(index=0;index<sizeof(registrations)/sizeof(registrations[0]);index++)if(streq(registrations[index].name,name))return registrations[index].invoker;return 0;}

int native_command_registration_register(void){
    unsigned root_index,version_index,overload_index;
    for(root_index=0;root_index<native_schema_root_count;root_index++){
        const NativeSchemaRoot *root=&native_schema_roots[root_index];const NativeSchemaVersion *versions=native_schema_root_versions(root);const NativeSharedCommand *loaded=native_command_loader_versions(root_index);if(!loaded)return 0;
        for(version_index=0;version_index<root->version_count;version_index++){
            NativeCommand *command=(NativeCommand*)loaded[version_index].object;const NativeSchemaOverload *schemas=native_schema_version_overloads(&versions[version_index]);NativeSharedCommand *overloads;if(!command)return 0;overloads=(NativeSharedCommand*)command->overloads.begin;
            for(overload_index=0;overload_index<versions[version_index].overload_count;overload_index++){
                NativeCommandOverload *overload=(NativeCommandOverload*)overloads[overload_index].object;NativeCommandCallback *callback;NativeCommandInvokerFn invoker=registration_invoker(native_schema_text(schemas[overload_index].dispatch));if(!overload||!invoker)return 0;
                if(overload->callbacks.begin!=overload->callbacks.end)continue;
                callback=(NativeCommandCallback*)((VectorAllocateFn)SEAM_gstd_allocator_allocate)(sizeof(*callback),0);if(!callback)return 0;native_callback_init(callback,&schemas[overload_index],invoker);
                overload->callbacks.begin=(u32)callback;overload->callbacks.end=(u32)(callback+1);overload->callbacks.capacity=overload->callbacks.end;
            }
        }
    }
    return 1;
}
