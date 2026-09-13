#include "../native_registry.h"
#include "../native_output.h"
#include "../native_schema.h"
#include "../native_parser.h"
#include "../native_property_bag.h"
#include "block_command_origin.h"
#include "command_parser_validation.h"
#include "native_json_request.h"

typedef void *(*MinecraftCommandsCtorFn)(void*,void*,void*);
typedef u32 (*RequestCommandExecutionFn)(void*,void**,void*,void*);
typedef void (*CommandOverloadGetCallbackFn)(NativeCommandCallback*,NativeCommandOverload*,u32);

static char active_trace_buf[64];
static unsigned active_trace_len;
static int active_trace_recording;

void native_command_trace_begin(void){
    active_trace_len=0;
    active_trace_buf[0]=0;
    active_trace_recording=1;
}

const char *native_command_trace_end(void){
    active_trace_recording=0;
    return active_trace_buf;
}

void native_command_trace(char marker){
    s->host.debug_string(&marker,1);
    if(active_trace_recording&&active_trace_len+2<sizeof(active_trace_buf)){
        if(active_trace_len)active_trace_buf[active_trace_len++]=' ';
        active_trace_buf[active_trace_len++]=marker;
        active_trace_buf[active_trace_len]=0;
    }
}

static void command_trace_text(const char*prefix,const char*text){unsigned length=0,prefix_length=0;while(prefix&&prefix[prefix_length])prefix_length++;s->host.debug_string(prefix,prefix_length);while(text&&text[length]&&length<127)length++;s->host.debug_string(text?text:"",length);}

static u32 command_parser_is_valid_slash_command(void*parser,void*origin,NativeGstdString*message){
    const char *text=message&&message->handle?(const char*)message->handle:"";
    const char *root=native_command_parser_root(text);
    command_trace_text("NuMC3DS command input: ",text);command_trace_text("NuMC3DS command root: ",root?root:"<none>");
    return command_parser_validate(parser,origin,message);
}

static void command_parser_get_command_name(NativeGstdString*out,void*parser,void*origin,NativeGstdString*message){
    const char *name=native_command_parser_root(message&&message->handle?(const char*)message->handle:"");u32 scratch=0;
    (void)parser;
    (void)origin;
    command_trace_text("NuMC3DS command name: ",name?name:"<none>");
    if(!out)return;
    native_command_trace(parser==s->command_parser?'P':'p');
    ((StrCtor)SEAM_StrCtor)(out,name?name:"",&scratch);
    command_trace_text("NuMC3DS command output: ",out->handle?(const char*)out->handle:"<empty>");
    native_command_trace(out->handle?'7':'0');
}

static void command_parser_get_command_json(void *parser,void *origin,NativeGstdString *message,void *json){
    native_command_parser_get_command_json(parser,origin,message,json);
}

static void *minecraft_commands_ctor(void *self,void *owner,void *parser_context){
    void *result=((MinecraftCommandsCtorFn)s->commands_ctor.trampoline)(self,owner,parser_context);
    s->minecraft_commands=result?result:self;
    s->command_game=s->host.minecraft_game;
    return result;
}

static u32 request_command_execution(void *minecraft_commands,void **origin,void *message,void *error){
    void *game=s->command_game?s->command_game:s->host.minecraft_game;
    if(minecraft_commands)native_registry_attach(minecraft_commands,game);
    return ((RequestCommandExecutionFn)s->command_request.trampoline)(minecraft_commands,origin,message,error);
}

static void command_overload_get_callback(NativeCommandCallback *callback,NativeCommandOverload *overload,u32 index){
    native_command_trace('O');
    ((CommandOverloadGetCallbackFn)s->command_overload.trampoline)(callback,overload,index);
    native_command_trace(callback&&callback->invoker?'C':'c');
}

int native_command_system_install_hook(void){
    NuMC3DS_Hook *constructor=&s->commands_ctor,*execution=&s->command_request,*command_valid=&s->command_valid,*command_name=&s->command_name,*command_json=&s->command_json,*command_overload=&s->command_overload;
    constructor->target=SEAM_MinecraftCommands_ctor;constructor->replacement=(u32)minecraft_commands_ctor;constructor->expected[0]=0xE92D43F8u;constructor->expected[1]=0xE1A04000u;
    if(s->host.install_hook(constructor))return -40;
    execution->target=SEAM_RequestCommandExecution;execution->replacement=(u32)request_command_execution;execution->expected[0]=0xE92D4FF0u;execution->expected[1]=0xE1A04000u;
    if(s->host.install_hook(execution))return -41;
    command_valid->target=SEAM_CommandParser_isValidSlashCommand;command_valid->replacement=(u32)command_parser_is_valid_slash_command;command_valid->expected[0]=0xE59F0004u;command_valid->expected[1]=0xE5900000u;
    if(s->host.install_hook(command_valid))return -42;
    command_name->target=SEAM_CommandParser_getCommandName;command_name->replacement=(u32)command_parser_get_command_name;command_name->expected[0]=0xE59F1074u;command_name->expected[1]=0xE92D4070u;
    if(s->host.install_hook(command_name))return -43;
    command_json->target=SEAM_CommandParser_getCommandJson;command_json->replacement=(u32)command_parser_get_command_json;command_json->expected[0]=0xE92D41F0u;command_json->expected[1]=0xE24DD040u;
    if(s->host.install_hook(command_json))return -44;
    command_overload->target=SEAM_CommandOverload_getCallback;command_overload->replacement=(u32)command_overload_get_callback;command_overload->expected[0]=0xE2811024u;command_overload->expected[1]=0xE92D4070u;
    if(s->host.install_hook(command_overload))return -47;
    if(block_command_origin_install())return -90;
    return native_json_request_install();
}
