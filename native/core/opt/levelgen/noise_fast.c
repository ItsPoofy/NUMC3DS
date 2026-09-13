#include "noise_fast.h"
#include "../../hook_manager.h"

typedef struct {
    float x;
    float y;
    float z;
} Vec3;

typedef struct {
    float originX;
    float originY;
    float originZ;
    int permutation[512];
} SimplexNoise;

typedef void (*SimplexNoiseAdd3DFn)(
    SimplexNoise *self,
    float *output,
    const Vec3 *origin,
    int xSize,
    int ySize,
    int zSize,
    const Vec3 *scale,
    float amplitude
);

#define STOCK_SIMPLEX_NOISE_ADD3D ((SimplexNoiseAdd3DFn)0x006C0584)
#define TARGET_PERLIN_CALL_SITE    0x006A8444u
#define EXPECTED_PERLIN_CALL_INSTR 0xEB00604Eu

typedef struct {
    int Y;
    float fy0;
    float fy1;
    float v;
} YSample;

static inline int fast_floor(float v) {
    int i = (int)v;
    return v < (float)i ? i - 1 : i;
}

static inline float fade(float t) {
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

static inline float lerp(float t, float a, float b) {
    return a + t * (b - a);
}

static inline float grad3(int hash, float x, float y, float z) {
    int h = hash & 15;
    float u = (h < 8) ? x : y;
    float v = (h < 4) ? y : ((h == 12 || h == 14) ? x : z);
    float res_u = (h & 1) ? -u : u;
    float res_v = (h & 2) ? -v : v;
    return res_u + res_v;
}

void simplex_noise_add3d_fast(
    SimplexNoise *self,
    float *output,
    const Vec3 *origin,
    int xSize,
    int ySize,
    int zSize,
    const Vec3 *scale,
    float amplitude
) {
    if (ySize == 1 || ySize > 64 || !output || !self || !origin || !scale ||
        xSize <= 0 || ySize <= 0 || zSize <= 0) {
        STOCK_SIMPLEX_NOISE_ADD3D(self, output, origin, xSize, ySize, zSize, scale, amplitude);
        return;
    }

    float inv_amplitude = 1.0f / amplitude;
    const int *p = self->permutation;
    YSample y_samples[64];

    for (int iy = 0; iy < ySize; iy++) {
        float y_pos = origin->y + (float)iy;
        float y_coord = self->originY + y_pos * scale->y;
        int y_floor = fast_floor(y_coord);
        y_samples[iy].Y = y_floor & 255;
        float fy = y_coord - (float)y_floor;
        y_samples[iy].fy0 = fy;
        y_samples[iy].fy1 = fy - 1.0f;
        y_samples[iy].v = fade(fy);
    }

    int out_idx = 0;

    for (int ix = 0; ix < xSize; ix++) {
        float x_pos = origin->x + (float)ix;
        float x_coord = self->originX + x_pos * scale->x;
        int x_floor = fast_floor(x_coord);
        int X = x_floor & 255;
        float fx0 = x_coord - (float)x_floor;
        float fx1 = fx0 - 1.0f;
        float u = fade(fx0);

        int pX0 = p[X];
        int pX1 = p[X + 1];

        for (int iz = 0; iz < zSize; iz++) {
            float z_pos = origin->z + (float)iz;
            float z_coord = self->originZ + z_pos * scale->z;
            int z_floor = fast_floor(z_coord);
            int Z = z_floor & 255;
            float fz0 = z_coord - (float)z_floor;
            float fz1 = fz0 - 1.0f;
            float w = fade(fz0);

            int last_Y = -1;
            float out00 = 0.0f;
            float out10 = 0.0f;
            float out01 = 0.0f;
            float out11 = 0.0f;

            for (int iy = 0; iy < ySize; iy++) {
                int Y = y_samples[iy].Y;
                float v = y_samples[iy].v;

                if (iy == 0 || Y != last_Y) {
                    float fy0 = y_samples[iy].fy0;
                    float fy1 = y_samples[iy].fy1;

                    int A  = pX0 + Y;
                    int AA = p[A]     + Z;
                    int AB = p[A + 1] + Z;

                    int B  = pX1 + Y;
                    int BA = p[B]     + Z;
                    int BB = p[B + 1] + Z;

                    float g000 = grad3(p[AA],     fx0, fy0, fz0);
                    float g100 = grad3(p[BA],     fx1, fy0, fz0);
                    float g010 = grad3(p[AB],     fx0, fy1, fz0);
                    float g110 = grad3(p[BB],     fx1, fy1, fz0);

                    float g001 = grad3(p[AA + 1], fx0, fy0, fz1);
                    float g101 = grad3(p[BA + 1], fx1, fy0, fz1);
                    float g011 = grad3(p[AB + 1], fx0, fy1, fz1);
                    float g111 = grad3(p[BB + 1], fx1, fy1, fz1);

                    out00 = lerp(u, g000, g100);
                    out10 = lerp(u, g010, g110);
                    out01 = lerp(u, g001, g101);
                    out11 = lerp(u, g011, g111);

                    last_Y = Y;
                }

                float y0 = lerp(v, out00, out10);
                float y1 = lerp(v, out01, out11);

                float val = lerp(w, y0, y1);

                output[out_idx++] += val * inv_amplitude;
            }
        }
    }
}

numc3ds_s32 noise_fast_install_hooks(void) {
    return hook_manager_patch_call(
        TARGET_PERLIN_CALL_SITE,
        EXPECTED_PERLIN_CALL_INSTR,
        (numc3ds_u32)simplex_noise_add3d_fast
    );
}
