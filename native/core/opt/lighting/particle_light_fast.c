#include "particle_light_fast.h"
#include "../../state.h"


typedef Color (*GetColorFn)(const u8 *light);
#define LightTexture_getColorForUV ((GetColorFn)0x001D9440u)

static NuMC3DS_Hook particle_light_hook;

static inline int fast_floor(float val)
{
    int i = (int)val;
    return val < (float)i ? i - 1 : i;
}

static inline void sample_light(const u8 *region, const u8 *chunk, int x, int y, int z, u8 *out_sky, u8 *out_block)
{
    short height_limit = *(const short *)(region + 0x18);
    if (y < 0 || y >= height_limit) {
        *out_sky = region[60];
        *out_block = region[61];
        return;
    }

    int cx = x >> 4;
    int cz = z >> 4;
    const u8 *target_chunk = chunk;
    if (!target_chunk || *(const int *)(target_chunk + 0x20) != cx || *(const int *)(target_chunk + 0x24) != cz) {
        const u8 *cached = *(const u8 * const *)(region + 0x34);
        if (cached && *(const int *)(cached + 0x20) == cx && *(const int *)(cached + 0x24) == cz) {
            target_chunk = cached;
        } else {
            int chunk_pos[2];
            chunk_pos[0] = cx;
            chunk_pos[1] = cz;
            target_chunk = ((const u8 *(*)(const u8 *, const int *))0x0017AB3Cu)(region, chunk_pos);
        }
    }

    if (!target_chunk) {
        *out_sky = region[60];
        *out_block = region[61];
        return;
    }

    u32 section = (u32)y >> 4;
    u32 section_count = *(const u32 *)(target_chunk + 0x7c);
    if (section >= section_count) {
        *out_sky = target_chunk[0xb44];
        *out_block = target_chunk[0xb45];
        return;
    }

    const u8 *subchunk = *(const u8 * const *)(target_chunk + 0x5c + section * 4u);
    if (!subchunk) {
        *out_sky = 0;
        *out_block = 0;
        return;
    }

    const u8 *light_array = *(const u8 * const *)(subchunk + 0x1800);
    if (!light_array) {
        *out_sky = 0;
        *out_block = 0;
        return;
    }

    u32 idx = (((u32)x & 15u) << 8) | (((u32)z & 15u) << 4) | ((u32)y & 15u);
    u8 val = light_array[idx];
    *out_sky = val >> 4;
    *out_block = val & 15u;
}

Color particle_get_light_color_fast(const void *particle, float partial_ticks)
{
    const float *pos = (const float *)((const u8 *)particle + 0x94);
    const float *old_pos = (const float *)((const u8 *)particle + 0xA0);
    float px = old_pos[0] + (pos[0] - old_pos[0]) * partial_ticks;
    float py = old_pos[1] + (pos[1] - old_pos[1]) * partial_ticks;
    float pz = old_pos[2] + (pos[2] - old_pos[2]) * partial_ticks;

    int bx = fast_floor(px);
    int by = fast_floor(py);
    int bz = fast_floor(pz);

    const u8 *region = *(const u8 * const *)((const u8 *)particle + 0x84);
    u8 min_light = *(const u8 *)0x00A34726u;
    short height_limit = *(const short *)(region + 0x18);

    u32 lx = (u32)bx & 15u;
    u32 lz = (u32)bz & 15u;
    u32 ly = (u32)by & 15u;

    u8 max_sky;
    u8 max_block;

    const u8 *chunk = *(const u8 * const *)(region + 0x34);
    if (lx != 0 && lx != 15 && lz != 0 && lz != 15 && ly != 0 && ly != 15 &&
        by > 0 && (by + 1) < height_limit &&
        chunk && *(const int *)(chunk + 0x20) == (bx >> 4) && *(const int *)(chunk + 0x24) == (bz >> 4)) {

        u32 section = (u32)by >> 4;
        u32 section_count = *(const u32 *)(chunk + 0x7c);
        if (section >= section_count) {
            max_sky = chunk[0xb44];
            max_block = chunk[0xb45];
        } else {
            const u8 *subchunk = *(const u8 * const *)(chunk + 0x5c + section * 4u);
            if (subchunk) {
                const u8 *light_array = *(const u8 * const *)(subchunk + 0x1800);
                if (light_array) {
                    u32 base = (lx << 8) | (lz << 4) | ly;
                    u8 v0 = light_array[base];
                    u8 v1 = light_array[base + 1];
                    u8 v2 = light_array[base - 1];
                    u8 v3 = light_array[base - 16];
                    u8 v4 = light_array[base + 16];
                    u8 v5 = light_array[base + 256];
                    u8 v6 = light_array[base - 256];

                    max_sky = v0 >> 4;
                    max_block = v0 & 15u;

                    #define CHECK_NIBBLES(v) do { \
                        u8 s = (v) >> 4; \
                        u8 b = (v) & 15u; \
                        if (s > max_sky) max_sky = s; \
                        if (b > max_block) max_block = b; \
                    } while (0)

                    CHECK_NIBBLES(v1);
                    CHECK_NIBBLES(v2);
                    CHECK_NIBBLES(v3);
                    CHECK_NIBBLES(v4);
                    CHECK_NIBBLES(v5);
                    CHECK_NIBBLES(v6);
                    #undef CHECK_NIBBLES
                } else {
                    max_sky = 0;
                    max_block = 0;
                }
            } else {
                max_sky = 0;
                max_block = 0;
            }
        }
    } else {
        u8 s, b;
        sample_light(region, chunk, bx, by, bz, &max_sky, &max_block);

        sample_light(region, chunk, bx, by + 1, bz, &s, &b);
        if (s > max_sky) max_sky = s;
        if (b > max_block) max_block = b;

        sample_light(region, chunk, bx, by - 1, bz, &s, &b);
        if (s > max_sky) max_sky = s;
        if (b > max_block) max_block = b;

        sample_light(region, chunk, bx, by, bz - 1, &s, &b);
        if (s > max_sky) max_sky = s;
        if (b > max_block) max_block = b;

        sample_light(region, chunk, bx, by, bz + 1, &s, &b);
        if (s > max_sky) max_sky = s;
        if (b > max_block) max_block = b;

        sample_light(region, chunk, bx + 1, by, bz, &s, &b);
        if (s > max_sky) max_sky = s;
        if (b > max_block) max_block = b;

        sample_light(region, chunk, bx - 1, by, bz, &s, &b);
        if (s > max_sky) max_sky = s;
        if (b > max_block) max_block = b;
    }

    if (max_block < min_light) max_block = min_light;

    u8 light_bytes[2];
    light_bytes[0] = max_sky;
    light_bytes[1] = max_block;

    return LightTexture_getColorForUV(light_bytes);
}

int particle_light_fast_install_hooks(void)
{
    particle_light_hook.target = 0x0065CD68u;
    particle_light_hook.replacement = (u32)particle_get_light_color_fast;
    particle_light_hook.expected[0] = 0xE92D40F0u;
    particle_light_hook.expected[1] = 0xE1A04000u;
    if (s->host.install_hook(&particle_light_hook)) return -71;
    return 0;
}
