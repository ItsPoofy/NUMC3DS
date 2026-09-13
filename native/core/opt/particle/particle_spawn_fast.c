#include "particle_spawn_fast.h"
#include "../../state.h"

typedef void (*LevelAddTerrainParticleFn)(
    void *level,
    const int *block_pos,
    const u8 *block_id_data,
    const float *in_motion,
    int count);

typedef int (*LevelListenerAddParticleFn)(
    void *listener,
    int particle_type,
    const float *pos,
    const float *motion,
    u32 data);

typedef int (*BlockIsTypeFn)(void *block, void *other);
typedef void *(*AchievementManagerGetInstanceFn)(void);
typedef void (*AchievementManagerUnlockFn)(void *manager, int achievement_id);
typedef int (*BlockHasPropertyFn)(void *block, int property_mask);

#define Block_isType ((BlockIsTypeFn)0x0071F980u)
#define AchievementManager_getInstance ((AchievementManagerGetInstanceFn)0x00228EB8u)
#define AchievementManager_unlock ((AchievementManagerUnlockFn)0x0042BFD0u)
#define Block_hasProperty ((BlockHasPropertyFn)0x0071DF24u)
#define BlockPos_randomFloat ((float (*)(const int *))0x007319F0u)

static NuMC3DS_Hook particle_spawn_hook;

void level_add_terrain_particle_fast(
    void *level,
    const int *block_pos,
    const u8 *block_id_data,
    const float *in_motion,
    int count)
{
    if (!level || !block_pos || !block_id_data || !in_motion) return;

    u8 bid = block_id_data[0];
    if (bid == 0) return;

    void **mBlocks = (void **)0x00B10520u;
    void *block = mBlocks[bid];
    if (!block) return;

    void *glass = *(void **)0x00A34770u;
    void *ice   = *(void **)0x00A349D0u;
    if (Block_isType(block, glass) || Block_isType(block, ice)) {
        void *ach = AchievementManager_getInstance();
        if (ach) {
            AchievementManager_unlock(ach, 0xE);
        }
    }

    if (count < 1) {
        if (Block_hasProperty(block, 0x200000)) {
            count = 5;
        } else {
            count = 4;
        }
    }

    void **listeners_begin = *(void ***)((u8 *)level + 0x74);
    void **listeners_end   = *(void ***)((u8 *)level + 0x78);
    if (!listeners_begin || listeners_begin == listeners_end) return;
    u32 num_listeners = (u32)(listeners_end - listeners_begin);

    float fcount = (float)count;
    const float scale = 0.20000000298023224f;

    float bx = (float)block_pos[0];
    float by = (float)block_pos[1];
    float bz = (float)block_pos[2];

    u8 bdata = block_id_data[1];
    u32 low_data = (u32)bid | ((u32)bdata << 8);
    float random_value = BlockPos_randomFloat(block_pos);
    u32 texture_seed = (u32)(int)(random_value * 65535.0f);
    u32 data = (texture_seed << 16) | low_data;

    for (int ix = 0; ix < count; ix++) {
        float fx = ((float)ix + 0.5f) / fcount;
        float px = bx + fx;
        float mx = in_motion[0] + (fx - 0.5f) * scale;

        for (int iy = 1; iy < count; iy++) {
            float fy = ((float)iy + 0.5f) / fcount;
            float py = by + fy;
            float my = in_motion[1] + (fy - 0.5f) * scale;

            for (int iz = 0; iz < count; iz++) {
                float fz = ((float)iz + 0.5f) / fcount;
                float pz = bz + fz;
                float mz = in_motion[2] + (fz - 0.5f) * scale;

                float pos[3] = { px, py, pz };
                float mot[3] = { mx, my, mz };

                for (u32 l = 0; l < num_listeners; l++) {
                    void *listener = listeners_begin[l];
                    if (listener) {
                        void **vtable = *(void ***)listener;
                        if (vtable) {
                            LevelListenerAddParticleFn add_fn =
                                (LevelListenerAddParticleFn)vtable[12];
                            if (add_fn && add_fn(listener, 0x12, pos, mot, data)) {
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
}

int particle_spawn_fast_install_hook(void)
{
    particle_spawn_hook.target = 0x005C7BA4u;
    particle_spawn_hook.replacement = (u32)level_add_terrain_particle_fast;
    particle_spawn_hook.expected[0] = 0xE92D4FFFu;
    particle_spawn_hook.expected[1] = 0xE1A06000u;
    return s->host.install_hook(&particle_spawn_hook);
}
