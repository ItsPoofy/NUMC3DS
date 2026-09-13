#include "particle_move_fast.h"
#include "../../state.h"

typedef void (*ParticleMoveFn)(void *particle, float *motion);

static NuMC3DS_Hook particle_move_hook;

static inline int fast_floor(float val)
{
    int i = (int)val;
    return val < (float)i ? i - 1 : i;
}

static inline u32 block_index(int x, int y, int z)
{
    return (((u32)x & 15u) << 8) | (((u32)z & 15u) << 4) | ((u32)y & 15u);
}

void particle_move_fast(void *particle, float *motion)
{
    if (!particle || !motion) return;

    /* If physics is disabled (noclip), update position directly */
    if (*(const char *)((const u8 *)particle + 0xac) != 0) {
        float *box = (float *)((u8 *)particle + 0xb0);
        float dx = motion[0], dy = motion[1], dz = motion[2];
        box[0] += dx; box[3] += dx;
        box[1] += dy; box[4] += dy;
        box[2] += dz; box[5] += dz;
        float *pos = (float *)((u8 *)particle + 0x94);
        pos[0] = (box[3] - box[0]) * 0.5f + box[0];
        pos[1] = (box[4] - box[1]) * 0.5f + box[1];
        pos[2] = (box[5] - box[2]) * 0.5f + box[2];
        return;
    }

    const float *box = (const float *)((const u8 *)particle + 0xb0);
    float dx = motion[0];
    float dy = motion[1];
    float dz = motion[2];

    /* Expanded bounding box calculation */
    float min_x = box[0] + (dx < 0.0f ? dx : 0.0f);
    float max_x = box[3] + (dx > 0.0f ? dx : 0.0f);
    float min_y = box[1] + (dy < 0.0f ? dy : 0.0f);
    float max_y = box[4] + (dy > 0.0f ? dy : 0.0f);
    float min_z = box[2] + (dz < 0.0f ? dz : 0.0f);
    float max_z = box[5] + (dz > 0.0f ? dz : 0.0f);

    int bx0 = fast_floor(min_x);
    int bx1 = fast_floor(max_x);
    int by0 = fast_floor(min_y);
    int by1 = fast_floor(max_y);
    int bz0 = fast_floor(min_z);
    int bz1 = fast_floor(max_z);

    const u8 *region = *(const u8 * const *)((const u8 *)particle + 0x84);
    if (!region) goto fallback;

    short height_limit = *(const short *)(region + 0x18);
    if (by0 < 0 || by1 >= height_limit) goto fallback;

    /* Particles are small (width ~0.2); span is at most 1 block boundary */
    if ((bx1 - bx0) > 1 || (by1 - by0) > 1 || (bz1 - bz0) > 1) goto fallback;

    /* Fast path: Check if all affected blocks are within the same SubChunk */
    int cx0 = bx0 >> 4, cx1 = bx1 >> 4;
    int cz0 = bz0 >> 4, cz1 = bz1 >> 4;
    int sy0 = by0 >> 4, sy1 = by1 >> 4;

    if (cx0 == cx1 && cz0 == cz1 && sy0 == sy1) {
        const u8 *chunk = *(const u8 * const *)(region + 0x34);
        if (chunk && *(const int *)(chunk + 0x20) == cx0 && *(const int *)(chunk + 0x24) == cz0) {
            u32 section_count = *(const u32 *)(chunk + 0x7c);
            if ((u32)sy0 < section_count) {
                const u8 *subchunk = *(const u8 * const *)(chunk + 0x5c + (u32)sy0 * 4u);
                if (!subchunk) {
                    /* SubChunk is unallocated -> 100% pure air */
                    goto apply_air_move;
                }

                if (bx0 == bx1 && by0 == by1 && bz0 == bz1) {
                    if (subchunk[block_index(bx0, by0, bz0)] == 0)
                        goto apply_air_move;
                    goto fallback;
                }

                /* Check all blocks in the cube range [bx0..bx1, by0..by1, bz0..bz1] */
                for (int x = bx0; x <= bx1; x++) {
                    for (int z = bz0; z <= bz1; z++) {
                        for (int y = by0; y <= by1; y++) {
                            if (subchunk[block_index(x, y, z)] != 0) {
                                goto fallback; /* Non-air block encountered */
                            }
                        }
                    }
                }
                goto apply_air_move;
            }
        }
    }

fallback:
    ((ParticleMoveFn)particle_move_hook.trampoline)(particle, motion);
    return;

apply_air_move:
    {
        float *mut_box = (float *)((u8 *)particle + 0xb0);
        mut_box[0] += dx; mut_box[3] += dx;
        mut_box[1] += dy; mut_box[4] += dy;
        mut_box[2] += dz; mut_box[5] += dz;
        float *pos = (float *)((u8 *)particle + 0x94);
        pos[0] = (mut_box[3] - mut_box[0]) * 0.5f + mut_box[0];
        pos[1] = (mut_box[4] - mut_box[1]) * 0.5f + mut_box[1];
        pos[2] = (mut_box[5] - mut_box[2]) * 0.5f + mut_box[2];
        *(u8 *)((u8 *)particle + 0xad) = 0; /* on_ground = 0 */
    }
}

int particle_move_fast_install_hook(void)
{
    particle_move_hook.target = 0x0065D278u;
    particle_move_hook.replacement = (u32)particle_move_fast;
    particle_move_hook.expected[0] = 0xE92D41F0u;
    particle_move_hook.expected[1] = 0xE1A04000u;
    return s->host.install_hook(&particle_move_hook);
}
