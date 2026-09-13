typedef unsigned int u32;
typedef unsigned char u8;

typedef void *(*StringCtorFn)(void *, const char *, void *);
typedef void (*VectorPushBackFn)(void *, void *);
typedef void (*StringDtorFn)(void *);
typedef int (*RandomNextIntFn)(int);
typedef void (*ResourceLocationCtorFn)(void *, const char *, u32);
typedef void (*ResourceReadDispatchFn)(void *, u32 *);
typedef void (*ReaderCtorFn)(void *);
typedef int (*ReaderAttachFn)(void *, u32);
typedef u32 (*ReaderSizeFn)(void *);
typedef int (*ReaderReadFn)(void *, void *, u32);
typedef void (*ReaderCleanupFn)(void *);

static void *heap_alloc(u32 size)
{
    return ((void *(*)(u32, void *))0x0011493Cu)(size, (void *)0x00994898u);
}

static void heap_free(void *ptr)
{
    if (ptr) {
        ((void (*)(void *, int))0x001146E8u)(ptr, -1);
    }
}

static void mem_zero(void *dst, u32 size)
{
    volatile u8 *d = (volatile u8 *)dst;
    while (size--) *d++ = 0;
}

static int match_key(const char *p, const char *k)
{
    int i;
    for (i = 0; k[i]; ++i) {
        if (p[i] != k[i]) return 0;
    }
    return 1;
}

static int try_read_path(const char *path, u32 loader_id, u8 **bytes_out, u32 *length_out)
{
    u32 location[5];
    u32 reader[6];
    u32 resource_handle = 0x00B3EB10u;
    u32 size;
    u8 *bytes;

    mem_zero(location, sizeof(location));
    mem_zero(reader, sizeof(reader));

    ((ResourceLocationCtorFn)0x0033C630u)(location, path, loader_id);
    ((ReaderCtorFn)0x0010CA24u)(reader);
    ((ResourceReadDispatchFn)0x0065FA5Cu)(location, &resource_handle);

    if (((ReaderAttachFn)0x0010C86Cu)(reader, resource_handle)) {
        size = ((ReaderSizeFn)0x0010C9D8u)(reader);
        if (size > 0 && size <= 131072) {
            bytes = (u8 *)heap_alloc(size + 1);
            if (bytes) {
                if (((ReaderReadFn)0x0010C958u)(reader, bytes, size)) {
                    bytes[size] = 0;
                    ((ReaderCleanupFn)0x0010C9A4u)(reader);
                    ((ReaderCleanupFn)0x0010CA5Cu)(reader);
                    *bytes_out = bytes;
                    *length_out = size;
                    return 1;
                }
                heap_free(bytes);
            }
        }
        ((ReaderCleanupFn)0x0010C9A4u)(reader);
        ((ReaderCleanupFn)0x0010CA5Cu)(reader);
    } else {
        ((ReaderCleanupFn)0x0010CA5Cu)(reader);
    }
    return 0;
}

static int load_splashes_file(u8 **bytes_out, u32 *length_out)
{
    static const char * const paths[] = {
        "resourcepacks/vanilla/splashes.json",
        "resourcepacks/vanilla/client/splashes.json",
        "splashes.json",
    };
    int i;

    if (!bytes_out || !length_out) return 0;
    *bytes_out = 0;
    *length_out = 0;

    for (i = 0; i < 3; ++i) {
        if (try_read_path(paths[i], 6, bytes_out, length_out)) {
            return 1;
        }
    }
    return 0;
}

__attribute__((section(".entry"), used))
void numc3ds_build_splashes(void *screen)
{
    void *vec;
    u8 *json_bytes = 0;
    u32 json_len = 0;
    const char *p;
    const char *end;
    char buffer[256];
    u32 splash_count = 0;

    if (!screen) return;
    vec = (u8 *)screen + 0x120;

    if (load_splashes_file(&json_bytes, &json_len) && json_bytes && json_len > 0) {
        p = (const char *)json_bytes;
        end = p + json_len;

        while (p + 10 < end) {
            if (match_key(p, "\"splashes\"")) {
                p += 10;
                break;
            }
            p++;
        }

        while (p < end && *p != '[') p++;
        if (p < end) p++;

        while (p < end) {
            while (p < end && (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n' || *p == ',')) p++;
            if (p >= end || *p == ']') break;
            if (*p == '"') {
                p++;
                u32 bi = 0;
                while (p < end && *p != '"') {
                    if (*p == '\\' && (p + 1) < end) {
                        p++;
                        if (*p == 'n') buffer[bi++] = '\n';
                        else if (*p == 'r') buffer[bi++] = '\r';
                        else if (*p == 't') buffer[bi++] = '\t';
                        else if (*p == 'u' && (p + 4) < end) {
                            u32 val = 0;
                            int hi;
                            for (hi = 1; hi <= 4; ++hi) {
                                char c = p[hi];
                                val <<= 4;
                                if (c >= '0' && c <= '9') val |= (u32)(c - '0');
                                else if (c >= 'a' && c <= 'f') val |= (u32)(c - 'a' + 10);
                                else if (c >= 'A' && c <= 'F') val |= (u32)(c - 'A' + 10);
                            }
                            p += 4;
                            if (val < 0x80) {
                                buffer[bi++] = (char)val;
                            } else if (val < 0x800) {
                                if (bi + 2 < sizeof(buffer)) {
                                    buffer[bi++] = (char)(0xC0 | (val >> 6));
                                    buffer[bi++] = (char)(0x80 | (val & 0x3F));
                                }
                            } else {
                                if (bi + 3 < sizeof(buffer)) {
                                    buffer[bi++] = (char)(0xE0 | (val >> 12));
                                    buffer[bi++] = (char)(0x80 | ((val >> 6) & 0x3F));
                                    buffer[bi++] = (char)(0x80 | (val & 0x3F));
                                }
                            }
                        } else {
                            buffer[bi++] = *p;
                        }
                    } else {
                        buffer[bi++] = *p;
                    }
                    if (bi + 1 >= sizeof(buffer)) break;
                    p++;
                }
                buffer[bi] = 0;
                if (p < end && *p == '"') p++;

                if (bi > 0) {
                    char dummy_alloc[4];
                    char str_obj[16];
                    mem_zero(str_obj, sizeof(str_obj));
                    ((StringCtorFn)0x002FF221u)(str_obj, buffer, dummy_alloc);
                    ((VectorPushBackFn)0x001214F4u)(vec, str_obj);
                    ((StringDtorFn)0x002FEBBDu)(str_obj);
                    splash_count++;
                }
            } else {
                p++;
            }
        }

        heap_free(json_bytes);
    }

    if (splash_count == 0) {
        char dummy_alloc[4];
        char str_obj[16];
        mem_zero(str_obj, sizeof(str_obj));
        ((StringCtorFn)0x002FF221u)(str_obj, "Minecraft: New Nintendo 3DS Edition", dummy_alloc);
        ((VectorPushBackFn)0x001214F4u)(vec, str_obj);
        ((StringDtorFn)0x002FEBBDu)(str_obj);
        splash_count = 1;
    }

    {
        u32 start = *(u32 *)((u8 *)screen + 0x120);
        u32 finish = *(u32 *)((u8 *)screen + 0x124);
        u32 count = (finish >= start) ? ((finish - start) >> 2) : splash_count;
        if (count > 0) {
            *(u32 *)((u8 *)screen + 0x12C) = (u32)((RandomNextIntFn)0x0057F3F8u)((int)count);
        }
    }
}
