#include "matrix_stack_fast.h"
#include "../../state.h"

static NuMC3DS_Hook matrix_hooks[7];

float *matrix_scale_floats(float *m, float sx, float sy, float sz)
{
    m[0] *= sx;  m[1] *= sx;  m[2] *= sx;  m[3] *= sx;
    m[4] *= sy;  m[5] *= sy;  m[6] *= sy;  m[7] *= sy;
    m[8] *= sz;  m[9] *= sz;  m[10] *= sz; m[11] *= sz;
    return m;
}

float *matrix_scale_vec3(float *m, const float *v)
{
    float sx = v[0], sy = v[1], sz = v[2];
    m[0] *= sx;  m[1] *= sx;  m[2] *= sx;  m[3] *= sx;
    m[4] *= sy;  m[5] *= sy;  m[6] *= sy;  m[7] *= sy;
    m[8] *= sz;  m[9] *= sz;  m[10] *= sz; m[11] *= sz;
    return m;
}

float *matrix_translate_vec3(float *m, const float *v)
{
    float tx = v[0], ty = v[1], tz = v[2];
    m[12] += m[0] * tx + m[4] * ty + m[8]  * tz;
    m[13] += m[1] * tx + m[5] * ty + m[9]  * tz;
    m[14] += m[2] * tx + m[6] * ty + m[10] * tz;
    m[15] += m[3] * tx + m[7] * ty + m[11] * tz;
    return m;
}

void matrix_transpose_inplace(float *m)
{
    float t;
    #define MATRIX_SWAP(a, b) t = m[a]; m[a] = m[b]; m[b] = t;
    MATRIX_SWAP(1, 4);
    MATRIX_SWAP(2, 8);
    MATRIX_SWAP(3, 12);
    MATRIX_SWAP(6, 9);
    MATRIX_SWAP(7, 13);
    MATRIX_SWAP(11, 14);
    #undef MATRIX_SWAP
}

void matrix_record_copy(void *dest, const void *src)
{
    const u32 *s_ptr = (const u32 *)src;
    u32 *d_ptr = (u32 *)dest;
    d_ptr[0] = s_ptr[0];   d_ptr[1] = s_ptr[1];   d_ptr[2] = s_ptr[2];   d_ptr[3] = s_ptr[3];
    d_ptr[4] = s_ptr[4];   d_ptr[5] = s_ptr[5];   d_ptr[6] = s_ptr[6];   d_ptr[7] = s_ptr[7];
    d_ptr[8] = s_ptr[8];   d_ptr[9] = s_ptr[9];   d_ptr[10] = s_ptr[10]; d_ptr[11] = s_ptr[11];
    d_ptr[12] = s_ptr[12]; d_ptr[13] = s_ptr[13]; d_ptr[14] = s_ptr[14]; d_ptr[15] = s_ptr[15];
    ((u8 *)d_ptr)[64] = ((const u8 *)s_ptr)[64];
}

void *matrix_mul(float *a, const float *b)
{
    float temp[16];
    int c;
    for (c = 0; c < 4; c++) {
        float b0 = b[c * 4 + 0];
        float b1 = b[c * 4 + 1];
        float b2 = b[c * 4 + 2];
        float b3 = b[c * 4 + 3];
        int r;
        for (r = 0; r < 4; r++) {
            float v0 = a[r + 0] * b0;
            float v1 = a[r + 4] * b1;
            float v2 = a[r + 8] * b2;
            float v3 = a[r + 12] * b3;
            temp[c * 4 + r] = (v0 + v1) + v2 + v3;
        }
    }
    for (c = 0; c < 16; c++) {
        a[c] = temp[c];
    }
    return a;
}

int matrix_stack_fast_install_hooks(void)
{
    static const u32 targets[7] = {
        0x005FC100u,
        0x005FC56Cu,
        0x005FC750u,
        0x005FBD7Cu,
        0x005FC40Cu,
        0x005FC1F8u,
        0x005FC6A8u
    };
    static const u32 expected[7][2] = {
        { 0xE92D40F0u, 0xE24DD054u },
        { 0xE92D4010u, 0xE24DD040u },
        { 0xE92D4010u, 0xE1A04001u },
        { 0xE92D4070u, 0xE1A05000u },
        { 0xE92D40F0u, 0xE24DD0B4u },
        { 0xE92D40F0u, 0xE24DD094u },
        { 0xE92D4010u, 0xE1A04001u }
    };
    u32 replacements[7];
    u32 i;
    replacements[0] = (u32)matrix_scale_vec3;
    replacements[1] = (u32)matrix_transpose_inplace;
    replacements[2] = (u32)matrix_record_copy;
    replacements[3] = (u32)matrix_mul;
    replacements[4] = (u32)matrix_translate_vec3;
    replacements[5] = (u32)matrix_scale_floats;
    replacements[6] = (u32)matrix_record_copy;

    for (i = 0; i < 7; i++) {
        matrix_hooks[i].target = targets[i];
        matrix_hooks[i].replacement = replacements[i];
        matrix_hooks[i].expected[0] = expected[i][0];
        matrix_hooks[i].expected[1] = expected[i][1];
        if (s->host.install_hook(&matrix_hooks[i])) return -50 - (int)i;
    }
    return 0;
}
