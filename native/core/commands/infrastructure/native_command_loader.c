#include "../native_command_loader.h"
#include "../native_callbacks.h"
#include "../native_result_formatter.h"
#include "../../internal.h"

typedef void *(*OperatorNewFn)(u32);
typedef void *(*VectorAllocateFn)(u32,u32);
typedef void (*RegisterStringEnumFn)(void*,NativeGstdString*,NativeCommandCallback*);
typedef void (*SharedAssignFn)(NativeSharedCommand*,u32,u32);

typedef struct {
    NativeSharedCommand *commands;
    unsigned *root_offsets;
    void *parser;
    unsigned command_count;
} NativeCommandLoaderState;

static NativeCommandLoaderState loaded;

static void *vector_allocate(unsigned bytes){return ((VectorAllocateFn)SEAM_gstd_allocator_allocate)(bytes,0);}
static void *object_allocate(unsigned bytes){void*object=((OperatorNewFn)SEAM_operator_new)(bytes);if(object)zero(object,bytes);return object;}
static void object_free(void *object){if(object)((void(*)(void*))SEAM_operator_delete)(object);}

static void command_shared_release(NativeSharedCommand *shared){
    if(shared&&shared->control)((SharedAssignFn)SEAM_CommandShared_assign)(shared,0,0);
}

static void loader_state_destroy(NativeCommandLoaderState *state){
    unsigned index;
    if(!state)return;
    for(index=0;state->commands&&index<state->command_count;index++)command_shared_release(&state->commands[index]);
    object_free(state->commands);
    object_free(state->root_offsets);
    zero(state,sizeof(*state));
}

static int shared_create(NativeSharedCommand*shared,void*object){
    void*control;if(!shared||!object)return 0;shared->object=shared->control=0;control=((ControlAlloc)SEAM_ControlAlloc)();if(!control)return 0;
    ((ControlFn)SEAM_ControlLock)(control);if(*((u8*)control+8)){shared->object=(u32)object;shared->control=(u32)control;((ControlFn)SEAM_ControlReference)(control);}
    ((ControlFn)SEAM_ControlUnlock)(control);return shared->control!=0;
}

static int string_init(NativeGstdString*string,const char*text){u32 scratch=0;if(!string)return 0;((StrCtor)SEAM_StrCtor)(string,text?text:"",&scratch);return string->handle!=0;}

static int load_parameter(NativeCommandParameter*destination,const NativeSchemaParameter*source){
    zero(destination,sizeof(*destination));destination->type=source->type;destination->optional=native_schema_parameter_optional(source);
    destination->target_main=native_schema_parameter_target_main(source);destination->target_players_only=native_schema_parameter_players_only(source);
    if(!string_init(&destination->name,native_schema_text(source->name)))return 0;
    if(string_init(&destination->enum_name,native_schema_text(source->enum_name)))return 1;
    ((StrDtor)SEAM_StrDtor)(&destination->name);
    zero(destination,sizeof(*destination));
    return 0;
}

static int load_parameter_vector(NativeVector *vector,const NativeSchemaParameter*source,unsigned count){
    NativeCommandParameter*parameters;unsigned index;if(!count)return 1;
    parameters=(NativeCommandParameter*)vector_allocate(sizeof(*parameters)*count);if(!parameters)return 0;zero(parameters,sizeof(*parameters)*count);
    vector->begin=(u32)parameters;vector->end=(u32)parameters;vector->capacity=(u32)(parameters+count);
    for(index=0;index<count;index++){vector->end=(u32)(parameters+index+1);if(!load_parameter(&parameters[index],&source[index]))return 0;}
    return 1;
}

static int load_overload(NativeSharedCommand*shared,const NativeSchemaOverload*schema){
    const NativeSchemaParameter*parameters=native_schema_overload_parameters(schema);NativeCommandOverload*overload=(NativeCommandOverload*)object_allocate(sizeof(*overload));unsigned index,required=0;if(!overload)return 0;
    if(!shared_create(shared,overload)){object_free(overload);return 0;}
    if(!string_init(&overload->name,native_schema_text(schema->name))||!string_init(&overload->usage,native_schema_text(schema->usage))||!string_init(&overload->compact_usage,native_schema_text(schema->name))||!string_init(&overload->step_name,""))return 0;
    for(index=0;index<schema->parameter_count;index++)if(!native_schema_parameter_optional(&parameters[index]))required++;overload->required_parameter_count=(u8)required;
    overload->output_to_speech=(u8)native_schema_overload_flag(schema,NATIVE_SCHEMA_OVERLOAD_OUTPUT_SPEECH);
    if(!load_parameter_vector(&overload->input,native_schema_overload_parameters(schema),schema->parameter_count))return 0;
    if(!load_parameter_vector(&overload->output,native_schema_overload_outputs(schema),schema->output_count))return 0;
    if(!native_command_overload_set_format_strings(overload,schema))return 0;
    return 1;
}

static int load_overloads(NativeCommand*command,const NativeSchemaVersion*version){
    const NativeSchemaOverload*source=native_schema_version_overloads(version);NativeSharedCommand*overloads;unsigned index;if(!version->overload_count)return 0;
    overloads=(NativeSharedCommand*)vector_allocate(sizeof(*overloads)*version->overload_count);if(!overloads)return 0;zero(overloads,sizeof(*overloads)*version->overload_count);
    command->overloads.begin=(u32)overloads;command->overloads.end=(u32)overloads;command->overloads.capacity=(u32)(overloads+version->overload_count);
    for(index=0;index<version->overload_count;index++){command->overloads.end=(u32)(overloads+index+1);if(!load_overload(&overloads[index],&source[index]))return 0;}
    return 1;
}

static int load_command(NativeSharedCommand*shared,const NativeSchemaRoot*root,const NativeSchemaVersion*version,unsigned version_index){
    NativeCommand*command=(NativeCommand*)object_allocate(sizeof(*command));if(!command)return 0;
    if(!shared_create(shared,command)){object_free(command);return 0;}
    if(!string_init(&command->name,native_schema_text(root->name))||!string_init(&command->description,native_schema_text(version->description))||!string_init(&command->usage,native_schema_text(root->name)))return 0;
    command->reserved_04=(u8)(version_index+1);
    command->flags_14=1;command->permission=version->permission;command->player_origin_policy=3;command->requires_edu=native_schema_version_flag(version,NATIVE_SCHEMA_VERSION_REQUIRES_EDU);
    command->requires_chat_permission=native_schema_version_flag(version,NATIVE_SCHEMA_VERSION_CHAT_PERMISSION);command->requires_tell_permission=native_schema_version_flag(version,NATIVE_SCHEMA_VERSION_TELL_PERMISSION);
    command->is_hidden=native_schema_version_flag(version,NATIVE_SCHEMA_VERSION_HIDDEN);command->allows_indirect_exec=native_schema_version_flag(version,NATIVE_SCHEMA_VERSION_ALLOWS_INDIRECT);
    return load_overloads(command,version);
}

static int load_commands(void *parser,int *changed){
    NativeCommandLoaderState next;unsigned root_index,version_index,total_versions=0;
    if(changed)*changed=0;
    if(loaded.parser==parser&&loaded.commands&&loaded.root_offsets)return 1;
    zero(&next,sizeof(next));
    for(root_index=0;root_index<native_schema_root_count;root_index++)total_versions+=native_schema_roots[root_index].version_count;
    next.root_offsets=(unsigned*)object_allocate(sizeof(*next.root_offsets)*(native_schema_root_count+1));next.commands=(NativeSharedCommand*)object_allocate(sizeof(*next.commands)*total_versions);
    if(!next.root_offsets||!next.commands)goto failed;
    for(root_index=0;root_index<native_schema_root_count;root_index++){
        const NativeSchemaRoot*root=&native_schema_roots[root_index];const NativeSchemaVersion*versions=native_schema_root_versions(root);
        next.root_offsets[root_index]=next.command_count;
        for(version_index=0;version_index<root->version_count;version_index++){
            NativeSharedCommand *command=&next.commands[next.command_count++];
            if(!load_command(command,root,&versions[version_index],version_index))goto failed;
        }
    }
    next.root_offsets[native_schema_root_count]=next.command_count;
    next.parser=parser;
    loader_state_destroy(&loaded);
    loaded=next;
    if(changed)*changed=1;
    return 1;
failed:
    loader_state_destroy(&next);
    return 0;
}


static void register_string_enums(void*parser){
    unsigned root_index,version_index,overload_index,parameter_index;NativeGstdString name;u32 scratch;NativeCommandCallback callback;
    for(root_index=0;root_index<native_schema_root_count;root_index++){const NativeSchemaRoot*root=&native_schema_roots[root_index];for(version_index=0;version_index<root->version_count;version_index++){const NativeSchemaVersion*version=&native_schema_root_versions(root)[version_index];for(overload_index=0;overload_index<version->overload_count;overload_index++){const NativeSchemaOverload*overload=&native_schema_version_overloads(version)[overload_index];for(parameter_index=0;parameter_index<overload->parameter_count;parameter_index++){const NativeSchemaParameter*parameter=&native_schema_overload_parameters(overload)[parameter_index];const char*enum_name=native_schema_text(parameter->enum_name);const char*enum_values=native_schema_text(parameter->enum_values);if(parameter->type!=NATIVE_PARAM_STRING_ENUM||!enum_name[0]||(!enum_values[0]&&!native_enum_callback_available(parameter)))continue;scratch=0;zero(&name,sizeof(name));((StrCtor)SEAM_StrCtor)(&name,enum_name,&scratch);native_enum_callback_init(&callback,parameter);((RegisterStringEnumFn)SEAM_CommandParser_registerStringEnum)(parser,&name,&callback);((StrDtor)SEAM_StrDtor)(&name);}}}}
}

int native_command_loader_load(void *parser){int changed=0;if(!parser||!load_commands(parser,&changed))return 0;if(changed)register_string_enums(parser);return 1;}
const NativeSharedCommand *native_command_loader_versions(unsigned root_index){return loaded.commands&&loaded.root_offsets&&root_index<native_schema_root_count?loaded.commands+loaded.root_offsets[root_index]:0;}
