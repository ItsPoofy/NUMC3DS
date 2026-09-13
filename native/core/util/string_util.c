#include "string_util.h"
#include "../rt.h"

void cp(void* d, const void* v, numc3ds_u32 n) {
    unsigned char* a = d;
    const unsigned char* b = v;
    while (n--) *a++ = *b++;
}

void zero(void* d, numc3ds_u32 n) {
    unsigned char* a = d;
    while (n--) *a++ = 0;
}

void copy_text(char* d, const char* text, unsigned max) {
    unsigned n = 0;
    if (text) {
        while (n < max && text[n]) {
            d[n] = text[n];
            n++;
        }
    }
    d[n] = 0;
}

unsigned text_len(const char* t) {
    unsigned n = 0;
    if (t) while (t[n]) n++;
    return n;
}

int streq(const char* a, const char* b) {
    while (*a && *a == *b) {
        a++;
        b++;
    }
    return *a == *b;
}

const char* spaces(const char* p) {
    while (*p == ' ') p++;
    return p;
}

const char* word(const char* p, char* out, unsigned cap) {
    unsigned n = 0;
    p = spaces(p);
    while (*p && *p != ' ') {
        if (n + 1 < cap) out[n++] = *p;
        p++;
    }
    out[n] = 0;
    return spaces(p);
}

void append(char* out, unsigned* len, const char* text) {
    while (*text && *len < MAX_TEXT) out[(*len)++] = *text++;
    out[*len] = 0;
}

void append_int(char* out, unsigned* len, int value) {
    char rev[12];
    unsigned n = 0;
    int q;
    if (value < 0) {
        append(out, len, "-");
        value = -value;
    }
    do {
        q = ((SignedDivide)0x003021F0u)(value, 10);
        rev[n++] = (char)('0' + value - q * 10);
        value = q;
    } while (value && n < sizeof(rev));
    while (n && *len < MAX_TEXT) out[(*len)++] = rev[--n];
    out[*len] = 0;
}

numc3ds_u32 make_str(numc3ds_u32* out, const char* t) {
    numc3ds_u32 sc = 0;
    ((StrCtor)0x002FF221u)(out, t, &sc);
    return *out;
}

void drop_str(numc3ds_u32* h) {
    ((StrDtor)0x002FEBBDu)(h);
}

void release_obj(void* o) {
    if (o) ((void(*)(void*))(*(void***)o)[1])(o);
}

const char* strip_ns(const char* n) {
    static const char ns[] = "minecraft:";
    unsigned i;
    for (i = 0; ns[i]; i++) if (n[i] != ns[i]) return n;
    return n + i;
}
