#ifndef NUMC3DS_MCPE_TYPES_H
#define NUMC3DS_MCPE_TYPES_H

typedef unsigned char mcpe_u8;
typedef signed char mcpe_s8;
typedef unsigned short mcpe_u16;
typedef signed short mcpe_s16;
typedef unsigned int mcpe_u32;
typedef signed int mcpe_s32;
typedef unsigned long long mcpe_u64;
typedef signed long long mcpe_s64;

typedef struct {
    mcpe_u32 lo;
    mcpe_u32 hi;
} McpeWireU64;

#endif
