#include "map_shading.h"
#include "../internal.h"

static NuMC3DS_Hook row_hook;
static NuMC3DS_Hook chunk_hook, elevation_hook;

__attribute__((naked)) static void sample_chunk(void){
    __asm__ volatile(
        "movs r5, r0\n"
        "bne 1f\n"
        "ldr r0, =0xBF800000\n"
        "vmov s21, r0\n"
        "ldr pc, =0x0072D3E4\n"
        "1: ldr pc, =0x0072C8B0\n"
    );
}

__attribute__((naked)) static void sample_elevation(void){
    __asm__ volatile(
        "vcmp.f32 s21, #0.0\n"
        "vmrs APSR_nzcv, FPSCR\n"
        "bge 1f\n"
        "vmov.f32 s21, s20\n"
        "ldr pc, =0x0072D3E4\n"
        "1:\n"
        "vsub.f32 s1, s20, s21\n"
        "ldr r0, [sp, #0xcc]\n"
        "ldr pc, =0x0072D000\n"
    );
}

__attribute__((naked)) static void map_shading_row(void){
    __asm__ volatile(
        "ldr r0, [sp, #0x1c]\n"
        "cmp r0, #0\n"
        "blt 1f\n"
        "ldr r1, [sp, #0x98]\n"
        "cmp r1, #0\n"
        "beq 1f\n"
        "push {r2,r3}\n"
        "ldr r1, =0x00B2E560\n"
        "ldr r2, [sp, #0x30]\n"
        "ldr r3, [r1]\n"
        "cmp r2, r3\n"
        "ldreq r2, [sp, #0x34]\n"
        "ldreq r3, [r1, #4]\n"
        "cmpeq r2, r3\n"
        "ldreq r2, [sp, #0x38]\n"
        "ldreq r3, [r1, #8]\n"
        "cmpeq r2, r3\n"
        "ldreq r2, [sp, #0x3c]\n"
        "ldreq r3, [r1, #12]\n"
        "cmpeq r2, r3\n"
        "pop {r2,r3}\n"
        "beq 1f\n"
        "cmp r0, #0\n"
        "ldr pc, =0x0072D14C\n"
        "1: ldr r0, =0xBF800000\n"
        "vmov s21, r0\n"
        "ldr pc, =0x0072D3E4\n"
    );
}

int map_shading_install(void){
    int result;
    row_hook.target=0x0072D144u;
    row_hook.expected[0]=0xE59D001Cu;
    row_hook.expected[1]=0xE3500000u;
    row_hook.replacement=(u32)map_shading_row;
    result=s->host.install_hook(&row_hook);
    if(result)return result;
    chunk_hook.target=0x0072C8A8u;
    chunk_hook.expected[0]=0xE1B05000u;
    chunk_hook.expected[1]=0x0A0002CCu;
    chunk_hook.replacement=(u32)sample_chunk;
    result=s->host.install_hook(&chunk_hook);
    if(result)return result;
    elevation_hook.target=0x0072CFF8u;
    elevation_hook.expected[0]=0xEE7A0A6Au;
    elevation_hook.expected[1]=0xE59D00CCu;
    elevation_hook.replacement=(u32)sample_elevation;
    return s->host.install_hook(&elevation_hook);
}
