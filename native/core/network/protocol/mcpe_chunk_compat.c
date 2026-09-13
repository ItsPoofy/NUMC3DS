#include "mcpe_chunk_compat.h"
#include "../session/mcpe_transport_selector.h"
#include "../../hook_manager.h"
#include "../../seams.h"
#include "../../state.h"
#include "../../../diagnostics/network_debug.h"
#include "../../world/extended_chunk_storage.h"

typedef void (*LevelChunkNewSubChunkFn)(void *, unsigned int, int);
typedef void *(*BrightnessStorageAllocFn)(u32);
typedef u32 (*StreamReadByteFn)(void *);
typedef u32 (*StreamReadBytesFn)(void *, void *, u32);
typedef void (*FullChunkHandlerFn)(void *, void *, void *);
typedef u32 (*TryChangeStateFn)(void *, u32, u32, u32);
typedef void (*FireAreaFn)(void *, void *, void *);
typedef void (*SetFinalizedFn)(void *, u32);
typedef void (*MovePlayerReadFn)(void *, void *);
typedef void *(*WorldLimitRequestChunkFn)(void *, const int *, int);
typedef void *(*WorldLimitGetExistingChunkFn)(void *, const int *);

static NuMC3DS_Hook deserialize_lighting_hook;
static NuMC3DS_Hook levelchunk_deserialize_subchunk_hook;
static NuMC3DS_Hook full_chunk_handler_hook;
static NuMC3DS_Hook try_change_state_hook;
static NuMC3DS_Hook fire_area_hook;
static NuMC3DS_Hook set_finalized_hook;
static NuMC3DS_Hook move_player_read_hook;
static NuMC3DS_Hook world_limit_request_hook;
static NuMC3DS_Hook world_limit_existing_hook;

static u8 s_lighting_scratch[4096];
static u32 s_subchunk_call_count;
static u32 s_lighting_call_count;
static u32 s_packet_call_count;
static u32 s_last_chunk;
static u32 s_network_chunks_seen;
static u32 s_state_log_count;
static u32 s_fire_log_count;
static u32 s_final_log_count;
static u32 s_move_log_count;

static void *on_world_limit_request_chunk(void *source, const int *chunk_pos, int mode)
{
    WorldLimitRequestChunkFn original;
    void *wrapped;

    if (mcpe_transport_selector_kind() == MCPE_TRANSPORT_IPV4 && source) {
        wrapped = *(void **)((u8 *)source + 16u);
        if (wrapped) {
            void **vtable = *(void ***)wrapped;
            return ((void *(*)(void *, const int *, int))vtable[3])(wrapped, chunk_pos, mode);
        }
    }

    original = (WorldLimitRequestChunkFn)world_limit_request_hook.trampoline;
    return original(source, chunk_pos, mode);
}

static void *on_world_limit_get_existing_chunk(void *source, const int *chunk_pos)
{
    WorldLimitGetExistingChunkFn original;
    void *wrapped;

    if (mcpe_transport_selector_kind() == MCPE_TRANSPORT_IPV4 && source) {
        wrapped = *(void **)((u8 *)source + 16u);
        if (wrapped) {
            void **vtable = *(void ***)wrapped;
            return ((void *(*)(void *, const int *))vtable[2])(wrapped, chunk_pos);
        }
    }

    original = (WorldLimitGetExistingChunkFn)world_limit_existing_hook.trampoline;
    return original(source, chunk_pos);
}

static void on_move_player_read(void *packet, void *stream)
{
    MovePlayerReadFn original;

    original = (MovePlayerReadFn)move_player_read_hook.trampoline;
    original(packet, stream);
    if (s_move_log_count < 8u) {
        s_move_log_count++;
        net_log_open(NET_LOG_INFO, "chunk", "move_player");
        net_log_dec("n", s_move_log_count);
        net_log_signed("x", (s32)*(float *)((u8 *)packet + 0x10u));
        net_log_signed("y", (s32)*(float *)((u8 *)packet + 0x14u));
        net_log_signed("z", (s32)*(float *)((u8 *)packet + 0x18u));
        net_log_dec("mode", *(u8 *)((u8 *)packet + 0x28u));
        net_log_dec("onground", *(u8 *)((u8 *)packet + 0x29u));
        net_log_close();
    }
}

static u32 on_try_change_state(void *chunk, u32 expected, u32 desired, u32 extra)
{
    TryChangeStateFn original;
    u32 result;

    original = (TryChangeStateFn)try_change_state_hook.trampoline;
    result = original(chunk, expected, desired, extra);
    if (s_network_chunks_seen && s_state_log_count < 8u) {
        s_state_log_count++;
        net_log_open(NET_LOG_INFO, "chunk", "try_change_state");
        net_log_hex("chunk", (u32)chunk);
        net_log_dec("want", desired);
        net_log_dec("result", result);
        net_log_close();
    }
    return result;
}

static void on_fire_area(void *target, void *min, void *max)
{
    FireAreaFn original;

    original = (FireAreaFn)fire_area_hook.trampoline;
    if (s_network_chunks_seen && s_fire_log_count < 8u) {
        s_fire_log_count++;
        net_log_open(NET_LOG_INFO, "chunk", "fire_area");
        net_log_hex("target", (u32)target);
        net_log_hex("min", (u32)min);
        net_log_hex("max", (u32)max);
        net_log_close();
    }
    original(target, min, max);
}

static void on_set_finalized(void *chunk, u32 state)
{
    SetFinalizedFn original;

    original = (SetFinalizedFn)set_finalized_hook.trampoline;
    if (s_network_chunks_seen && s_final_log_count < 8u) {
        s_final_log_count++;
        net_log_open(NET_LOG_INFO, "chunk", "set_finalized");
        net_log_hex("chunk", (u32)chunk);
        net_log_dec("state", state);
        net_log_close();
    }
    original(chunk, state);
}

static void log_subchunk_sample(void *chunk, unsigned int subchunk_index, u8 *subchunk, u32 block_sum, u32 first4, u32 reads_ok)
{
    net_log_open(NET_LOG_INFO, "chunk", "subchunk");
    net_log_dec("n", s_subchunk_call_count);
    net_log_dec("idx", subchunk_index);
    net_log_hex("chunk", (u32)chunk);
    net_log_hex("sub", (u32)subchunk);
    net_log_dec("subs", *(u32 *)((u8 *)chunk + 0x7cu));
    net_log_dec("sum", block_sum);
    net_log_hex("first4", first4);
    net_log_flag("reads", (int)reads_ok);
    net_log_close();
}

static void on_levelchunk_deserialize_subchunk(void *chunk, unsigned int subchunk_index, void *stream)
{
    StreamReadByteFn read_byte;
    StreamReadBytesFn read_bytes;
    u8 *subchunk;
    u8 *light_buf;
    const u8 *sky_light;
    const u8 *block_light;
    unsigned int i;
    int detailed;
    u32 reads_ok;
    u32 block_sum;
    u32 first4;

    if (!chunk || !stream) return;
    read_byte = (*(StreamReadByteFn **)stream)[6];
    read_bytes = (*(StreamReadBytesFn **)stream)[10];
    if (!read_byte || !read_bytes) return;

    s_subchunk_call_count++;
    detailed = s_subchunk_call_count <= 16u;
    s_last_chunk = (u32)chunk;

    if (subchunk_index < 8u) {
        ((LevelChunkNewSubChunkFn)SEAM_LevelChunk_newSubChunk)(chunk, subchunk_index, 0);
        if (*(u32 *)((u8 *)chunk + 0x7cu) <= subchunk_index ||
            !(subchunk = *(u8 **)((u8 *)chunk + 0x5cu + subchunk_index * 4u))) {
            subchunk = 0;
        }
    } else if (subchunk_index < 16u) {
        subchunk = extended_chunk_get_or_create_subchunk(chunk, subchunk_index);
    } else {
        subchunk = 0;
    }

    if (!subchunk) {
        read_byte(stream);
        reads_ok = read_bytes(stream, s_lighting_scratch, 4096u);
        reads_ok &= read_bytes(stream, s_lighting_scratch, 2048u);
        reads_ok &= read_bytes(stream, s_lighting_scratch, 2048u);
        reads_ok &= read_bytes(stream, s_lighting_scratch, 2048u);
        if (detailed) log_subchunk_sample(chunk, subchunk_index, 0, 0u, 0u, reads_ok);
        return;
    }

    read_byte(stream);
    reads_ok = read_bytes(stream, subchunk, 4096u);
    reads_ok &= read_bytes(stream, subchunk + 4096u, 2048u);
    reads_ok &= read_bytes(stream, s_lighting_scratch, 2048u);
    reads_ok &= read_bytes(stream, s_lighting_scratch + 2048u, 2048u);

    if (detailed) {
        block_sum = 0;
        for (i = 0; i < 4096u; ++i) block_sum += subchunk[i];
        first4 = (u32)subchunk[0] | ((u32)subchunk[1] << 8) | ((u32)subchunk[2] << 16) | ((u32)subchunk[3] << 24);
        log_subchunk_sample(chunk, subchunk_index, subchunk, block_sum, first4, reads_ok);
    }

    light_buf = *(u8 **)(subchunk + 0x1800u);
    if (!light_buf) {
        light_buf = (u8 *)((BrightnessStorageAllocFn)SEAM_SubChunkBrightnessStorage_allocChecked)(4096u);
        if (light_buf) *(u8 **)(subchunk + 0x1800u) = light_buf;
    }
    if (!light_buf) return;

    sky_light = s_lighting_scratch;
    block_light = s_lighting_scratch + 2048u;
    for (i = 0; i < 2048u; ++i) {
        u8 sky = sky_light[i];
        u8 block = block_light[i];
        light_buf[i * 2u] = (u8)(((sky & 0x0fu) << 4) | (block & 0x0fu));
        light_buf[i * 2u + 1u] = (u8)((sky & 0xf0u) | ((block >> 4) & 0x0fu));
    }
}

static void on_deserialize_subchunk_lighting(void *chunk, unsigned int subchunk_index, void *stream)
{
    (void)chunk;
    (void)subchunk_index;
    (void)stream;
    s_lighting_call_count++;
    if (s_lighting_call_count <= 16u) {
        net_log_open(NET_LOG_INFO, "chunk", "lighting_noop");
        net_log_dec("n", s_lighting_call_count);
        net_log_dec("idx", subchunk_index);
        net_log_close();
    }
}

static void on_full_chunk_handler(void *handler, void *identifier, void *packet)
{
    FullChunkHandlerFn original;
    u32 chunk;

    original = (FullChunkHandlerFn)full_chunk_handler_hook.trampoline;
    s_packet_call_count++;
    s_last_chunk = 0;
    original(handler, identifier, packet);
    s_network_chunks_seen = 1u;

    chunk = s_last_chunk;
    if (chunk) {
        u32 client = *(u32 *)((u8 *)handler + 0x24u);
        u32 player = client ? *(u32 *)((u8 *)client + 0x30u) : 0u;
        u32 region = player ? *(u32 *)((u8 *)player + 0x210u) : 0u;
        if (region) {
            void *min_pos = (void *)((u8 *)chunk + 8u);
            void *max_pos = (void *)((u8 *)chunk + 0x14u);
            ((FireAreaFn)0x00176658u)((void *)region, min_pos, max_pos);
        }
    }

    if (s_packet_call_count <= 8u) {
        chunk = s_last_chunk;
        net_log_open(NET_LOG_INFO, "chunk", "packet");
        net_log_dec("n", s_packet_call_count);
        net_log_signed("cx", *(s32 *)((u8 *)packet + 8u));
        net_log_signed("cz", *(s32 *)((u8 *)packet + 0xcu));
        net_log_hex("chunk", chunk);
        if (chunk) {
            net_log_dec("subs", *(u32 *)((u8 *)chunk + 0x7cu));
            net_log_dec("state", *(u32 *)((u8 *)chunk + 0x38u));
        }
        {
            u32 client = *(u32 *)((u8 *)handler + 0x24u);
            u32 player = client ? *(u32 *)((u8 *)client + 0x30u) : 0u;
            if (player) {
                float px = *(float *)((u8 *)player + 0x1c0u);
                float py = *(float *)((u8 *)player + 0x1c4u);
                float pz = *(float *)((u8 *)player + 0x1c8u);
                u32 region = *(u32 *)((u8 *)player + 0x210u);
                u32 dim = region ? *(u32 *)((u8 *)region + 0x14u) : 0u;
                u32 source = dim ? *(u32 *)((u8 *)dim + 0x138u) : 0u;
                net_log_signed("px", (s32)px);
                net_log_signed("py", (s32)py);
                net_log_signed("pz", (s32)pz);
                net_log_signed("pcx", (s32)(px * 0.0625f));
                net_log_signed("pcz", (s32)(pz * 0.0625f));
                net_log_hex("player", player);
                net_log_hex("region", region);
                net_log_hex("source", source);
            }
        }
        net_log_close();
    }
}

int mcpe_chunk_compat_install(void)
{
    if (*(volatile u32 *)0x0048F94Cu == 0xEA000008u) {
        *(volatile u32 *)0x0048F94Cu = 0xE320F000u;
    }

    zero(&full_chunk_handler_hook, sizeof(full_chunk_handler_hook));
    full_chunk_handler_hook.target = SEAM_ClientNetworkHandler_handleFullChunkDataPacket;
    full_chunk_handler_hook.replacement = (u32)on_full_chunk_handler;
    full_chunk_handler_hook.expected[0] = 0xE92D4FF0u;
    full_chunk_handler_hook.expected[1] = 0xE1A04000u;
    if (s->host.install_hook(&full_chunk_handler_hook)) return -86;

    zero(&try_change_state_hook, sizeof(try_change_state_hook));
    try_change_state_hook.target = SEAM_LevelChunk_tryChangeState;
    try_change_state_hook.replacement = (u32)on_try_change_state;
    try_change_state_hook.expected[0] = 0xE92D4007u;
    try_change_state_hook.expected[1] = 0xE28D1004u;
    if (s->host.install_hook(&try_change_state_hook)) return -90;

    zero(&fire_area_hook, sizeof(fire_area_hook));
    fire_area_hook.target = 0x00176658u;
    fire_area_hook.replacement = (u32)on_fire_area;
    fire_area_hook.expected[0] = 0xE92D40F0u;
    fire_area_hook.expected[1] = 0xE24DD00Cu;
    if (s->host.install_hook(&fire_area_hook)) return -91;

    zero(&set_finalized_hook, sizeof(set_finalized_hook));
    set_finalized_hook.target = SEAM_LevelChunk_setFinalized;
    set_finalized_hook.replacement = (u32)on_set_finalized;
    set_finalized_hook.expected[0] = 0xE92D4030u;
    set_finalized_hook.expected[1] = 0xE1A05000u;
    if (s->host.install_hook(&set_finalized_hook)) return -92;

    zero(&move_player_read_hook, sizeof(move_player_read_hook));
    move_player_read_hook.target = 0x00332608u;
    move_player_read_hook.replacement = (u32)on_move_player_read;
    move_player_read_hook.expected[0] = 0xE92D4070u;
    move_player_read_hook.expected[1] = 0xE24DD008u;
    if (s->host.install_hook(&move_player_read_hook)) return -93;

    zero(&levelchunk_deserialize_subchunk_hook, sizeof(levelchunk_deserialize_subchunk_hook));
    levelchunk_deserialize_subchunk_hook.target = SEAM_LevelChunk_deserializeSubChunk;
    levelchunk_deserialize_subchunk_hook.replacement = (u32)on_levelchunk_deserialize_subchunk;
    levelchunk_deserialize_subchunk_hook.expected[0] = 0xE92D4070u;
    levelchunk_deserialize_subchunk_hook.expected[1] = 0xE1A05002u;
    if (s->host.install_hook(&levelchunk_deserialize_subchunk_hook)) return -87;

    zero(&deserialize_lighting_hook, sizeof(deserialize_lighting_hook));
    deserialize_lighting_hook.target = SEAM_LevelChunk_deserializeSubChunkLighting;
    deserialize_lighting_hook.replacement = (u32)on_deserialize_subchunk_lighting;
    deserialize_lighting_hook.expected[0] = 0xE92D4070u;
    deserialize_lighting_hook.expected[1] = 0xE1A05000u;
    if (s->host.install_hook(&deserialize_lighting_hook)) return -89;

    zero(&world_limit_request_hook, sizeof(world_limit_request_hook));
    world_limit_request_hook.target = 0x004416B4u;
    world_limit_request_hook.replacement = (u32)on_world_limit_request_chunk;
    world_limit_request_hook.expected[0] = 0xE92D0030u;
    world_limit_request_hook.expected[1] = 0xE591C000u;
    if (s->host.install_hook(&world_limit_request_hook)) return -94;

    zero(&world_limit_existing_hook, sizeof(world_limit_existing_hook));
    world_limit_existing_hook.target = 0x00441728u;
    world_limit_existing_hook.replacement = (u32)on_world_limit_get_existing_chunk;
    world_limit_existing_hook.expected[0] = 0xE5913000u;
    world_limit_existing_hook.expected[1] = 0xE590C01Cu;
    if (s->host.install_hook(&world_limit_existing_hook)) return -95;

    return 0;
}
