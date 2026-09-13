#include "map_update_schedule.h"
#include "../internal.h"

static NuMC3DS_Hook bounds_hook, start_hook;

__attribute__((naked)) static void sample_bounds(void){
    __asm__ volatile(
        "add r1, r2, r4, lsl #1\n"
        "cmp r1, #0\n"
        "movlt r1, #0\n"
        "cmp r1, #128\n"
        "movgt r1, #128\n"
        "mul r0, r0, r0\n"
        "ldr pc, =0x0072C6B8\n"
    );
}

__attribute__((naked)) static void sample_start(void){
    __asm__ volatile(
        "ldr r0, [sp, #0x150]\n"
        "ldrb r1, [r0, #0x1c]\n"
        "mov r0, #48\n"
        "lsr r0, r0, r1\n"
        "ldr r1, [sp, #0x90]\n"
        "ldrsb r1, [r1, #0x85]\n"
        "cmp r1, #0\n"
        "lsrne r0, r0, #1\n"
        "ldr r1, [sp, #0x6c]\n"
        "sub r0, r1, r0\n"
        "cmp r0, #0\n"
        "movlt r0, #0\n"
        "cmp r0, #128\n"
        "movgt r0, #128\n"
        "ldr r1, [sp, #0x18]\n"
        "push {r0}\n"
        "ldr r0, =0xBF800000\n"
        "vmov s21, r0\n"
        "pop {r0}\n"
        "ldr pc, =0x0072C770\n"
    );
}

int map_update_schedule_install(void){
    int result;
    bounds_hook.target=0x0072C6B0u;
    bounds_hook.expected[0]=0xE0811002u;
    bounds_hook.expected[1]=0xE0000090u;
    bounds_hook.replacement=(u32)sample_bounds;
    start_hook.target=0x0072C764u;
    start_hook.expected[0]=0xE59D0044u;
    start_hook.expected[1]=0xE59D1018u;
    start_hook.replacement=(u32)sample_start;
    result=s->host.install_hook(&bounds_hook);
    if(result)return result;
    return s->host.install_hook(&start_hook);
}
