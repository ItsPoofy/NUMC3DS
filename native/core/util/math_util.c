#include "math_util.h"
#include "string_util.h"
#include "../rt.h"

numc3ds_u32 rng_state = 0x12345678u;

int integer(const char** at, int* out) {
    const char* p = spaces(*at);
    int sign = 1, value = 0, any = 0;
    if (*p == '-') {
        sign = -1;
        p++;
    } else if (*p == '+') {
        p++;
    }
    while (*p >= '0' && *p <= '9') {
        any = 1;
        value = value * 10 + (*p++ - '0');
    }
    if (!any) return 0;
    *out = value * sign;
    *at = spaces(p);
    return 1;
}

int decimal(const char** at, float base, float* out) {
    const char* p = spaces(*at);
    int relative = 0, sign = 1, whole = 0, frac = 0, scale = 1, any = 0;
    if (*p == '~') {
        relative = 1;
        p++;
        if (*p == ' ' || !*p) {
            *out = base;
            *at = spaces(p);
            return 1;
        }
    }
    if (*p == '-') {
        sign = -1;
        p++;
    } else if (*p == '+') {
        p++;
    }
    while (*p >= '0' && *p <= '9') {
        any = 1;
        whole = whole * 10 + (*p++ - '0');
    }
    if (*p == '.') {
        p++;
        while (*p >= '0' && *p <= '9') {
            any = 1;
            frac = frac * 10 + (*p++ - '0');
            scale *= 10;
        }
    }
    if (!any) return 0;
    *out = (relative ? base : 0.0f) + (float)sign * ((float)whole + (float)frac / (float)scale);
    *at = spaces(p);
    return 1;
}

int divide(int value, int divisor) {
    return ((SignedDivide)0x003021F0u)(value, divisor);
}

unsigned int __aeabi_uidiv(unsigned num, unsigned den) {
    return (unsigned int)((unsigned long long(*)(unsigned, unsigned))0x002FFF64u)(num, den);
}

unsigned long long __aeabi_uidivmod(unsigned num, unsigned den) {
    return ((unsigned long long(*)(unsigned, unsigned))0x002FFF64u)(num, den);
}

numc3ds_u32 rng_next(void) {
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 17;
    rng_state ^= rng_state << 5;
    return rng_state;
}
