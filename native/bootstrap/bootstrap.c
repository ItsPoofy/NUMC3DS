#include "../include/numc3ds_abi.h"

#ifndef NUMC3DS_BOOTSTRAP_DIAGNOSTICS
#define NUMC3DS_BOOTSTRAP_DIAGNOSTICS 0
#endif

typedef numc3ds_u32 u32;
typedef numc3ds_s32 Result;

enum {
    NATIVE_FILE_SIZE = NUMC3DS_NATIVE_FILE_SIZE,
    RESOURCE_LOADER_RAW = 6,
    EMPTY_RESOURCE_HANDLE = 0x00B3EB10,
    ARM_ABSOLUTE_JUMP = 0xE51FF004,
    RESOURCE_LOADER_HOOK = 0x002307A4,
    RESOURCE_LOADER_ORIGINAL = 0xEBFFDFE8,
    PATCH_ALIAS = 0x07000000,
    PAGE_SIZE = 0x1000,
    CUR_PROCESS_HANDLE = 0xFFFF8001,
    MAP_PROCESS_MEMORY_MAGIC = 0xFFFFFFF2,
};

typedef void (*GameFunction)(void *game, int reload_existing_atlases);
typedef void (*ResourceLocationCtorFn)(void *location, const char *path, u32 loader);
typedef void (*ResourceReadDispatchFn)(void *location, u32 *resource_handle);
typedef void (*ReaderCtorFn)(void *reader);
typedef int (*ReaderAttachFn)(void *reader, u32 resource_handle);
typedef u32 (*ReaderSizeFn)(void *reader);
typedef int (*ReaderReadFn)(void *reader, void *destination, u32 size);
typedef void (*ReaderCleanupFn)(void *reader);
typedef void *(*GameHeapAllocFn)(u32 size, void *selector);
typedef void (*GameHeapFreeFn)(void *allocation, int selector);
__attribute__((section(".call_veneers"), used))
volatile u32 numc3ds_call_veneers[8];

static const char native_path[] = "numc3ds/native.bin";
#if NUMC3DS_BOOTSTRAP_DIAGNOSTICS
static const char msg_open[] = "NuMC3DS bootstrap: resource open failed\n";
static const char msg_size[] = "NuMC3DS bootstrap: native file size mismatch\n";
static const char msg_alloc[] = "NuMC3DS bootstrap: game heap allocation failed\n";
static const char msg_read[] = "NuMC3DS bootstrap: resource read failed\n";
static const char msg_header[] = "NuMC3DS bootstrap: invalid module header\n";
static const char msg_reloc[] = "NuMC3DS bootstrap: invalid relocation table\n";
static const char msg_rwx[] = "NuMC3DS bootstrap: executable permission failed\n";
static const char msg_entry[] = "NuMC3DS bootstrap: core initialization failed\n";
#endif


__attribute__((section(".patch_targets"), used, noinline, naked))
void numc3ds_levelchunk_metadata_write_patch(void)
{
    __asm__ volatile(
        "ldrb r0, [r5, #1]\n"
        "cmp r0, r10\n"
        "beq 1f\n"
        "ldr r0, [sp, #0xc]\n"
        "b 0x00157d40\n"
        "1:\n"
        "b 0x00157d74\n"
    );
}

static void host_debug_string(const char *text, u32 length)
{
    register const char *r0 __asm__("r0") = text;
    register u32 r1 __asm__("r1") = length;
    __asm__ volatile(
        "push {lr}\n"
        "svc 0x3D\n"
        "pop {lr}"
        : : "r"(r0), "r"(r1) : "r2", "r3", "r12", "cc", "memory");
}

#if NUMC3DS_BOOTSTRAP_DIAGNOSTICS
#define DEBUG_MESSAGE(message) host_debug_string((message), sizeof(message) - 1)
#else
#define DEBUG_MESSAGE(message) ((void)0)
#endif

static Result svc_make_process_rwx(void)
{
    register u32 r0 __asm__("r0") = CUR_PROCESS_HANDLE;
    register u32 r1 __asm__("r1") = 1;
    register u32 r2 __asm__("r2") = 0;
    register u32 r3 __asm__("r3") = 0;
    __asm__ volatile(
        "push {lr}\n"
        "svc 0xB3\n"
        "pop {lr}"
        : "+r"(r0) : "r"(r1), "r"(r2), "r"(r3) : "r12", "cc", "memory");
    return (Result)r0;
}

static Result svc_map_current_process_alias(u32 destination, u32 source, u32 size)
{
    register u32 r0 __asm__("r0") = MAP_PROCESS_MEMORY_MAGIC;
    register u32 r1 __asm__("r1") = destination;
    register u32 r2 __asm__("r2") = CUR_PROCESS_HANDLE;
    register u32 r3 __asm__("r3") = source;
    register u32 r4 __asm__("r4") = size;
    register u32 r5 __asm__("r5") = 0;
    register u32 r6 __asm__("r6") = CUR_PROCESS_HANDLE;
    __asm__ volatile(
        "push {r4, r5, r6, lr}\n"
        "svc 0xA0\n"
        "pop {r4, r5, r6, lr}"
        : "+r"(r0), "+r"(r1), "+r"(r2), "+r"(r3), "+r"(r4), "+r"(r5), "+r"(r6)
        : : "r12", "cc", "memory");
    return (Result)r0;
}

static Result svc_unmap_current_process_alias(u32 destination, u32 size)
{
    register u32 r0 __asm__("r0") = CUR_PROCESS_HANDLE;
    register u32 r1 __asm__("r1") = destination;
    register u32 r2 __asm__("r2") = size;
    __asm__ volatile(
        "push {lr}\n"
        "svc 0xA1\n"
        "pop {lr}"
        : "+r"(r0), "+r"(r1), "+r"(r2) : : "r3", "r12", "cc", "memory");
    return (Result)r0;
}

static void sync_code_cache(u32 address, u32 size)
{
    register u32 r0 __asm__("r0") = CUR_PROCESS_HANDLE;
    register u32 r1 __asm__("r1") = address;
    register u32 r2 __asm__("r2") = size;
    __asm__ volatile(
        "push {lr}\n"
        "svc 0x54\n"
        "svc 0x94\n"
        "pop {lr}"
        : "+r"(r0) : "r"(r1), "r"(r2) : "r3", "r12", "cc", "memory");
}

static int write_game_code(u32 address, const u32 *words, u32 count)
{
    u32 page = address & ~(PAGE_SIZE - 1);
    u32 page_offset = address & (PAGE_SIZE - 1);
    u32 size = page_offset + count * sizeof(u32) > PAGE_SIZE ? PAGE_SIZE * 2 : PAGE_SIZE;
    volatile u32 *alias;
    u32 index;

    if (svc_map_current_process_alias(PATCH_ALIAS, page, size) < 0) {
        return -1;
    }
    alias = (volatile u32 *)(PATCH_ALIAS + page_offset);
    for (index = 0; index < count; index++) {
        alias[index] = words[index];
    }
    sync_code_cache(PATCH_ALIAS + page_offset, count * sizeof(u32));
    return svc_unmap_current_process_alias(PATCH_ALIAS, size) < 0 ? -2 : 0;
}

static int disarm_bootstrap_hook(void)
{
    const u32 original = RESOURCE_LOADER_ORIGINAL;

    if (svc_make_process_rwx() < 0) {
        return -1;
    }
    return write_game_code(RESOURCE_LOADER_HOOK, &original, 1);
}

static void *host_heap_alloc(u32 size)
{
    return ((GameHeapAllocFn)0x0011493C)(size, (void *)0x00994898);
}

static void host_heap_free(void *allocation)
{
    if (allocation) {
        ((GameHeapFreeFn)0x001146E8)(allocation, -1);
    }
}

static numc3ds_s32 host_install_hook(NuMC3DS_Hook *hook)
{
    u32 *target;
    u32 *trampoline;
    u32 replacement[2];

    if (!hook || !hook->target || !hook->replacement || (hook->target & 3)) {
        return -1;
    }
    target = (u32 *)hook->target;
    if (target[0] != hook->expected[0] || target[1] != hook->expected[1]) {
        return -2;
    }
    trampoline = (u32 *)host_heap_alloc(16);
    if (!trampoline) {
        return -3;
    }
    if (svc_make_process_rwx() < 0) {
        host_heap_free(trampoline);
        return -4;
    }

    hook->original[0] = target[0];
    hook->original[1] = target[1];
    trampoline[0] = target[0];
    trampoline[1] = target[1];
    trampoline[2] = ARM_ABSOLUTE_JUMP;
    trampoline[3] = hook->target + 8;
    hook->trampoline = (u32)trampoline;
    replacement[0] = ARM_ABSOLUTE_JUMP;
    replacement[1] = hook->replacement;
    if (write_game_code(hook->target, replacement, 2) != 0) {
        host_heap_free(trampoline);
        hook->trampoline = 0;
        return -5;
    }
    return 0;
}

static numc3ds_s32 host_remove_hook(NuMC3DS_Hook *hook)
{
    u32 *target;
    u32 original[2];
    if (!hook || !hook->target || !hook->trampoline) {
        return -1;
    }
    target = (u32 *)hook->target;
    if (target[0] != ARM_ABSOLUTE_JUMP || target[1] != hook->replacement) {
        return -2;
    }
    original[0] = hook->original[0];
    original[1] = hook->original[1];
    if (write_game_code(hook->target, original, 2) != 0) {
        return -3;
    }
    host_heap_free((void *)hook->trampoline);
    hook->trampoline = 0;
    return 0;
}

static int validate_and_relocate(void *module_file, void **image_out, u32 *image_size_out,
                                 NuMC3DS_ModuleEntry *entry_out)
{
    NuMC3DS_ModuleHeader *header = (NuMC3DS_ModuleHeader *)module_file;
    unsigned char *file = (unsigned char *)module_file;
    unsigned char *image;
    u32 *relocations;
    u32 index;

    if (header->magic != NUMC3DS_MODULE_MAGIC ||
        header->format_version != NUMC3DS_MODULE_FORMAT_VERSION ||
        header->header_size != sizeof(NuMC3DS_ModuleHeader) ||
        header->required_host_abi != NUMC3DS_HOST_ABI_VERSION ||
        header->image_offset < header->header_size ||
        header->image_offset + header->image_size > NATIVE_FILE_SIZE ||
        header->entry_offset >= header->image_size ||
        header->relocation_offset > NATIVE_FILE_SIZE ||
        header->relocation_count > (NATIVE_FILE_SIZE - header->relocation_offset) / 4) {
        return -1;
    }

    image = file + header->image_offset;
    relocations = (u32 *)(file + header->relocation_offset);
    for (index = 0; index < header->relocation_count; index++) {
        u32 offset = relocations[index];
        if (offset > header->image_size - 4 || (offset & 3)) {
            return -2;
        }
        *(u32 *)(image + offset) += (u32)image;
    }

    *image_out = image;
    *image_size_out = header->image_size;
    *entry_out = (NuMC3DS_ModuleEntry)(image + header->entry_offset);
    return 0;
}

__attribute__((section(".entry")))
void numc3ds_bootstrap(void *game)
{
    GameFunction load_client_resources = (GameFunction)0x0022874C;
#if NUMC3DS_BOOTSTRAP_ONLY
    load_client_resources(game, 0);
    return;
#else
    ResourceLocationCtorFn resource_location_ctor = (ResourceLocationCtorFn)0x0033C630;
    ResourceReadDispatchFn resource_read = (ResourceReadDispatchFn)0x0065FA5C;
    ReaderCtorFn reader_ctor = (ReaderCtorFn)0x0010CA24;
    ReaderAttachFn reader_attach = (ReaderAttachFn)0x0010C86C;
    ReaderSizeFn reader_size = (ReaderSizeFn)0x0010C9D8;
    ReaderReadFn reader_read = (ReaderReadFn)0x0010C958;
    ReaderCleanupFn reader_close = (ReaderCleanupFn)0x0010C9A4;
    ReaderCleanupFn reader_destroy = (ReaderCleanupFn)0x0010CA5C;
    u32 location[5];
    u32 reader[6];
    u32 resource_handle = EMPTY_RESOURCE_HANDLE;
    void *module_file;
    void *image;
    u32 image_size;
    NuMC3DS_ModuleEntry entry;
    NuMC3DS_HostAbi host;
    int validation;

    if (disarm_bootstrap_hook() != 0) {
        DEBUG_MESSAGE(msg_rwx);
        load_client_resources(game, 0);
        return;
    }

    resource_location_ctor(location, native_path, RESOURCE_LOADER_RAW);
    reader_ctor(reader);
    resource_read(location, &resource_handle);
    if (!reader_attach(reader, resource_handle)) {
        DEBUG_MESSAGE(msg_open);
        reader_destroy(reader);
        load_client_resources(game, 0);
        return;
    }
    if (reader_size(reader) != NATIVE_FILE_SIZE) {
        DEBUG_MESSAGE(msg_size);
        reader_close(reader);
        reader_destroy(reader);
        load_client_resources(game, 0);
        return;
    }

    module_file = host_heap_alloc(NATIVE_FILE_SIZE);
    if (!module_file) {
        DEBUG_MESSAGE(msg_alloc);
        reader_close(reader);
        reader_destroy(reader);
        load_client_resources(game, 0);
        return;
    }
    if (!reader_read(reader, module_file, NATIVE_FILE_SIZE)) {
        DEBUG_MESSAGE(msg_read);
        reader_close(reader);
        reader_destroy(reader);
        host_heap_free(module_file);
        load_client_resources(game, 0);
        return;
    }
    reader_close(reader);
    reader_destroy(reader);

    validation = validate_and_relocate(module_file, &image, &image_size, &entry);
    if (validation != 0) {
        DEBUG_MESSAGE(validation == -1 ? msg_header : msg_reloc);
        host_heap_free(module_file);
        load_client_resources(game, 0);
        return;
    }
    if (svc_make_process_rwx() < 0) {
        DEBUG_MESSAGE(msg_rwx);
        host_heap_free(module_file);
        load_client_resources(game, 0);
        return;
    }
    sync_code_cache((u32)module_file, NATIVE_FILE_SIZE);
    host.magic = NUMC3DS_HOST_MAGIC;
    host.abi_version = NUMC3DS_HOST_ABI_VERSION;
    host.struct_size = sizeof(host);
    host.module_base = (u32)image;
    host.module_size = image_size;
    host.minecraft_game = game;
    host.heap_alloc = host_heap_alloc;
    host.heap_free = host_heap_free;
    host.debug_string = host_debug_string;
    host.install_hook = host_install_hook;
    host.remove_hook = host_remove_hook;

    if (entry(&host) != 0) {
        DEBUG_MESSAGE(msg_entry);
        host_heap_free(module_file);
    }
    load_client_resources(game, 0);
#endif
}
