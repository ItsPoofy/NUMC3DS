#ifndef NUMC3DS_ABI_H
#define NUMC3DS_ABI_H

typedef unsigned int numc3ds_u32;
typedef int numc3ds_s32;

enum {
    NUMC3DS_MODULE_MAGIC = 0x434D754E,
    NUMC3DS_MODULE_FORMAT_VERSION = 1,
    NUMC3DS_HOST_MAGIC = 0x4148754E,
    NUMC3DS_HOST_ABI_VERSION = 1,
};

typedef struct {
    numc3ds_u32 magic;
    numc3ds_u32 format_version;
    numc3ds_u32 header_size;
    numc3ds_u32 required_host_abi;
    numc3ds_u32 image_offset;
    numc3ds_u32 image_size;
    numc3ds_u32 entry_offset;
    numc3ds_u32 relocation_offset;
    numc3ds_u32 relocation_count;
    numc3ds_u32 flags;
} NuMC3DS_ModuleHeader;

typedef struct {
    numc3ds_u32 target;
    numc3ds_u32 replacement;
    numc3ds_u32 expected[2];
    numc3ds_u32 original[2];
    numc3ds_u32 trampoline;
} NuMC3DS_Hook;

typedef struct NuMC3DS_HostAbi {
    numc3ds_u32 magic;
    numc3ds_u32 abi_version;
    numc3ds_u32 struct_size;
    numc3ds_u32 module_base;
    numc3ds_u32 module_size;
    void *minecraft_game;
    void *(*heap_alloc)(numc3ds_u32 size);
    void (*heap_free)(void *allocation);
    void (*debug_string)(const char *text, numc3ds_u32 length);
    numc3ds_s32 (*install_hook)(NuMC3DS_Hook *hook);
    numc3ds_s32 (*remove_hook)(NuMC3DS_Hook *hook);
} NuMC3DS_HostAbi;

typedef numc3ds_s32 (*NuMC3DS_ModuleEntry)(const NuMC3DS_HostAbi *host);

#endif
