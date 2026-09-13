#ifndef NUMC3DS_MATH_FAST_H
#define NUMC3DS_MATH_FAST_H

#include "../../../include/numc3ds_abi.h"

void math_fast_vec4_scale(float *out, const float *in, const float *scalar);
void math_fast_vec4_add(float *out, const float *a, const float *b);
float math_fast_floorf(float x);
float math_fast_ceilf(float x);
void math_fast_matrix_copy_4x4(float *dst, const float *src);
int math_fast_spheres_intersect(float x1, float y1, float z1, float r1, float x2, float y2, float z2, float r2);

int math_fast_install_hooks(void);

#endif /* NUMC3DS_MATH_FAST_H */
