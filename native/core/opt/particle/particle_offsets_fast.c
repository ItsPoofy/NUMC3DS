#include "particle_offsets_fast.h"
#include "../../state.h"

static NuMC3DS_Hook particle_offsets_hook;

static inline float fast_sqrt(float val)
{
    float res;
    __asm__("vsqrt.f32 %0, %1" : "=t"(res) : "t"(val));
    return res;
}

static inline float poly_acos(float x)
{
    /* Stock approximation: 1.57079637 - (x * 0.87266463 + x^3 * 0.69813168) */
    return (1.57079637f - (x * 0.87266463f + (x * x * x) * 0.69813168f));
}

void particle_generate_vertex_offsets_fast(
    const void *particle,
    float *p_r1c,
    float *p_r1d,
    float *p_r1e,
    float *p_r1f,
    float *p_r20)
{
    const float *cam = *(const float * const *)0x0065D23Cu;
    if (!cam || !particle) return;

    const float *pos = (const float *)((const u8 *)particle + 0x94);
    float dx = cam[0] - pos[0];
    float dy = cam[1] - pos[1];
    float dz = cam[2] - pos[2];

    float f4 = dx;
    float f2 = dz;
    float abs_dx = (f4 < 0.0f) ? -f4 : f4;
    if (abs_dx <= 0.0f) f4 = 0.00009999999747378752f;

    float abs_dz = (f2 < 0.0f) ? -f2 : f2;
    if (abs_dz <= 0.0f) f2 = 0.00009999999747378752f;

    float xz_sq = f4 * f4 + f2 * f2;
    if (xz_sq <= 0.0f) return;

    float inv_xz = 1.0f / fast_sqrt(xz_sq);
    f2 *= inv_xz;
    f4 *= inv_xz;

    float in_s10 = (f4 > 0.0f) ? 1.0f : -1.0f;
    float f6 = -f2;

    float total_sq = dx * dx + dy * dy + dz * dz;
    if (total_sq <= 0.0f) return;

    float inv_3d = 1.0f / fast_sqrt(total_sq);
    float sign_y = (dy * inv_3d <= 0.0f) ? -1.0f : 1.0f;

    float f5 = (f4 * dx + f2 * dz) * inv_3d;

    float p0 = poly_acos(f5) * sign_y;
    float p1 = poly_acos(f6) * in_s10;

    const float rad_to_idx = **(const float * const *)0x0065D240u;
    const float *sin_table = *(const float * const *)0x0065D248u;
    const float quarter = 16384.0f;

    int idx_cos_p = ((int)(quarter + p1 * rad_to_idx)) & 0xFFFF;
    int idx_sin_p = ((int)(p1 * rad_to_idx)) & 0xFFFF;
    int idx_sin_y = ((int)(p0 * rad_to_idx)) & 0xFFFF;
    int idx_cos_y = ((int)(quarter + p0 * rad_to_idx)) & 0xFFFF;

    float cos_p = sin_table[idx_cos_p];
    float sin_p = sin_table[idx_sin_p];
    float sin_y = sin_table[idx_sin_y];
    float cos_y = sin_table[idx_cos_y];

    *p_r1c = cos_p;
    *p_r1d = sin_p;
    *p_r1e = -(sin_p * sin_y);
    *p_r1f = sin_y * cos_p;
    *p_r20 = cos_y;
}

int particle_offsets_fast_install_hook(void)
{
    particle_offsets_hook.target = 0x0065D124u;
    particle_offsets_hook.replacement = (u32)particle_generate_vertex_offsets_fast;
    particle_offsets_hook.expected[0] = 0xE92D41F0u;
    particle_offsets_hook.expected[1] = 0xE24DD018u;
    return s->host.install_hook(&particle_offsets_hook);
}
