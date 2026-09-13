#include "chunk_tick_fast.h"
#include "../../state.h"

#define SEAM_Player_tickWorld       0x00607444u
#define SEAM_world_to_chunk_pos     0x0064CA34u
#define SEAM_wasTickedThisTick      0x00695E64u
#define SEAM_LevelChunk_tick        0x0015B678u
#define SEAM_LevelChunk_altTick     0x00158E40u
#define SEAM_player_post_tick       0x005F5FE4u
#define SEAM_chunk_offsets_table    0x00B161A8u

static NuMC3DS_Hook s_tick_hook;

typedef void (*WorldToChunkPosFn)(int *out_pos, const void *vec3_pos);
typedef int (*GetChunkFn)(void *source, const int *pos);
typedef int (*WasTickedFn)(void *chunk, int tickCount);
typedef void (*ChunkTickFn)(void *chunk, void *player, int tickCount);
typedef void (*ChunkAltTickFn)(void *chunk, void *player_field);
typedef void (*PlayerPostTickFn)(void *player, void *player_field);

static int player_tick_world_fast(void *player, int tickCount)
{
    if (!player) return 0;
    if (!*(u8 *)((u8 *)player + 0x1214)) return 0;

    int center_pos[2];
    ((WorldToChunkPosFn)SEAM_world_to_chunk_pos)(center_pos, (u8 *)player + 0x1c0);

    /* Entity::getLevel(player) returns *(void **)(player + 0x1a0) */
    void *level = *(void **)((u8 *)player + 0x1a0);
    if (!level) return 0;

    /* Dimension is at *(void **)(player + 0x11ec) */
    void *dimension = *(void **)((u8 *)player + 0x11ec);
    if (!dimension) return 0;

    void *chunk_source = *(void **)((u8 *)dimension + 0x10);
    if (!chunk_source) return 0;

    void **source_vtable = *(void ***)chunk_source;
    GetChunkFn get_chunk = (GetChunkFn)source_vtable[2]; /* [vtable + 8] */

    /* Hoist level flags outside the 57-chunk loop */
    int flag1 = (int)*(signed char *)((u8 *)level + 0x1674);
    int flag2 = (int)*(signed char *)((u8 *)level + 0x16fc);
    int use_alt_tick = (flag1 == 0 && flag2 != 0);

    void *player_field = *(void **)((u8 *)player + 0x210);
    const int *offsets = (const int *)SEAM_chunk_offsets_table;

    int cx = center_pos[0];
    int cz = center_pos[1];
    int chunk_tick_count = 0;

    for (int i = 0; i < 57; i++) {
        int pos[2];
        pos[0] = cx + offsets[i * 2];
        pos[1] = cz + offsets[i * 2 + 1];

        void *chunk = (void *)get_chunk(chunk_source, pos);
        if (!chunk) continue;

        int state = *(int *)((u8 *)chunk + 0x38);
        if (state < 2) continue;
        chunk_tick_count++;
        if (state < 6) continue;
        chunk_tick_count++;

        if (((WasTickedFn)SEAM_wasTickedThisTick)(chunk, tickCount)) continue;

        if (use_alt_tick) {
            ((ChunkAltTickFn)SEAM_LevelChunk_altTick)(chunk, player_field);
        } else {
            ((ChunkTickFn)SEAM_LevelChunk_tick)(chunk, player, tickCount);
        }
    }

    ((PlayerPostTickFn)SEAM_player_post_tick)(player, player_field);

    /* Check bed sleeping: lines 0x006075A0 - 0x00607604 */
    int is_client = (flag1 != 0);
    if (!is_client && *(u8 *)((u8 *)player + 0x18a8) == 0) {
        typedef int (*SleepTimerFn)(void *player);
        typedef void (*StopSleepFn)(void *player);
        int stop = 0;
        if (!*(u8 *)((u8 *)player + 0x18b1)) {
            stop = 1;
        } else {
            int timer = ((SleepTimerFn)0x00714334u)(player);
            if (timer <= 0 || *(u8 *)((u8 *)player + 0x1148) == 4) {
                stop = 1;
            }
        }
        if (stop) {
            ((StopSleepFn)0x00603028u)(player);
        }
    }

    /* Check entity interaction target: lines 0x00607604 - 0x006076B8 */
    if (*(u8 *)((u8 *)player + 0x18b1)) {
        u32 *uuid_ptr = (u32 *)((u8 *)player + 0x11d0);
        if (uuid_ptr[0] != 0xFFFFFFFFu || uuid_ptr[1] != 0xFFFFFFFFu) {
            typedef void *(*FindEntityFn)(void *level, int zero, u32 u1, u32 u2, int stack_zero);
            void *target = ((FindEntityFn)0x00720768u)(level, 0, uuid_ptr[0], uuid_ptr[1], 0);
            if (target) {
                void **target_vtable = *(void ***)target;
                typedef int (*CanInteractFn)(void *target, void *player);
                CanInteractFn can_interact = (CanInteractFn)target_vtable[0x220 / 4];
                if (can_interact(target, player)) {
                    typedef float (*DistanceToSqrFn)(void *target, void *player);
                    float dist_sq = ((DistanceToSqrFn)0x00723498u)(target, player);
                    if (dist_sq < 36.0f) {
                        void **player_vtable = *(void ***)player;
                        typedef void (*InteractFn)(void *player, void *target);
                        InteractFn interact = (InteractFn)player_vtable[0x94 / 4];
                        interact(player, target);
                    }
                }
            }
            uuid_ptr[0] = 0xFFFFFFFFu;
            uuid_ptr[1] = 0xFFFFFFFFu;
        }
    }

    return chunk_tick_count;
}

int chunk_tick_fast_install_hooks(void)
{
    s_tick_hook.target = SEAM_Player_tickWorld;
    s_tick_hook.replacement = (u32)player_tick_world_fast;
    s_tick_hook.expected[0] = 0xE92D47F0u;
    s_tick_hook.expected[1] = 0xE1A05000u;
    if (s->host.install_hook(&s_tick_hook)) return -81;
    return 0;
}
