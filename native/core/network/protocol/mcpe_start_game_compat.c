#include "mcpe_start_game_compat.h"
#include "../session/mcpe_transport_selector.h"
#include "../../../diagnostics/network_debug.h"
#include "../../commands/native_types.h"
#include "../../hook_manager.h"
#include "../../seams.h"
#include "../../state.h"

typedef unsigned long long u64;
typedef void (*StartGameHandlerFn)(void *, void *, void *);
typedef void (*StartGamePacketReadFn)(void *, void *);
typedef int (*AllowIncomingPacketIdFn)(void *, void *, u32);
typedef void (*RequestChunkRadiusFn)(void *, u32);
typedef u32 (*ReadVarIntFn)(void *);
typedef u8 (*GetBoolFn)(void *);
typedef float (*GetFloatFn)(void *);
typedef void *(*GetStringFn)(void *, void *);
typedef u64 (*GetUnsignedInt64Fn)(void *);
typedef struct {
    void *begin;
    void *end;
    void *capacity;
} GameRuleWireVector;

typedef void (*ReadGameRulesVectorFn)(void *, void *);
typedef void (*SetRulesFromVectorFn)(void *, void *);
typedef void (*OperatorDeleteFn)(void *);
typedef void *(*StringAssignFn)(void *, const void *);
typedef void (*StringDtorFn)(void *);
typedef void (*GetEntityIdFn)(void *, void *);

static NuMC3DS_Hook start_game_hook;
static NuMC3DS_Hook start_game_read_hook;
static NuMC3DS_Hook allow_incoming_hook;
static NuMC3DS_Hook start_game_creative_hook;

static int on_allow_incoming_packet_id(void *handler, void *identifier, u32 packet_id)
{
    AllowIncomingPacketIdFn original;

    if (mcpe_transport_selector_kind() == MCPE_TRANSPORT_IPV4) {
        return 1;
    }

    original = (AllowIncomingPacketIdFn)allow_incoming_hook.trampoline;
    return original(handler, identifier, packet_id);
}

static void on_start_game_packet_read(void *packet, void *stream)
{
    StartGamePacketReadFn original;
    NativeGstdString temp_str;
    GameRuleWireVector rules_vec;

    if (mcpe_transport_selector_kind() != MCPE_TRANSPORT_IPV4) {
        original = (StartGamePacketReadFn)start_game_read_hook.trampoline;
        original(packet, stream);
        return;
    }

    net_log_open(NET_LOG_INFO, "start_game", "read_begin");
    net_log_hex("packet", (u32)packet);
    net_log_hex("stream", (u32)stream);
    net_log_close();

    ((GetEntityIdFn)0x007E1B3C)((u8 *)packet + 176, stream);
    ((GetEntityIdFn)0x007E1B54)((u8 *)packet + 184, stream);
    *(u32 *)((u8 *)packet + 192) = ((ReadVarIntFn)SEAM_ReadOnlyBinaryStream_readVarInt)(stream);
    *(float *)((u8 *)packet + 196) = ((GetFloatFn)SEAM_ReadOnlyBinaryStream_getFloat)(stream);
    *(float *)((u8 *)packet + 200) = ((GetFloatFn)SEAM_ReadOnlyBinaryStream_getFloat)(stream);
    *(float *)((u8 *)packet + 204) = ((GetFloatFn)SEAM_ReadOnlyBinaryStream_getFloat)(stream);
    *(float *)((u8 *)packet + 208) = ((GetFloatFn)SEAM_ReadOnlyBinaryStream_getFloat)(stream);
    *(float *)((u8 *)packet + 212) = ((GetFloatFn)SEAM_ReadOnlyBinaryStream_getFloat)(stream);

    u32 seed;
    u32 dimension;
    u32 generator;
    u32 gameType;
    u32 difficulty;
    u32 spawn_x, spawn_y, spawn_z;
    u8 hasAchievementsDisabled;
    u32 time;
    u8 eduWorld;
    float rainLevel;
    float lightningLevel;
    u8 commandsEnabled;
    u8 texturepacksRequired;

    seed = ((ReadVarIntFn)SEAM_ReadOnlyBinaryStream_readVarInt)(stream);
    dimension = ((ReadVarIntFn)SEAM_ReadOnlyBinaryStream_readVarInt)(stream);
    generator = ((ReadVarIntFn)SEAM_ReadOnlyBinaryStream_readVarInt)(stream);
    gameType = ((ReadVarIntFn)SEAM_ReadOnlyBinaryStream_readVarInt)(stream);
    difficulty = ((ReadVarIntFn)SEAM_ReadOnlyBinaryStream_readVarInt)(stream);
    spawn_x = ((ReadVarIntFn)SEAM_ReadOnlyBinaryStream_readVarInt)(stream);
    spawn_y = ((ReadVarIntFn)SEAM_ReadOnlyBinaryStream_getUnsignedVarInt)(stream);
    spawn_z = ((ReadVarIntFn)SEAM_ReadOnlyBinaryStream_readVarInt)(stream);
    hasAchievementsDisabled = ((GetBoolFn)SEAM_ReadOnlyBinaryStream_getBool)(stream);
    time = ((ReadVarIntFn)SEAM_ReadOnlyBinaryStream_readVarInt)(stream);
    eduWorld = ((GetBoolFn)SEAM_ReadOnlyBinaryStream_getBool)(stream);
    rainLevel = ((GetFloatFn)SEAM_ReadOnlyBinaryStream_getFloat)(stream);
    lightningLevel = ((GetFloatFn)SEAM_ReadOnlyBinaryStream_getFloat)(stream);
    commandsEnabled = ((GetBoolFn)SEAM_ReadOnlyBinaryStream_getBool)(stream);
    texturepacksRequired = ((GetBoolFn)SEAM_ReadOnlyBinaryStream_getBool)(stream);

    if (spawn_y >= 32767u) {
        spawn_y = 64u;
    }
    if (*(float *)((u8 *)packet + 200) >= 32767.0f || *(float *)((u8 *)packet + 200) < 0.0f) {
        *(float *)((u8 *)packet + 200) = (float)spawn_y;
    }

    *(u32 *)((u8 *)packet + 8 + 0x00) = seed;
    *(u32 *)((u8 *)packet + 8 + 0x04) = gameType;
    *(u32 *)((u8 *)packet + 8 + 0x08) = difficulty;
    *(u8 *)((u8 *)packet + 8 + 0x0C) = (u8)difficulty;
    *(u32 *)((u8 *)packet + 8 + 0x10) = generator;
    *(u8 *)((u8 *)packet + 8 + 0x14) = hasAchievementsDisabled;
    *(u8 *)((u8 *)packet + 8 + 0x15) = 1;
    *(u32 *)((u8 *)packet + 8 + 0x18) = dimension;
    *(u32 *)((u8 *)packet + 8 + 0x1C) = time;
    *(u8 *)((u8 *)packet + 8 + 0x20) = eduWorld;
    *(u8 *)((u8 *)packet + 8 + 0x21) = 1;
    *(u8 *)((u8 *)packet + 8 + 0x22) = 1;
    *(u8 *)((u8 *)packet + 8 + 0x23) = 1;
    *(u8 *)((u8 *)packet + 8 + 0x24) = commandsEnabled;
    *(u8 *)((u8 *)packet + 8 + 0x25) = texturepacksRequired;
    *(u32 *)((u8 *)packet + 8 + 0x28) = spawn_x;
    *(u32 *)((u8 *)packet + 8 + 0x2C) = spawn_y;
    *(u32 *)((u8 *)packet + 8 + 0x30) = spawn_z;
    *(float *)((u8 *)packet + 8 + 0x48) = rainLevel;
    *(float *)((u8 *)packet + 8 + 0x4C) = lightningLevel;

    rules_vec.begin = 0;
    rules_vec.end = 0;
    rules_vec.capacity = 0;
    ((ReadGameRulesVectorFn)SEAM_GameRules_readVector)(&rules_vec, stream);
    ((SetRulesFromVectorFn)SEAM_GameRules_setRulesFromVector)((u8 *)packet + 8 + 0x88, &rules_vec);

    if (rules_vec.begin) {
        u8 *cur = (u8 *)rules_vec.begin;
        u8 *end = (u8 *)rules_vec.end;
        while (cur < end) {
            ((StringDtorFn)SEAM_GstdString_dtor)(cur);
            cur += 12;
        }
        ((OperatorDeleteFn)SEAM_operator_delete)(rules_vec.begin);
    }

    zero(&temp_str, sizeof(temp_str));
    ((GetStringFn)SEAM_ReadOnlyBinaryStream_getString)(&temp_str, stream);
    ((StringAssignFn)SEAM_GstdString_assign)((u8 *)packet + 216, &temp_str);
    ((StringDtorFn)SEAM_GstdString_dtor)(&temp_str);

    zero(&temp_str, sizeof(temp_str));
    ((GetStringFn)SEAM_ReadOnlyBinaryStream_getString)(&temp_str, stream);
    ((StringAssignFn)SEAM_GstdString_assign)((u8 *)packet + 220, &temp_str);
    ((StringDtorFn)SEAM_GstdString_dtor)(&temp_str);

    zero(&temp_str, sizeof(temp_str));
    ((GetStringFn)SEAM_ReadOnlyBinaryStream_getString)(&temp_str, stream);
    ((StringAssignFn)SEAM_GstdString_assign)((u8 *)packet + 224, &temp_str);
    ((StringDtorFn)SEAM_GstdString_dtor)(&temp_str);

    *(u8 *)((u8 *)packet + 228) = ((GetBoolFn)SEAM_ReadOnlyBinaryStream_getBool)(stream);
    *(u64 *)((u8 *)packet + 232) = ((GetUnsignedInt64Fn)SEAM_ReadOnlyBinaryStream_getUnsignedInt64)(stream);

    net_log_open(NET_LOG_INFO, "start_game", "read_complete");
    net_log_dec("game_type", *(u32 *)((u8 *)packet + 192));
    net_log_signed("x", (s32)*(float *)((u8 *)packet + 196));
    net_log_signed("y", (s32)*(float *)((u8 *)packet + 200));
    net_log_signed("z", (s32)*(float *)((u8 *)packet + 204));
    net_log_dec("sx", spawn_x);
    net_log_dec("sy", spawn_y);
    net_log_dec("sz", spawn_z);
    net_log_dec("sxw", *(u32 *)((u8 *)packet + 0x30u));
    net_log_dec("syw", *(u32 *)((u8 *)packet + 0x34u));
    net_log_dec("szw", *(u32 *)((u8 *)packet + 0x38u));
    net_log_dec("dim", dimension);
    net_log_dec("gen", generator);
    net_log_close();
}

static void on_start_game(void *handler, void *identifier, void *packet)
{
    StartGameHandlerFn original;
    void *client;
    void *player;
    void **storage;

    net_log_open(NET_LOG_INFO, "start_game", "handle_begin");
    net_log_hex("handler", (u32)handler);
    net_log_hex("packet", (u32)packet);
    net_log_close();

    original = (StartGameHandlerFn)start_game_hook.trampoline;
    original(handler, identifier, packet);

    net_log_open(NET_LOG_INFO, "start_game", "handle_original_done");
    net_log_close();

    if (mcpe_transport_selector_kind() != MCPE_TRANSPORT_IPV4 || !handler || !identifier) {
        return;
    }

    *(u8 *)((u8 *)handler + 29u) = 1;

    storage = (void **)0x00B0D744u;
    if (!storage[0] || storage[0] == storage[1]) {
        ((void (*)(void))0x0056E450u)();
    }

    client = *(void **)((u8 *)handler + 36u);
    if (client) {
        player = *(void **)((u8 *)client + 48u);
        if (player) {
            void *region = *(void **)((u8 *)player + 0x210u);
            if (region) {
                void *dim = *(void **)((u8 *)region + 0x14u);
                if (dim) {
                    *(u16 *)((u8 *)dim + 0x88) = 256;
                    *(u16 *)((u8 *)region + 0x18) = 256;
                }
            }
            ((RequestChunkRadiusFn)SEAM_LocalPlayer_requestChunkRadius)(player, 4u);
            net_log_open(NET_LOG_INFO, "start_game", "local_player_radius_requested");
            net_log_hex("player", (u32)player);
            net_log_signed("px", (s32)*(float *)((u8 *)player + 0x1c0u));
            net_log_signed("py", (s32)*(float *)((u8 *)player + 0x1c4u));
            net_log_signed("pz", (s32)*(float *)((u8 *)player + 0x1c8u));
            net_log_close();
        }
    }
}

int mcpe_start_game_should_skip_creative_clear(void)
{
    return mcpe_transport_selector_kind() == MCPE_TRANSPORT_IPV4;
}

__attribute__((naked)) static void on_start_game_creative_clear(void)
{
    __asm__ volatile (
        "push {r0-r3, r12, lr}\n"
        "bl mcpe_start_game_should_skip_creative_clear\n"
        "cmp r0, #0\n"
        "pop {r0-r3, r12, lr}\n"
        "bne 1f\n"
        "ldr fp, =0x00B0D744\n"
        "ldr r9, [fp, #4]\n"
        "ldr pc, =0x0048D7E8\n"
        "1:\n"
        "ldr pc, =0x0048D8C4\n"
    );
}

int mcpe_start_game_compat_install(void)
{
    zero(&start_game_hook, sizeof(start_game_hook));
    start_game_hook.target = SEAM_ClientNetworkHandler_handleStartGamePacket;
    start_game_hook.replacement = (u32)on_start_game;
    start_game_hook.expected[0] = 0xE92D4FF0u;
    start_game_hook.expected[1] = 0xE1A04000u;
    if (s->host.install_hook(&start_game_hook)) return -85;

    zero(&start_game_read_hook, sizeof(start_game_read_hook));
    start_game_read_hook.target = SEAM_StartGamePacket_read;
    start_game_read_hook.replacement = (u32)on_start_game_packet_read;
    start_game_read_hook.expected[0] = 0xE92D40F0u;
    start_game_read_hook.expected[1] = 0xE24DD0CCu;
    if (s->host.install_hook(&start_game_read_hook)) return -86;

    zero(&allow_incoming_hook, sizeof(allow_incoming_hook));
    allow_incoming_hook.target = SEAM_ClientNetworkHandler_allowIncomingPacketId;
    allow_incoming_hook.replacement = (u32)on_allow_incoming_packet_id;
    allow_incoming_hook.expected[0] = 0xE5D0001Du;
    allow_incoming_hook.expected[1] = 0xE3500000u;
    if (s->host.install_hook(&allow_incoming_hook)) return -87;

    zero(&start_game_creative_hook, sizeof(start_game_creative_hook));
    start_game_creative_hook.target = 0x0048D7E0u;
    start_game_creative_hook.replacement = (u32)on_start_game_creative_clear;
    start_game_creative_hook.expected[0] = 0xE59FB2B4u;
    start_game_creative_hook.expected[1] = 0xE59B9004u;
    if (s->host.install_hook(&start_game_creative_hook)) return -88;

    return 0;
}
