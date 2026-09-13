#include <stddef.h>

void *memcpy(void *destination, const void *source, size_t length)
{
    unsigned char *out = (unsigned char *)destination;
    const unsigned char *in = (const unsigned char *)source;
    size_t index;
    for (index = 0; index < length; ++index) out[index] = in[index];
    return destination;
}

void *memset(void *destination, int value, size_t length)
{
    unsigned char *out = (unsigned char *)destination;
    size_t index;
    for (index = 0; index < length; ++index) out[index] = (unsigned char)value;
    return destination;
}

void *memmove(void *destination, const void *source, size_t length)
{
    unsigned char *out = (unsigned char *)destination;
    const unsigned char *in = (const unsigned char *)source;
    size_t index;
    if (out == in || !length) return destination;
    if (out < in) {
        for (index = 0; index < length; ++index) out[index] = in[index];
    } else {
        for (index = length; index; --index) out[index - 1] = in[index - 1];
    }
    return destination;
}

int memcmp(const void *left, const void *right, size_t length)
{
    const unsigned char *a = (const unsigned char *)left;
    const unsigned char *b = (const unsigned char *)right;
    size_t index;
    for (index = 0; index < length; ++index) {
        if (a[index] != b[index]) return a[index] < b[index] ? -1 : 1;
    }
    return 0;
}

size_t strlen(const char *text)
{
    size_t length = 0;
    while (text && text[length]) ++length;
    return length;
}

void __aeabi_memcpy(void *destination, const void *source, size_t length)
{
    memcpy(destination, source, length);
}

void __aeabi_memcpy4(void *destination, const void *source, size_t length)
{
    memcpy(destination, source, length);
}
