#ifndef NUMC3DS_STRING_UTIL_H
#define NUMC3DS_STRING_UTIL_H

#include "../../include/numc3ds_abi.h"

void cp(void* d, const void* v, numc3ds_u32 n);
void zero(void* d, numc3ds_u32 n);
void copy_text(char* d, const char* text, unsigned max);
unsigned text_len(const char* t);
int streq(const char* a, const char* b);
const char* spaces(const char* p);
const char* word(const char* p, char* out, unsigned cap);
void append(char* out, unsigned* len, const char* text);
void append_int(char* out, unsigned* len, int value);
numc3ds_u32 make_str(numc3ds_u32* out, const char* t);
void drop_str(numc3ds_u32* h);
void release_obj(void* o);
const char* strip_ns(const char* n);

#endif /* NUMC3DS_STRING_UTIL_H */
