#include "skin_wire.h"
#include "skin_service.h"
#include "skin_picker_hooks.h"
#include "../util/string_util.h"
#include "../../state.h"
#include "../../seams.h"
#include "../../../diagnostics/network_debug.h"
#include "../world_transfer/world_transfer_fs.h"

typedef void *(*AppPlatformSingletonFn)(void);
typedef void *(*AppPlatformMapPathFn)(void *out, void *platform, const void *path);
typedef void (*StrCtorFn)(void *out, const char *text, void *scratch);
typedef void (*StrDtorFn)(void *out);

enum {
    SKIN_WIRE_HEADER_BYTES = 0x20u,
    SKIN_WIRE_MAX_SOURCE_BYTES = 0x100000u,
    SKIN_WIRE_MAX_DIMENSION = 64u
};

static u32 read_le32(const u8 *bytes)
{
    return (u32)bytes[0] | ((u32)bytes[1] << 8) |
           ((u32)bytes[2] << 16) | ((u32)bytes[3] << 24);
}

static int read_file(const char *path, u8 **bytes_out, u32 *length_out)
{
    u32 file[6];
    u8 *bytes;
    u32 size;
    if (!path || !bytes_out || !length_out || !s || !s->host.heap_alloc) return 0;
    zero(file, sizeof(file));
    *bytes_out = 0;
    *length_out = 0;
    ((void (*)(void *))SEAM_ResourceFile_constructor)(file);
    if (!((int (*)(void *, const char *))SEAM_ResourceFile_open)(file, path)) {
        ((void (*)(void *))SEAM_ResourceFile_destructor)(file);
        return 0;
    }
    size = ((u32 (*)(void *))SEAM_ResourceFile_getSize)(file);
    if (!size || size > SKIN_WIRE_MAX_SOURCE_BYTES) {
        ((void (*)(void *))SEAM_ResourceFile_destructor)(file);
        return 0;
    }
    bytes = (u8 *)s->host.heap_alloc(size);
    if (!bytes) {
        ((void (*)(void *))SEAM_ResourceFile_destructor)(file);
        return 0;
    }
    if (!((int (*)(void *, void *, u32))SEAM_ResourceFile_read)(file, bytes, size)) {
        ((void (*)(void *))SEAM_ResourceFile_destructor)(file);
        s->host.heap_free(bytes);
        return 0;
    }
    ((void (*)(void *))SEAM_ResourceFile_destructor)(file);
    *bytes_out = bytes;
    *length_out = size;
    return 1;
}

static u32 pica_morton(u32 x, u32 y)
{
    return (x & 1u) |
           ((y & 1u) << 1) |
           ((x & 2u) << 1) |
           ((y & 2u) << 2) |
           ((x & 4u) << 2) |
           ((y & 4u) << 3);
}

static int decode_serializable_skin(const u8 *source, u32 source_length,
                                    u8 **pixels_out, u32 *pixels_length,
                                    const char **skin_id, u8 *slim)
{
    u32 width;
    u32 height;
    u32 pixels;
    u32 raw_length;
    u8 *raw;
    u32 x;
    u32 y;

    if (!source || source_length < SKIN_WIRE_HEADER_BYTES || !pixels_out ||
        !pixels_length || !skin_id || !slim || !s || !s->host.heap_alloc) return 0;
    if (read_le32(source) != 0x54534433u || read_le32(source + 4u) != 3u) return 0;
    width = read_le32(source + 0x0cu);
    height = read_le32(source + 0x10u);
    if (!width || !height || width > SKIN_WIRE_MAX_DIMENSION ||
        height > SKIN_WIRE_MAX_DIMENSION || (width != 64u) ||
        (height != 32u && height != 64u)) return 0;
    pixels = width * height;
    if (source_length != SKIN_WIRE_HEADER_BYTES + pixels * 4u) return 0;
    raw_length = pixels * 4u;
    raw = (u8 *)s->host.heap_alloc(raw_length);
    if (!raw) return 0;
    for (y = 0; y < height; ++y) {
        u32 gpu_y = height - 1u - y;
        for (x = 0; x < width; ++x) {
            u32 morton = pica_morton(x, gpu_y);
            u32 source_pixel = (gpu_y & ~7u) * width + (x & ~7u) * 8u + morton;
            const u8 *pixel = source + SKIN_WIRE_HEADER_BYTES + source_pixel * 4u;
            u8 *destination = raw + (y * width + x) * 4u;
            destination[0] = pixel[3];
            destination[1] = pixel[2];
            destination[2] = pixel[1];
            destination[3] = pixel[0];
        }
    }
    *pixels_out = raw;
    *pixels_length = raw_length;
    return 1;
}

static int build_from_path(const char *path, const char *skin_id, u8 slim,
                           McpeSkinWireAsset *asset)
{
    u8 *source;
    u32 source_length;
    if (!read_file(path, &source, &source_length)) return 0;
    if (!decode_serializable_skin(source, source_length, &asset->bytes, &asset->length,
                                  &asset->skin_id, &asset->slim)) {
        s->host.heap_free(source);
        return 0;
    }
    s->host.heap_free(source);
    asset->skin_id = skin_id;
    asset->slim = slim;
    return 1;
}

static int build_3dst_path(char *out, unsigned capacity, const char *path)
{
    unsigned length;
    if (!out || !capacity || !path) return 0;
    if (!wt_fs_copy_text(out, capacity, path)) return 0;
    length = text_len(out);
    if (length < 4u || length + 2u > capacity) return 0;
    if (out[length - 4u] != '.' ||
        !((out[length - 3u] == 'p' && out[length - 2u] == 'n' && out[length - 1u] == 'g') ||
          (out[length - 3u] == 't' && out[length - 2u] == 'g' && out[length - 1u] == 'a'))) return 0;
    out[length - 3u] = '3';
    out[length - 2u] = 'd';
    out[length - 1u] = 's';
    out[length] = 't';
    out[length + 1u] = '\0';
    return 1;
}

static int build_from_native_skin(void *skin, const char *skin_id, u8 slim,
                                  McpeSkinWireAsset *asset)
{
    void *platform;
    void **vtable;
    const char *logical_path;
    const char *mapped_path;
    u32 logical_path_string = 0;
    u32 mapped_path_string = 0;
    u32 scratch = 0;
    char texture_path[WT_FS_MAX_PATH];
    int result = 0;
    texture_path[0] = 0;
    net_log_open(NET_LOG_INFO, "skin_wire", "native_begin");
    net_log_hex("skin", (u32)skin);
    net_log_text("id", skin_id);
    net_log_close();
    if (!skin || !asset) return 0;
    logical_path = *(const char **)((u8 *)skin + 0x1cu);
    platform = ((AppPlatformSingletonFn)SEAM_AppPlatform_singleton)();
    vtable = platform ? *(void ***)platform : 0;
    net_log_open(NET_LOG_INFO, "skin_wire", "native_source");
    net_log_text("path", logical_path);
    net_log_hex("platform", (u32)platform);
    net_log_close();
    if (!logical_path || !logical_path[0] || !vtable || !vtable[0x14c / 4]) return 0;
    ((StrCtorFn)SEAM_StrCtor)(&logical_path_string, logical_path, &scratch);
    ((AppPlatformMapPathFn)vtable[0x14c / 4])(&mapped_path_string, platform,
                                              &logical_path_string);
    mapped_path = *(const char **)&mapped_path_string;
    net_log_open(NET_LOG_INFO, "skin_wire", "mapped");
    net_log_text("path", mapped_path);
    net_log_close();
    if (mapped_path && build_3dst_path(texture_path, sizeof(texture_path), mapped_path)) {
        result = build_from_path(texture_path, skin_id, slim, asset);
    }
    net_log_open(NET_LOG_INFO, "skin_wire", "native_result");
    net_log_text("path", texture_path);
    net_log_signed("result", result);
    net_log_close();
    if (mapped_path_string) ((StrDtorFn)SEAM_StrDtor)(&mapped_path_string);
    if (logical_path_string) ((StrDtorFn)SEAM_StrDtor)(&logical_path_string);
    return result;
}

static int skin_name_matches(const char *a, const char *b)
{
    if (!a || !b) return 0;
    while (*a && *b) {
        char ca = (*a == '_') ? '-' : *a;
        char cb = (*b == '_') ? '-' : *b;
        if (ca != cb) return 0;
        a++;
        b++;
    }
    return *a == *b;
}

int skin_wire_build(McpeSkinWireAsset *asset)
{
    CustomSkinEntry *entry;
    int index;
    void *repo;
    void *current_skin;
    void *steve_skin;
    const char *sname = 0;
    if (!asset) return 0;
    zero(asset, sizeof(*asset));

    repo = skin_picker_get_repository();
    current_skin = repo ? ((void *(*)(void *))SEAM_SkinRepository_getCurrentSkin)(repo) : 0;
    net_log_open(NET_LOG_INFO, "skin_wire", "repository");
    net_log_hex("repo", (u32)repo);
    net_log_hex("current", (u32)current_skin);
    net_log_close();
    if (current_skin) {
        sname = *(const char **)((u8 *)current_skin + 0x0C);
    }

    if (current_skin) {
        int is_alex = sname && (streq(sname, "Alex") || streq(sname, "Standard_Alex"));
        int is_steve = sname && (streq(sname, "Steve") || streq(sname, "Standard_Steve"));

        if (is_alex) {
            return build_from_native_skin(current_skin, "Standard_Alex", 1, asset);
        } else if (is_steve) {
            return build_from_native_skin(current_skin, "Standard_Steve", 0, asset);
        } else {
            for (index = 0; index < skin_service_get_count(); ++index) {
                entry = skin_service_get_entry(index);
                if (entry && entry->is_valid && entry->model_type != SKIN_MODEL_CUSTOM_3D) {
                    if (current_skin == entry->skin_instance ||
                        (sname && skin_name_matches(sname, entry->name))) {
                        if (build_from_path(entry->texture_path,
                                            entry->model_type == SKIN_MODEL_SLIM ?
                                                "Standard_CustomSlim" : "Standard_Custom",
                                            entry->model_type == SKIN_MODEL_SLIM, asset)) return 1;
                    }
                }
            }
        }
    }

    steve_skin = repo ? ((void *(*)(void *, int))SEAM_SkinRepository_getSkinId)(repo, 3) : 0;
    return build_from_native_skin(steve_skin, "Standard_Steve", 0, asset);
}



void skin_wire_release(McpeSkinWireAsset *asset)
{
    if (!asset) return;
    if (asset->bytes && s && s->host.heap_free) s->host.heap_free(asset->bytes);
    zero(asset, sizeof(*asset));
}

int skin_wire_encode_3dst(const numc3ds_u8 *src_rgba, numc3ds_u32 width, numc3ds_u32 height,
                          numc3ds_u8 *dst_3dst, numc3ds_u32 dst_capacity, numc3ds_u32 *out_length)
{
    u32 pixels;
    u32 total_size;
    u32 *header;
    u32 x, y;

    if (!src_rgba || !dst_3dst || !out_length) return 0;
    if (width != 64u || (height != 32u && height != 64u)) return 0;

    pixels = width * height;
    total_size = SKIN_WIRE_HEADER_BYTES + pixels * 4u;
    if (dst_capacity < total_size) return 0;

    header = (u32 *)dst_3dst;
    header[0] = 0x54534433u;
    header[1] = 3u;
    header[2] = 0u;
    header[3] = width;
    header[4] = height;
    header[5] = width;
    header[6] = height;
    header[7] = 1u;

    for (y = 0; y < height; ++y) {
        u32 gpu_y = height - 1u - y;
        for (x = 0; x < width; ++x) {
            u32 morton = pica_morton(x, gpu_y);
            u32 tiled_pixel = (gpu_y & ~7u) * width + (x & ~7u) * 8u + morton;
            const u8 *pixel = src_rgba + (y * width + x) * 4u;
            u8 *destination = dst_3dst + SKIN_WIRE_HEADER_BYTES + tiled_pixel * 4u;
            destination[0] = pixel[3];
            destination[1] = pixel[2];
            destination[2] = pixel[1];
            destination[3] = pixel[0];
        }
    }

    *out_length = total_size;
    return 1;
}
