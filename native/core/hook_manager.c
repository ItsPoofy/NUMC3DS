#include "hook_manager.h"
#include "state.h"

#ifndef NUMC3DS_NATIVE_FILE_SIZE
#define NUMC3DS_NATIVE_FILE_SIZE 0x00080000
#endif

enum {
    NATIVE_FILE_SIZE = NUMC3DS_NATIVE_FILE_SIZE,
    CUR_PROCESS_HANDLE = 0xFFFF8001,
    ARM_ABSOLUTE_JUMP = 0xE51FF004,
    CALL_VENEER_BASE = 0x00918FE0,
    CALL_VENEER_COUNT = 4,
    /* Two relocated instructions, the absolute jump, its target, and one
       literal slot per relocated instruction. */
    TRAMPOLINE_BYTES = 24,
};

#define ARM_LDR_PC_RELATIVE_MASK 0x0F7F0000u
#define ARM_LDR_PC_RELATIVE_VALUE 0x051F0000u

/* A trampoline is executed from the native module, not from the hooked
   function's original address. Any PC-relative instruction copied verbatim
   would therefore read the wrong location, so relocate the common
   `ldr Rt, [pc, #imm]` prologue form through a literal slot placed in the
   trampoline. Other instructions are copied unchanged. */
static numc3ds_u32 relocate_trampoline_instruction(numc3ds_u32 instruction,
                                                   numc3ds_u32 original_address,
                                                   numc3ds_u32 trampoline_address,
                                                   numc3ds_u32 literal_address,
                                                   int *supported)
{
    numc3ds_u32 value_address;
    numc3ds_u32 register_index;
    numc3ds_u32 immediate;
    numc3ds_s32 offset;

    if ((instruction & ARM_LDR_PC_RELATIVE_MASK) != ARM_LDR_PC_RELATIVE_VALUE) {
        return instruction;
    }
    immediate = instruction & 0xFFFu;
    if (instruction & (1u << 23)) {
        value_address = original_address + 8u + immediate;
    } else {
        value_address = original_address + 8u - immediate;
    }
    register_index = (instruction >> 12) & 0xFu;
    offset = (numc3ds_s32)(literal_address - (trampoline_address + 8u));
    if (offset < 0 || offset > 0xFFF) {
        *supported = 0;
        return instruction;
    }
    *(volatile numc3ds_u32 *)literal_address = *(volatile numc3ds_u32 *)value_address;
    return (instruction & 0xF0000000u) | 0x059F0000u |
           (register_index << 12) | (numc3ds_u32)offset;
}

static numc3ds_u32 arena_used;
static NuMC3DS_Hook *installed_hooks[256];
static numc3ds_u32 installed_count;
static struct {
    numc3ds_u32 target;
    numc3ds_u32 original;
    numc3ds_u32 veneer;
    numc3ds_u32 replacement;
} installed_calls[256];
static numc3ds_u32 installed_call_count;
static numc3ds_u32 installed_veneer_count;

static numc3ds_u32 veneer_is_referenced(numc3ds_u32 veneer)
{
    numc3ds_u32 index;
    for (index = 0; index < installed_call_count; index++) {
        if (installed_calls[index].veneer == veneer) return 1u;
    }
    return 0u;
}

static void trace_words(char tag, numc3ds_u32 first, numc3ds_u32 second, numc3ds_u32 third)
{
    static const char digits[] = "0123456789ABCDEF";
    char line[29];
    numc3ds_u32 values[3];
    unsigned i;
    unsigned j;
    values[0] = first;
    values[1] = second;
    values[2] = third;
    line[0] = tag;
    for (i = 0; i < 3; ++i) {
        line[1 + i * 9] = ' ';
        for (j = 0; j < 8; ++j) {
            line[2 + i * 9 + j] = digits[(values[i] >> ((7 - j) * 4)) & 15u];
        }
    }
    line[28] = '\n';
    s->host.debug_string(line, sizeof(line));
}

static void sync_code_cache(numc3ds_u32 address, numc3ds_u32 size)
{
    register numc3ds_u32 r0 __asm__("r0") = CUR_PROCESS_HANDLE;
    register numc3ds_u32 r1 __asm__("r1") = address;
    register numc3ds_u32 r2 __asm__("r2") = size;
    __asm__ volatile(
        "push {lr}\n"
        "svc 0x54\n"
        "svc 0x94\n"
        "pop {lr}\n"
        : "+r"(r0) : "r"(r1), "r"(r2) : "r3", "r12", "cc", "memory");
}

void hook_manager_reset(void)
{
    arena_used = 0;
    installed_count = 0;
    installed_call_count = 0;
    installed_veneer_count = 0;
    trace_words('M', s->host.module_base, s->host.module_size,
                (s->host.module_base + s->host.module_size + 3u) & ~3u);
}

numc3ds_s32 hook_manager_install(NuMC3DS_Hook *hook)
{
    numc3ds_u32 *target;
    numc3ds_u32 *trampoline;
    numc3ds_u32 arena;
    numc3ds_u32 limit;

    if (!hook || !hook->target || !hook->replacement || (hook->target & 3)) return -1;
    target = (numc3ds_u32 *)hook->target;
    if (target[0] != hook->expected[0] || target[1] != hook->expected[1]) return -2;

    arena = (s->host.module_base + s->host.module_size + 3u) & ~3u;
    limit = s->host.module_base - sizeof(NuMC3DS_ModuleHeader) + NATIVE_FILE_SIZE;
    if (arena > limit || arena_used > limit - arena || limit - arena - arena_used < TRAMPOLINE_BYTES) return -3;
    trampoline = (numc3ds_u32 *)(arena + arena_used);
    arena_used += TRAMPOLINE_BYTES;

    hook->original[0] = target[0];
    hook->original[1] = target[1];
    trampoline[4] = 0;
    trampoline[5] = 0;
    {
        int supported = 1;
        trampoline[0] = relocate_trampoline_instruction(target[0], hook->target + 0u,
                                                        (numc3ds_u32)trampoline,
                                                        (numc3ds_u32)(trampoline + 4), &supported);
        trampoline[1] = relocate_trampoline_instruction(target[1], hook->target + 4u,
                                                        (numc3ds_u32)(trampoline + 1),
                                                        (numc3ds_u32)(trampoline + 5), &supported);
        if (!supported) {
            arena_used -= TRAMPOLINE_BYTES;
            return -4;
        }
    }
    trampoline[2] = ARM_ABSOLUTE_JUMP;
    trampoline[3] = hook->target + 8;
    hook->trampoline = (numc3ds_u32)trampoline;
    target[0] = ARM_ABSOLUTE_JUMP;
    target[1] = hook->replacement;
    sync_code_cache((numc3ds_u32)trampoline, TRAMPOLINE_BYTES);
    sync_code_cache(hook->target, 8u);
    if (installed_count < sizeof(installed_hooks) / sizeof(installed_hooks[0])) {
        installed_hooks[installed_count++] = hook;
    }
    trace_words('H', hook->target, hook->replacement, hook->trampoline);
    return 0;
}

numc3ds_s32 hook_manager_remove(NuMC3DS_Hook *hook)
{
    numc3ds_u32 *target;
    if (!hook || !hook->target || !hook->trampoline) return -1;
    target = (numc3ds_u32 *)hook->target;
    if (target[0] != ARM_ABSOLUTE_JUMP || target[1] != hook->replacement) return -2;
    target[0] = hook->original[0];
    target[1] = hook->original[1];
    sync_code_cache(hook->target, 8u);
    hook->trampoline = 0;
    return 0;
}

numc3ds_s32 hook_manager_patch_call(numc3ds_u32 target_address, numc3ds_u32 expected,
                                    numc3ds_u32 replacement)
{
    volatile numc3ds_u32 *target;
    volatile numc3ds_u32 *veneer;
    numc3ds_s32 delta;
    numc3ds_u32 instruction;
    numc3ds_u32 index;

    if (!target_address || !replacement || (target_address & 3u)) return -1;
    target = (volatile numc3ds_u32 *)target_address;
    if (*target != expected) return -2;
    if (installed_call_count >= sizeof(installed_calls) / sizeof(installed_calls[0])) return -4;
    veneer = 0;
    for (index = 0; index < installed_call_count; index++) {
        if (installed_calls[index].replacement == replacement) {
            veneer = (volatile numc3ds_u32 *)installed_calls[index].veneer;
            break;
        }
    }
    if (!veneer) {
        if (installed_veneer_count >= CALL_VENEER_COUNT) return -4;
        veneer = (volatile numc3ds_u32 *)(CALL_VENEER_BASE + installed_veneer_count * 8u);
        veneer[0] = ARM_ABSOLUTE_JUMP;
        veneer[1] = replacement;
        installed_veneer_count++;
    }
    delta = (numc3ds_s32)((numc3ds_u32)veneer - (target_address + 8u));
    if ((delta & 3) || delta < -0x02000000 || delta > 0x01FFFFFC) return -3;
    instruction = 0xEB000000u | (((numc3ds_u32)(delta >> 2)) & 0x00FFFFFFu);
    installed_calls[installed_call_count].target = target_address;
    installed_calls[installed_call_count].original = expected;
    installed_calls[installed_call_count].veneer = (numc3ds_u32)veneer;
    installed_calls[installed_call_count].replacement = replacement;
    installed_call_count++;
    *target = instruction;
    sync_code_cache((numc3ds_u32)veneer, 8u);
    sync_code_cache(target_address, 4u);
    trace_words('C', target_address, replacement, instruction);
    return 0;
}

numc3ds_s32 hook_manager_remove_call(numc3ds_u32 target_address)
{
    numc3ds_u32 index;
    for (index = installed_call_count; index > 0; index--) {
        if (installed_calls[index - 1u].target == target_address) {
            if (index != installed_call_count) return -2;
            index--;
            *(volatile numc3ds_u32 *)installed_calls[index].target = installed_calls[index].original;
            installed_call_count--;
            if (!veneer_is_referenced(installed_calls[index].veneer)) {
                *(volatile numc3ds_u32 *)installed_calls[index].veneer = 0;
                *(volatile numc3ds_u32 *)(installed_calls[index].veneer + 4u) = 0;
                installed_veneer_count--;
            }
            sync_code_cache(installed_calls[index].target, 4u);
            return 0;
        }
    }
    return -1;
}

void hook_manager_rollback_all(void)
{
    while (installed_call_count > 0) {
        numc3ds_u32 index = --installed_call_count;
        *(volatile numc3ds_u32 *)installed_calls[index].target = installed_calls[index].original;
        if (!veneer_is_referenced(installed_calls[index].veneer)) {
            *(volatile numc3ds_u32 *)installed_calls[index].veneer = 0;
            *(volatile numc3ds_u32 *)(installed_calls[index].veneer + 4u) = 0;
            installed_veneer_count--;
        }
        sync_code_cache(installed_calls[index].target, 4u);
    }
    while (installed_count > 0) {
        hook_manager_remove(installed_hooks[--installed_count]);
    }
}
