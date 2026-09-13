#include "extended_chunk_storage.h"
#include "../network/session/mcpe_transport_selector.h"
#include "../hook_manager.h"
#include "../seams.h"
#include "../state.h"

#define MAX_EXTENDED_CHUNKS 256

typedef struct {
    void *chunk;
    u8 *subchunks[8];
} ExtendedChunkEntry;

typedef void *(*SubChunkNewFn)(u32);
typedef void *(*SubChunkCtorFn)(void *, int, int);
typedef void (*SubChunkDtorFn)(void *);
typedef void (*BrightnessFreeFn)(void *);
typedef void (*LevelChunkNewSubChunkFn)(void *, unsigned int, int);
typedef void *(*LevelChunkDtorFn)(void *);
typedef void *(*OverWorldCtorFn)(void *, void *);
typedef void (*GenerateUnloadedChunkAABBFn)(void *, const float *, void *);

static ExtendedChunkEntry s_extended_chunks[MAX_EXTENDED_CHUNKS];
static NuMC3DS_Hook new_subchunk_hook;
static NuMC3DS_Hook dtor_hook;
static NuMC3DS_Hook overworld_ctor_hook;
static NuMC3DS_Hook gen_unloaded_aabb_hook;

static u8 *alloc_subchunk(void)
{
    void *sub = ((SubChunkNewFn)0x00661D80u)(0x180Cu);
    if (sub) {
        ((SubChunkCtorFn)0x00661CDCu)(sub, 0, 0);
        return (u8 *)sub;
    }
    if (s && s != (State *)0xFFFFFFFFu && s->magic == MAGIC && s->host.heap_alloc) {
        sub = s->host.heap_alloc(0x180Cu);
        if (sub) {
            zero(sub, 0x180Cu);
            return (u8 *)sub;
        }
    }
    return 0;
}

static void free_subchunk(u8 *sub)
{
    void *light;
    if (!sub) return;
    light = *(void **)(sub + 0x1800u);
    if (light) {
        ((BrightnessFreeFn)0x00484F70u)(light);
        *(void **)(sub + 0x1800u) = 0;
    }
    ((SubChunkDtorFn)0x00661D70u)(sub);
}

static ExtendedChunkEntry *find_entry(const void *chunk)
{
    u32 i, idx;
    if (!chunk) return 0;
    idx = (((u32)chunk) >> 6) & (MAX_EXTENDED_CHUNKS - 1);
    for (i = 0; i < MAX_EXTENDED_CHUNKS; ++i) {
        u32 slot = (idx + i) & (MAX_EXTENDED_CHUNKS - 1);
        if (s_extended_chunks[slot].chunk == chunk) {
            return &s_extended_chunks[slot];
        }
        if (!s_extended_chunks[slot].chunk) {
            return 0;
        }
    }
    return 0;
}

static ExtendedChunkEntry *find_or_create_entry(void *chunk)
{
    u32 i, idx, empty_slot = MAX_EXTENDED_CHUNKS;
    if (!chunk) return 0;
    idx = (((u32)chunk) >> 6) & (MAX_EXTENDED_CHUNKS - 1);
    for (i = 0; i < MAX_EXTENDED_CHUNKS; ++i) {
        u32 slot = (idx + i) & (MAX_EXTENDED_CHUNKS - 1);
        if (s_extended_chunks[slot].chunk == chunk) {
            return &s_extended_chunks[slot];
        }
        if (!s_extended_chunks[slot].chunk && empty_slot == MAX_EXTENDED_CHUNKS) {
            empty_slot = slot;
        }
    }
    if (empty_slot != MAX_EXTENDED_CHUNKS) {
        s_extended_chunks[empty_slot].chunk = chunk;
        zero(s_extended_chunks[empty_slot].subchunks, sizeof(s_extended_chunks[empty_slot].subchunks));
        return &s_extended_chunks[empty_slot];
    }
    return 0;
}

u8 *extended_chunk_get_subchunk(const void *chunk, u32 subchunk_index)
{
    ExtendedChunkEntry *entry;
    if (!chunk) return 0;
    if (subchunk_index < 8u) {
        if (subchunk_index >= *(const u32 *)((const u8 *)chunk + 0x7Cu)) return 0;
        return *(u8 **)((const u8 *)chunk + 0x5Cu + subchunk_index * 4u);
    }
    if (subchunk_index < 16u) {
        entry = find_entry(chunk);
        if (!entry) return 0;
        return entry->subchunks[subchunk_index - 8u];
    }
    return 0;
}

u8 *extended_chunk_get_or_create_subchunk(void *chunk, u32 subchunk_index)
{
    ExtendedChunkEntry *entry;
    u8 *sub;
    if (!chunk) return 0;
    if (subchunk_index < 8u) {
        ((LevelChunkNewSubChunkFn)SEAM_LevelChunk_newSubChunk)(chunk, subchunk_index, 0);
        if (subchunk_index >= *(u32 *)((u8 *)chunk + 0x7Cu)) return 0;
        return *(u8 **)((u8 *)chunk + 0x5Cu + subchunk_index * 4u);
    }
    if (subchunk_index < 16u) {
        entry = find_or_create_entry(chunk);
        if (!entry) return 0;
        sub = entry->subchunks[subchunk_index - 8u];
        if (!sub) {
            sub = alloc_subchunk();
            entry->subchunks[subchunk_index - 8u] = sub;
        }
        return sub;
    }
    return 0;
}

void extended_chunk_on_chunk_destroy(void *chunk)
{
    ExtendedChunkEntry *entry = find_entry(chunk);
    u32 i;
    if (!entry) return;
    for (i = 0; i < 8u; ++i) {
        if (entry->subchunks[i]) {
            free_subchunk(entry->subchunks[i]);
            entry->subchunks[i] = 0;
        }
    }
    entry->chunk = 0;
}

void extended_chunk_storage_reset(void)
{
    u32 i, j;
    for (i = 0; i < MAX_EXTENDED_CHUNKS; ++i) {
        if (s_extended_chunks[i].chunk) {
            for (j = 0; j < 8u; ++j) {
                if (s_extended_chunks[i].subchunks[j]) {
                    free_subchunk(s_extended_chunks[i].subchunks[j]);
                    s_extended_chunks[i].subchunks[j] = 0;
                }
            }
            s_extended_chunks[i].chunk = 0;
        }
    }
}

static void on_levelchunk_new_subchunk(void *chunk, unsigned int subchunk_index, int param_3)
{
    if (subchunk_index < 8u) {
        ((LevelChunkNewSubChunkFn)new_subchunk_hook.trampoline)(chunk, subchunk_index, param_3);
        return;
    }
    if (mcpe_transport_selector_kind() == MCPE_TRANSPORT_IPV4 && subchunk_index < 16u) {
        extended_chunk_get_or_create_subchunk(chunk, subchunk_index);
    }
}

static void *on_levelchunk_dtor(void *chunk)
{
    extended_chunk_on_chunk_destroy(chunk);
    return ((LevelChunkDtorFn)dtor_hook.trampoline)(chunk);
}

static void *on_overworld_ctor(void *self, void *level)
{
    void *result = ((OverWorldCtorFn)overworld_ctor_hook.trampoline)(self, level);
    if (mcpe_transport_selector_kind() == MCPE_TRANSPORT_IPV4 && result) {
        *(u16 *)((u8 *)result + 0x88) = 256;
    }
    return result;
}

static void on_generate_unloaded_chunk_aabb(void *bs, const float *aabb, void *out_vec)
{
    if (!aabb) return;
    if (aabb[0] < -32000000.0f || aabb[0] > 32000000.0f ||
        aabb[3] < -32000000.0f || aabb[3] > 32000000.0f ||
        aabb[2] < -32000000.0f || aabb[2] > 32000000.0f ||
        aabb[5] < -32000000.0f || aabb[5] > 32000000.0f ||
        (aabb[3] - aabb[0]) > 512.0f || (aabb[5] - aabb[2]) > 512.0f ||
        aabb[3] < aabb[0] || aabb[5] < aabb[2]) {
        return;
    }
    ((GenerateUnloadedChunkAABBFn)gen_unloaded_aabb_hook.trampoline)(bs, aabb, out_vec);
}

int extended_chunk_storage_install_hooks(void)
{
    zero(&new_subchunk_hook, sizeof(new_subchunk_hook));
    new_subchunk_hook.target = SEAM_LevelChunk_newSubChunk;
    new_subchunk_hook.replacement = (u32)on_levelchunk_new_subchunk;
    new_subchunk_hook.expected[0] = 0xE92D4FF0u;
    new_subchunk_hook.expected[1] = 0xE1A06000u;
    if (s->host.install_hook(&new_subchunk_hook)) return -90;

    zero(&dtor_hook, sizeof(dtor_hook));
    dtor_hook.target = 0x0015CF14u;
    dtor_hook.replacement = (u32)on_levelchunk_dtor;
    dtor_hook.expected[0] = 0xE92D47F0u;
    dtor_hook.expected[1] = 0xE2804B02u;
    if (s->host.install_hook(&dtor_hook)) return -91;

    zero(&overworld_ctor_hook, sizeof(overworld_ctor_hook));
    overworld_ctor_hook.target = 0x002D4308u;
    overworld_ctor_hook.replacement = (u32)on_overworld_ctor;
    overworld_ctor_hook.expected[0] = 0xE92D47F0u;
    overworld_ctor_hook.expected[1] = 0xE24DD080u;
    if (s->host.install_hook(&overworld_ctor_hook)) return -92;

    zero(&gen_unloaded_aabb_hook, sizeof(gen_unloaded_aabb_hook));
    gen_unloaded_aabb_hook.target = 0x00178E44u;
    gen_unloaded_aabb_hook.replacement = (u32)on_generate_unloaded_chunk_aabb;
    gen_unloaded_aabb_hook.expected[0] = 0xE92D4FF0u;
    gen_unloaded_aabb_hook.expected[1] = 0xE1A04001u;
    if (s->host.install_hook(&gen_unloaded_aabb_hook)) return -93;

    return 0;
}
