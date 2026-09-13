#ifndef NUMC3DS_BLOCKPOS_FAST_H
#define NUMC3DS_BLOCKPOS_FAST_H

void *blockpos_from_vec3(int *result, const float *vec);
void *blockpos_from_floats(int *result, float x, float y, float z);
void *chunkpos_from_blockpos(int *result, const int *block_pos);

int blockpos_fast_install_hooks(void);

#endif
