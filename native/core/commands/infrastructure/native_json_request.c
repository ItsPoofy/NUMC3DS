#include "native_json_request.h"
#include "../native_registry.h"
#include "../native_output.h"

typedef u32 (*JsonRequestFn)(void*,void**,void*,int,void*);
static NuMC3DS_Hook request_hook;

static u32 request(void *commands,void **origin,void *json,int flags,void *error){
    void *game=s->command_game?s->command_game:s->session_game;
    if(!game)game=s->host.minecraft_game;
    if(!native_registry_attach(commands,game))return native_command_result_error();
    return ((JsonRequestFn)request_hook.trampoline)(commands,origin,json,flags,error);
}

int native_json_request_install(void){
    request_hook.target=0x006ECC94u;request_hook.replacement=(u32)request;
    request_hook.expected[0]=0xE92D4FFFu;request_hook.expected[1]=0xE24DD0B4u;
    return s->host.install_hook(&request_hook);
}
