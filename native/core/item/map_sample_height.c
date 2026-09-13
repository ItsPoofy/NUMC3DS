#include "map_sample_height.h"
#include "../internal.h"

static NuMC3DS_Hook height_hook;

__attribute__((used,noinline)) static int sample_height(void *chunk,const void *position){
    int top[3];
    int height=((int(*)(void*,const void*))0x00695BB4u)(chunk,position)+1;
    ((void(*)(int*,void*,const void*))0x0015968Cu)(top,chunk,position);
    if(top[1]>height)height=top[1];
    return height;
}

__attribute__((naked)) static void on_sample_height(void){
    __asm__ volatile(
        "push {r2,r3,r12,lr}\n"
        "bl sample_height\n"
        "pop {r2,r3,r12,lr}\n"
        "mov r4, r0\n"
        "ldr pc, =0x0072CB50\n"
    );
}

int map_sample_height_install(void){
    height_hook.target=0x0072CB48u;
    height_hook.expected[0]=0xEBFDA419u;
    height_hook.expected[1]=0xE2804001u;
    height_hook.replacement=(u32)on_sample_height;
    return s->host.install_hook(&height_hook);
}
