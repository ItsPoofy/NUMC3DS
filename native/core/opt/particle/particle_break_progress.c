#include "particle_break_progress.h"
#include "../world/block_read_fast.h"
#include "../../state.h"

typedef void (*LevelEventFn)(void *, int, const float *, int);
typedef int *(*BlockPosCtorVec3Fn)(int *, const float *);
typedef void (*LevelAddTerrainParticleFn)(
    void *, const int *, const u8 *, const float *, int);

enum { LEVEL_EVENT_BLOCK_CRACK = 3600 };

static NuMC3DS_Hook break_progress_hook;

static void level_event_break_progress(
    void *renderer_player, int event_id, const float *position, int data)
{
    int block_position[3];
    u8 block[2];
    const float motion[3] = { 0.0f, 0.0f, 0.0f };
    void *player;
    void *level;
    void *source;

    ((LevelEventFn)break_progress_hook.trampoline)(
        renderer_player, event_id, position, data);
    if (event_id != LEVEL_EVENT_BLOCK_CRACK ||
        !renderer_player || !position)
        return;

    player = *(void **)((u8 *)renderer_player + 0x0f18);
    level = *(void **)((u8 *)renderer_player + 0x0f24);
    source = player ? *(void **)((u8 *)player + 0x0210) : 0;
    if (!level || !source) return;

    ((BlockPosCtorVec3Fn)0x00582C7Cu)(block_position, position);
    block_read_stock_id_data(block, source, block_position);
    if (!block[0]) return;

    ((LevelAddTerrainParticleFn)0x005C7BA4u)(
        level, block_position, block, motion, 2);
}

int particle_break_progress_install_hook(void)
{
    break_progress_hook.target = 0x003C9FE8u;
    break_progress_hook.replacement = (u32)level_event_break_progress;
    break_progress_hook.expected[0] = 0xE92D4FF0u;
    break_progress_hook.expected[1] = 0xE1A04000u;
    return s->host.install_hook(&break_progress_hook);
}
