#ifndef NUMC3DS_MATH_UTIL_H
#define NUMC3DS_MATH_UTIL_H

#include "../../include/numc3ds_abi.h"

extern numc3ds_u32 rng_state;

int integer(const char** at, int* out);
int decimal(const char** at, float base, float* out);
int divide(int value, int divisor);
unsigned int __aeabi_uidiv(unsigned num, unsigned den);
unsigned long long __aeabi_uidivmod(unsigned num, unsigned den);
numc3ds_u32 rng_next(void);

#endif /* NUMC3DS_MATH_UTIL_H */
