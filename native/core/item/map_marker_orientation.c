#include "map_marker_orientation.h"
#include "../internal.h"

typedef void *(*DecorationCtor)(void*,u8,u8,u8,u8,const void*,const void*);
static NuMC3DS_Hook marker_hook;
static NuMC3DS_Hook player_type_hook;

__attribute__((naked)) static void select_player_type(void){
    __asm__ volatile(
        "ldrb r1, [r0, #0x40]\n"
        "push {r2}\n"
        "ldrb r2, [r0]\n"
        "cmp r2, #0\n"
        "cmpeq r1, #2\n"
        "movls r1, #0\n"
        "pop {r2}\n"
        "add r0, sp, #0x20\n"
        "ldr pc, =0x003265B8\n"
    );
}

static void *on_construct(void *decoration,u8 type,u8 x,u8 y,u8 rotation,const void *label,const void *color){
    if(type==1)rotation=(rotation+8)&15;
    return ((DecorationCtor)marker_hook.trampoline)(decoration,type,x,y,rotation,label,color);
}

int map_marker_orientation_install(void){
    int result;
    marker_hook.target=0x002257B4u;
    marker_hook.expected[0]=0xE92D5FF0u;
    marker_hook.expected[1]=0xE1A04000u;
    marker_hook.replacement=(u32)on_construct;
    result=s->host.install_hook(&marker_hook);
    if(result)return result;
    player_type_hook.target=0x003265B0u;
    player_type_hook.expected[0]=0xE5D01040u;
    player_type_hook.expected[1]=0xE28D0020u;
    player_type_hook.replacement=(u32)select_player_type;
    return s->host.install_hook(&player_type_hook);
}
