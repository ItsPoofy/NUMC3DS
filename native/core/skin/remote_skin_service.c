#include "remote_skin_service.h"
#include "skin_picker_hooks.h"
#include "skin_wire.h"
#include "skin_types.h"
#include "../util/string_util.h"
#include "../world_transfer/world_transfer_fs.h"
#include "../../state.h"
#include "../../seams.h"

typedef void (*StrCtorFn)(void *out, const char *text, void *scratch);
typedef void (*StrDtorFn)(void *out);
typedef void (*ResourceLocationCtorFn)(void *out, const char *path, int loader_id);
typedef void* (*SkinCtorFn)(void *skin, const void *serializable_name, const void *display_name,
                            u32 category, const void *texture, u32 enabled, u32 flags,
                            float scale);
typedef void* (*SkinRepositoryGetSkinFn)(void *repo, const NativeGstdString *skin_name);
typedef void (*StringAssignFn)(void *dest, const void *src);
typedef void* (*SkinInfoGetModelFn)(void *skin_info);
typedef void (*ResolveSkinFn)(void *skin_info);

enum {
    REMOTE_SKIN_MAX_SLOTS = 16,
    REMOTE_SKIN_3DST_MAX = 0x20 + 64 * 64 * 4
};

typedef struct {
    char uuid_hex[33];
    char registered_name[48];
    u32 native_skin[0x38 / sizeof(u32)];
    u32 skin_name_str;
    u32 skin_name_scratch;
    u32 display_name_str;
    u32 display_name_scratch;
    u32 texture_location[0x14 / sizeof(u32)];
    int is_valid;
    u32 last_used;
} RemoteSkinSlot;

static RemoteSkinSlot s_remote_slots[REMOTE_SKIN_MAX_SLOTS];
static u32 s_remote_usage_counter = 0;
static NuMC3DS_Hook s_skin_repo_get_skin_hook;
static NuMC3DS_Hook s_skin_info_get_model_hook;

static int native_string_view(const NativeGstdString *value, const char **bytes, u32 *length)
{
    u32 handle;
    if (!value || !bytes || !length || !(handle = value->handle)) return 0;
    *bytes = (const char *)handle;
    *length = *(const u32 *)(handle - 4u);
    return 1;
}

static void format_uuid_hex(const u8 *uuid, char *out_hex)
{
    static const char hex_chars[] = "0123456789abcdef";
    int i;
    for (i = 0; i < 16; ++i) {
        out_hex[i * 2]     = hex_chars[(uuid[i] >> 4) & 0x0Fu];
        out_hex[i * 2 + 1] = hex_chars[uuid[i] & 0x0Fu];
    }
    out_hex[32] = '\0';
}

static int is_slim_model(const char *skin_id)
{
    unsigned i;
    if (!skin_id) return 0;
    for (i = 0; skin_id[i]; ++i) {
        if ((skin_id[i] == 'S' || skin_id[i] == 's') &&
            (skin_id[i + 1] == 'L' || skin_id[i + 1] == 'l') &&
            (skin_id[i + 2] == 'I' || skin_id[i + 2] == 'i') &&
            (skin_id[i + 3] == 'M' || skin_id[i + 3] == 'm')) return 1;
        if ((skin_id[i] == 'A' || skin_id[i] == 'a') &&
            (skin_id[i + 1] == 'L' || skin_id[i + 1] == 'l') &&
            (skin_id[i + 2] == 'E' || skin_id[i + 2] == 'e') &&
            (skin_id[i + 3] == 'X' || skin_id[i + 3] == 'x')) return 1;
    }
    return 0;
}

static void string_concat(char *dst, unsigned max_size, const char *src)
{
    unsigned dst_len = text_len(dst);
    unsigned i = 0;
    if (dst_len >= max_size) return;
    while (src && src[i] && (dst_len + i + 1 < max_size)) {
        dst[dst_len + i] = src[i];
        i++;
    }
    dst[dst_len + i] = '\0';
}

static int compare_mem(const char *a, const char *b, u32 len)
{
    u32 i;
    for (i = 0; i < len; ++i) {
        if ((u8)a[i] != (u8)b[i]) return (int)((u8)a[i] - (u8)b[i]);
    }
    return 0;
}

void *remote_skin_service_find(const char *name, u32 name_len)
{
    int i;
    if (!name || !name_len) return 0;
    for (i = 0; i < REMOTE_SKIN_MAX_SLOTS; ++i) {
        RemoteSkinSlot *slot = &s_remote_slots[i];
        if (slot->is_valid) {
            u32 slot_len = text_len(slot->registered_name);
            if (slot_len == name_len && compare_mem(slot->registered_name, name, name_len) == 0) {
                slot->last_used = ++s_remote_usage_counter;
                return slot->native_skin;
            }
        }
    }
    return 0;
}

int remote_skin_service_register(const u8 *uuid_16, const char *skin_id,
                                 const u8 *rgba_bytes, u32 rgba_len,
                                 char *out_registered_name, u32 out_capacity)
{
    char uuid_hex[33];
    char reg_name[48];
    char file_path[128];
    char png_path[128];
    u32 width = 64u;
    u32 height;
    int slim;
    int slot_idx = -1;
    int i;
    u32 oldest_used = 0xFFFFFFFFu;
    int oldest_idx = 0;
    u8 encoded_3dst[REMOTE_SKIN_3DST_MAX];
    u32 encoded_len = 0;
    WtFsFile file;
    RemoteSkinSlot *slot;

    if (!uuid_16 || !rgba_bytes || !out_registered_name || out_capacity < 48u) return 0;
    if (rgba_len == 8192u) {
        height = 32u;
    } else if (rgba_len == 16384u) {
        height = 64u;
    } else {
        return 0;
    }

    format_uuid_hex(uuid_16, uuid_hex);
    reg_name[0] = 'R';
    reg_name[1] = 'e';
    reg_name[2] = 'm';
    reg_name[3] = 'o';
    reg_name[4] = 't';
    reg_name[5] = 'e';
    reg_name[6] = '\0';
    string_concat(reg_name, sizeof(reg_name), uuid_hex);

    for (i = 0; i < REMOTE_SKIN_MAX_SLOTS; ++i) {
        if (s_remote_slots[i].is_valid &&
            compare_mem(s_remote_slots[i].uuid_hex, uuid_hex, 32u) == 0) {
            slot_idx = i;
            break;
        }
        if (!s_remote_slots[i].is_valid && slot_idx < 0) {
            slot_idx = i;
        }
        if (s_remote_slots[i].last_used < oldest_used) {
            oldest_used = s_remote_slots[i].last_used;
            oldest_idx = i;
        }
    }
    if (slot_idx < 0) {
        slot_idx = oldest_idx;
    }

    slot = &s_remote_slots[slot_idx];

    if (!skin_wire_encode_3dst(rgba_bytes, width, height, encoded_3dst,
                               sizeof(encoded_3dst), &encoded_len)) {
        return 0;
    }

    wt_fs_mkdirs(NUMC3DS_SKINS_DIR "/remote");

    file_path[0] = '\0';
    string_concat(file_path, sizeof(file_path), NUMC3DS_SKINS_DIR "/remote/");
    string_concat(file_path, sizeof(file_path), uuid_hex);
    string_concat(file_path, sizeof(file_path), ".3dst");

    wt_fs_delete_file(file_path);
    if (wt_fs_create_file(file_path, (long long)encoded_len)) {
        if (wt_fs_open_file(file_path, 1, &file)) {
            u32 written = 0;
            wt_fs_write(&file, 0, encoded_3dst, encoded_len, &written);
            wt_fs_close_file(&file);
        }
    }

    if (slot->is_valid) {
        if (slot->skin_name_str) ((StrDtorFn)SEAM_StrDtor)(&slot->skin_name_str);
        if (slot->display_name_str) ((StrDtorFn)SEAM_StrDtor)(&slot->display_name_str);
    }
    zero(slot, sizeof(*slot));

    copy_text(slot->uuid_hex, uuid_hex, sizeof(slot->uuid_hex) - 1);
    copy_text(slot->registered_name, reg_name, sizeof(slot->registered_name) - 1);

    slot->skin_name_scratch = 0;
    ((StrCtorFn)SEAM_StrCtor)(&slot->skin_name_str, slot->registered_name,
                              &slot->skin_name_scratch);
    slot->display_name_scratch = 0;
    ((StrCtorFn)SEAM_StrCtor)(&slot->display_name_str, slot->registered_name,
                              &slot->display_name_scratch);

    png_path[0] = '\0';
    string_concat(png_path, sizeof(png_path), NUMC3DS_SKINS_DIR "/remote/");
    string_concat(png_path, sizeof(png_path), uuid_hex);
    string_concat(png_path, sizeof(png_path), ".png");

    zero(slot->texture_location, sizeof(slot->texture_location));
    ((ResourceLocationCtorFn)SEAM_ResourceLocation_ctor)(slot->texture_location, png_path, 1);

    slim = is_slim_model(skin_id);
    {
        void *repo = skin_picker_get_repository();
        const u8 *template_skin = repo ? (const u8 *)((void *(*)(void *, int))0x006D0240u)(repo, slim ? 1 : 0) : 0;
        u32 category = template_skin ? *(const u32 *)(template_skin + 0x14) : 0;
        u32 flags = template_skin ? *(const u32 *)(template_skin + 0x18) : (slim ? 1u : 0u);
        float scale = template_skin ? *(const float *)(template_skin + 0x34) : 1.0f;

        ((SkinCtorFn)SEAM_Skin_ctor)(slot->native_skin, &slot->skin_name_str,
                                     &slot->display_name_str, category, slot->texture_location,
                                     1, flags, scale);
    }

    slot->is_valid = 1;
    slot->last_used = ++s_remote_usage_counter;

    copy_text(out_registered_name, slot->registered_name, out_capacity - 1);
    return 1;
}

void remote_skin_service_reset(void)
{
    int i;
    for (i = 0; i < REMOTE_SKIN_MAX_SLOTS; ++i) {
        RemoteSkinSlot *slot = &s_remote_slots[i];
        if (slot->is_valid) {
            if (slot->skin_name_str) ((StrDtorFn)SEAM_StrDtor)(&slot->skin_name_str);
            if (slot->display_name_str) ((StrDtorFn)SEAM_StrDtor)(&slot->display_name_str);
            slot->is_valid = 0;
        }
    }
    zero(s_remote_slots, sizeof(s_remote_slots));
    wt_fs_delete_tree(NUMC3DS_SKINS_DIR "/remote");
}

static void *on_skin_repository_get_skin(void *repo, const NativeGstdString *skin_name)
{
    const char *name_bytes = 0;
    u32 name_len = 0;
    void *skin = 0;
    if (skin_name && native_string_view(skin_name, &name_bytes, &name_len) &&
        name_bytes && name_len >= 6u) {
        if (name_bytes[0] == 'R' && name_bytes[1] == 'e' && name_bytes[2] == 'm' &&
            name_bytes[3] == 'o' && name_bytes[4] == 't' && name_bytes[5] == 'e') {
            skin = remote_skin_service_find(name_bytes, name_len);
            if (skin) return skin;
        }
    }
    skin = ((SkinRepositoryGetSkinFn)s_skin_repo_get_skin_hook.trampoline)(repo, skin_name);
    if (!skin && repo) {
        typedef void *(*GetSteveSkinFn)(void *repo);
        skin = ((GetSteveSkinFn)SEAM_SkinRepository_getSteveSkin)(repo);
    }
    return skin;
}

static void *on_skin_info_get_model(void *skin_info)
{
    SkinInfoGetModelFn original = (SkinInfoGetModelFn)s_skin_info_get_model_hook.trampoline;
    void *model = original(skin_info);
    if (!model && skin_info) {
        NativeGstdString steve_str;
        u32 scratch = 0;
        zero(&steve_str, sizeof(steve_str));
        ((StrCtorFn)SEAM_StrCtor)(&steve_str, "Standard_Steve", &scratch);
        ((StringAssignFn)SEAM_StrAssign)((void *)((u8 *)skin_info + 4), &steve_str);
        if (steve_str.handle) ((StrDtorFn)SEAM_StrDtor)(&steve_str);
        ((ResolveSkinFn)0x001EBE8Cu)(skin_info);
        model = *(void **)((u8 *)skin_info + 44);
    }
    return model;
}

int remote_skin_service_install_hook(void)
{
    remote_skin_service_reset();

    zero(&s_skin_repo_get_skin_hook, sizeof(s_skin_repo_get_skin_hook));
    s_skin_repo_get_skin_hook.target = SEAM_SkinRepository_getSkin;
    s_skin_repo_get_skin_hook.replacement = (u32)on_skin_repository_get_skin;
    s_skin_repo_get_skin_hook.expected[0] = 0xE92D4FF0u;
    s_skin_repo_get_skin_hook.expected[1] = 0xE24DD014u;
    if (s->host.install_hook(&s_skin_repo_get_skin_hook)) return -90;

    zero(&s_skin_info_get_model_hook, sizeof(s_skin_info_get_model_hook));
    s_skin_info_get_model_hook.target = 0x001EBE68u;
    s_skin_info_get_model_hook.replacement = (u32)on_skin_info_get_model;
    s_skin_info_get_model_hook.expected[0] = 0xE92D4010u;
    s_skin_info_get_model_hook.expected[1] = 0xE1A04000u;
    if (s->host.install_hook(&s_skin_info_get_model_hook)) return -91;

    return 0;
}
