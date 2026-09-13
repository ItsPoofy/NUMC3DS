#ifndef NUMC3DS_WORLD_BLOCK_SERVICE_H
#define NUMC3DS_WORLD_BLOCK_SERVICE_H

#include "../../include/numc3ds_abi.h"

void* world_source(void* level);
unsigned char read_block_id(void* src, const int* pos);
unsigned char read_block_data(void* src, const int* pos);
int write_block(void* src, const int* pos, unsigned char id, unsigned char data);
void destroy_block(void* level, void* src, const int* pos, int drops);
int region_bounds(const int a[3], const int b[3], int lo[3], int hi[3]);
int boxes_overlap(const int lo1[3], const int hi1[3], const int lo2[3], const int hi2[3]);
int topsolid(void* src, const int* pos);
int resolve_block_arg(char* name, unsigned char* idOut);

#endif /* NUMC3DS_WORLD_BLOCK_SERVICE_H */
