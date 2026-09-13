#include "block_read_fast.h"
#include "../../state.h"
#include "../../extension.h"
#include "../../world/extended_chunk_storage.h"

#ifndef NUMC3DS_OPTIMIZATIONS
#define NUMC3DS_OPTIMIZATIONS 1
#endif
#ifndef NUMC3DS_EXTENSION_FAST_READS
#define NUMC3DS_EXTENSION_FAST_READS 0
#endif

#if NUMC3DS_EXTENSION_FAST_READS
#define EXTENSION_TRY_BLOCK(source, position, id, data, accessor) \
    numc3ds_extension_try_block((source), (position), (id), (data), (accessor))
#define EXTENSION_NOTE_CALLER(source) \
    numc3ds_extension_note_block_caller( \
        (source), (u32)(unsigned long)__builtin_return_address(0))
#else
#define EXTENSION_TRY_BLOCK(source, position, id, data, accessor) 0
#define EXTENSION_NOTE_CALLER(source) ((void)0)
#endif

static NuMC3DS_Hook read_hooks[5];
static volatile u32 terrain_stock_readers;

static int fast_path_allowed(void)
{
#if !NUMC3DS_OPTIMIZATIONS
    return 0;
#else
    return __atomic_load_n(&terrain_stock_readers, __ATOMIC_ACQUIRE) == 0;
#endif
}

void block_read_terrain_scope_enter(void)
{
    __atomic_add_fetch(&terrain_stock_readers, 1u, __ATOMIC_ACQ_REL);
}

void block_read_terrain_scope_leave(void)
{
    __atomic_sub_fetch(&terrain_stock_readers, 1u, __ATOMIC_ACQ_REL);
}

void block_read_stock_id(u8 *result, const void *source, const int *position)
{
    ((void (*)(u8 *, const void *, const int *))read_hooks[0].trampoline)
        (result, source, position);
}

void block_read_stock_id_data(u8 *result, const void *source, const int *position)
{
    ((void (*)(u8 *, const void *, const int *))read_hooks[1].trampoline)
        (result, source, position);
}

static u32 block_index(const int *position)
{
    return (((u32)position[0] & 15u) << 8) |
           (((u32)position[2] & 15u) << 4) | ((u32)position[1] & 15u);
}

static const u8 *resolve_chunk(const u8 *source, const int *position)
{
    const u8 *chunk = *(const u8 * const *)(source + 0x34);
    int cx = position[0] >> 4;
    int cz = position[2] >> 4;
    if (chunk && *(const int *)(chunk + 0x20) == cx && *(const int *)(chunk + 0x24) == cz) {
        return chunk;
    }
    return (const u8 *)((void *(*)(const void *, const int *))0x00174964u)(source, position);
}

static const u8 *get_subchunk_for_read(const u8 *chunk, u32 section)
{
    if (!chunk) return 0;
    if (section < 8u) {
        if (section >= *(const u32 *)(chunk + 0x7c)) return 0;
        return *(const u8 * const *)(chunk + 0x5c + section * 4u);
    }
    if (section < 16u) {
        return (const u8 *)extended_chunk_get_subchunk(chunk, section);
    }
    return 0;
}

static void read_block_id(u8 *result, const u8 *source, const int *position)
{
    const u8 *chunk;
    const u8 *subchunk;
    int y;
    u32 section;
    int handled = EXTENSION_TRY_BLOCK(source, position, result, 0, 4u);
    if (handled) {
        if (handled == NUMC3DS_EXTENSION_REJECTED)
            EXTENSION_NOTE_CALLER(source);
        return;
    }
    if (!fast_path_allowed()) {
        block_read_stock_id(result, source, position);
        return;
    }
    y = position[1];
    if (y < 0 || y >= *(const short *)(source + 0x18)) { *result = 0; return; }
    section = (u32)y >> 4;
    chunk = resolve_chunk(source, position);
    subchunk = get_subchunk_for_read(chunk, section);
    if (!subchunk) {
        if (section >= 8u) { *result = 0; return; }
        ((void (*)(u8 *, const u8 *, const int *))read_hooks[0].trampoline)
            (result, source, position);
        return;
    }
    *result = subchunk[block_index(position)];
}

static void read_block_data(u8 *result, const u8 *source, const int *position)
{
    const u8 *chunk;
    const u8 *subchunk;
    int y;
    u32 section;
    u32 index;
    u8 data;
    int handled = EXTENSION_TRY_BLOCK(source, position, result, result + 1, 5u);
    if (handled) {
        if (handled == NUMC3DS_EXTENSION_REJECTED)
            EXTENSION_NOTE_CALLER(source);
        return;
    }
    if (!fast_path_allowed()) {
        block_read_stock_id_data(result, source, position);
        return;
    }
    y = position[1];
    if (y < 0 || y >= *(const short *)(source + 0x18)) { result[0] = 0; result[1] = 0; return; }
    section = (u32)y >> 4;
    chunk = resolve_chunk(source, position);
    subchunk = get_subchunk_for_read(chunk, section);
    if (!subchunk) {
        if (section >= 8u) { result[0] = 0; result[1] = 0; return; }
        ((void (*)(u8 *, const u8 *, const int *))read_hooks[1].trampoline)
            (result, source, position);
        return;
    }
    index = block_index(position);
    data = subchunk[0x1000 + (index >> 1)];
    result[0] = subchunk[index];
    result[1] = (data >> ((index & 1u) * 4u)) & 15u;
}

static u8 read_block_data_only(const u8 *source, const int *position)
{
    const u8 *chunk;
    const u8 *subchunk;
    int y;
    u32 section;
    u32 index;
    u8 data;
    int handled = EXTENSION_TRY_BLOCK(source, position, 0, &data, 6u);
    if (handled) {
        if (handled == NUMC3DS_EXTENSION_REJECTED)
            EXTENSION_NOTE_CALLER(source);
        return data;
    }
    if (!fast_path_allowed()) {
        return ((u8 (*)(const u8 *, const int *))read_hooks[2].trampoline)
            (source, position);
    }
    y = position[1];
    if (y < 0 || y >= *(const short *)(source + 0x18)) return 0;
    section = (u32)y >> 4;
    chunk = resolve_chunk(source, position);
    subchunk = get_subchunk_for_read(chunk, section);
    if (!subchunk) {
        if (section >= 8u) return 0;
        return ((u8 (*)(const u8 *, const int *))read_hooks[2].trampoline)
            (source, position);
    }
    index = block_index(position);
    data = subchunk[0x1000 + (index >> 1)];
    return (data >> ((index & 1u) * 4u)) & 15u;
}

static int is_air_pos(const u8 *source, const int *position)
{
    const u8 *chunk;
    const u8 *subchunk;
    int y;
    u32 section;
    u8 id;
    int handled = EXTENSION_TRY_BLOCK(source, position, &id, 0, 7u);
    if (handled) {
        if (handled == NUMC3DS_EXTENSION_REJECTED)
            EXTENSION_NOTE_CALLER(source);
        return id == 0;
    }
    if (!fast_path_allowed()) {
        return ((int (*)(const u8 *, const int *))read_hooks[3].trampoline)
            (source, position);
    }
    y = position[1];
    if (y < 0 || y >= *(const short *)(source + 0x18)) return 1;
    section = (u32)y >> 4;
    chunk = resolve_chunk(source, position);
    subchunk = get_subchunk_for_read(chunk, section);
    if (!subchunk) {
        if (section >= 8u) return 1;
        return ((int (*)(const u8 *, const int *))read_hooks[3].trampoline)
            (source, position);
    }
    return subchunk[block_index(position)] == 0;
}

static int is_air_xyz(const u8 *source, int x, int y, int z)
{
    int position[3];
    const u8 *chunk;
    const u8 *subchunk;
    u32 section;
    u32 idx;
    u8 id;
    position[0] = x;
    position[1] = y;
    position[2] = z;
    {
        int handled = EXTENSION_TRY_BLOCK(source, position, &id, 0, 8u);
        if (handled) {
            if (handled == NUMC3DS_EXTENSION_REJECTED)
                EXTENSION_NOTE_CALLER(source);
            return id == 0;
        }
    }
    if (!fast_path_allowed()) {
        return ((int (*)(const u8 *, int, int, int))read_hooks[4].trampoline)
            (source, x, y, z);
    }
    if (y < 0 || y >= *(const short *)(source + 0x18)) return 1;
    section = (u32)y >> 4;
    chunk = resolve_chunk(source, position);
    subchunk = get_subchunk_for_read(chunk, section);
    if (!subchunk) {
        if (section >= 8u) return 1;
        return ((int (*)(const u8 *, int, int, int))read_hooks[4].trampoline)
            (source, x, y, z);
    }
    idx = (((u32)x & 15u) << 8) | (((u32)z & 15u) << 4) | ((u32)y & 15u);
    return subchunk[idx] == 0;
}

int block_read_fast_install_hooks(void)
{
    u32 index;
    read_hooks[0].target = 0x00174648u;
    read_hooks[0].replacement = (u32)read_block_id;
    read_hooks[0].expected[0] = 0xE92D40F0u;
    read_hooks[0].expected[1] = 0xE1A04000u;

    read_hooks[1].target = 0x00176778u;
    read_hooks[1].replacement = (u32)read_block_data;
    read_hooks[1].expected[0] = 0xE92D40F0u;
    read_hooks[1].expected[1] = 0xE1A04000u;

    read_hooks[2].target = 0x0017A8C4u;
    read_hooks[2].replacement = (u32)read_block_data_only;
    read_hooks[2].expected[0] = 0xE92D4030u;
    read_hooks[2].expected[1] = 0xE1A05000u;

    read_hooks[3].target = 0x00175828u;
    read_hooks[3].replacement = (u32)is_air_pos;
    read_hooks[3].expected[0] = 0xE1A02001u;
    read_hooks[3].expected[1] = 0xE1A01000u;

    read_hooks[4].target = 0x00175850u;
    read_hooks[4].replacement = (u32)is_air_xyz;
    read_hooks[4].expected[0] = 0xE1A0C001u;
    read_hooks[4].expected[1] = 0xE52DE004u;

    for (index = 0; index < 5u; index++) {
        if (s->host.install_hook(&read_hooks[index])) return -45 - (int)index;
    }
    return 0;
}
