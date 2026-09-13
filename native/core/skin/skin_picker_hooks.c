#include "skin_picker_hooks.h"
#include "skin_service.h"
#include "../util/string_util.h"
#include "../../state.h"
#include "../../seams.h"

typedef void (*SkinPickerRenderTipsFn)(void *screen);
typedef void (*RenderButtonTipFn)(void *render_ctx, int x, int y, int glyph_id, void *str_obj);
typedef void (*StrCtorFn)(void *out, const char *text, void *scratch);
typedef void (*StrDtorFn)(void *out);
typedef void* (*GetSkinPacksByTypeFn)(void *repo, int type);
typedef void (*ResourceLocationCtorFn)(void *out, const char *path, int loader_id);
typedef void* (*SkinCtorFn)(void *skin, const void *serializable_name, const void *display_name,
                            u32 category, const void *texture, u32 enabled, u32 flags,
                            float scale);
typedef void* (*HeapAllocFn)(u32 size, void *selector);
typedef void* (*SkinPackCtorFn)(void *pack, int enabled, const void *serializable_name,
                                const void *pack_header, int pack_type, void *skins);
typedef void (*PackVectorPushFn)(void *vector, void *pack_slot);
typedef void* (*VectorAllocateFn)(u32 bytes, void *unused);
typedef void (*VectorDeallocateFn)(void *memory, u32 count, void *unused);

static NuMC3DS_Hook skin_picker_render_tips_hook;
static NuMC3DS_Hook s_get_skin_packs_by_type_hook;
static u32 s_last_pad_buttons = 0;

static u32 s_custom_native_skins[NUMC3DS_MAX_CUSTOM_SKINS][0x38 / sizeof(u32)];
static u32 s_custom_skin_names[NUMC3DS_MAX_CUSTOM_SKINS];
static u32 s_custom_skin_labels[NUMC3DS_MAX_CUSTOM_SKINS];
static u32 s_fallback_skin_name = 0;
static u32 s_fallback_skin_name_scratch = 0;
static u32 s_custom_header_str = 0;
static u32 s_custom_header_scratch = 0;
static u32 s_custom_name_str = 0;
static u32 s_custom_name_scratch = 0;
static void *s_registered_repo = 0;
static void *s_registered_pack = 0;

static int build_texture_resource_path(char *out, unsigned cap, const char *path)
{
    unsigned len;
    if (!out || cap < 5 || !path) return 0;
    copy_text(out, path, cap);
    len = text_len(out);
    if (len < 5 || len + 1 > cap) return 0;
    if (out[len - 5] != '.' || out[len - 4] != '3' || out[len - 3] != 'd' ||
        out[len - 2] != 's' || out[len - 1] != 't') return 0;
    out[len - 4] = 'p';
    out[len - 3] = 'n';
    out[len - 2] = 'g';
    out[len - 1] = '\0';
    return 1;
}

static unsigned build_custom_native_skins(const u8 *standard_skins, unsigned standard_skin_count)
{
    int count;
    int i;
    unsigned j;
    u32 scratch;
    char serialized_name[48];
    char texture_resource_path[96];
    count = skin_service_scan_skins();
    if (count < 0) count = 0;
    if (count > NUMC3DS_MAX_CUSTOM_SKINS) count = NUMC3DS_MAX_CUSTOM_SKINS;
    if (count == 0) {
        const u8 *steve_src;
        u32 enabled;
        if (!standard_skins || !standard_skin_count) return 0;
        steve_src = standard_skins + (standard_skin_count > 1 ? 1 : 0) * 0x38;
        if (!s_fallback_skin_name) {
            ((StrCtorFn)SEAM_StrCtor)(&s_fallback_skin_name, "Steve", &s_fallback_skin_name_scratch);
        }
        enabled = *(const u8 *)(steve_src + 0x30);
        ((SkinCtorFn)SEAM_Skin_ctor)(s_custom_native_skins[0], steve_src + 0x0C,
                                     &s_fallback_skin_name,
                                     *(const u32 *)(steve_src + 0x14), steve_src + 0x1C,
                                     enabled, *(const u32 *)(steve_src + 0x18),
                                     *(const float *)(steve_src + 0x34));
        return 1;
    }
    for (i = 0; i < count; i++) {
        CustomSkinEntry *entry = skin_service_get_entry(i);
        u8 *skin = (u8 *)s_custom_native_skins[i];
        const u8 *template_skin;
        u32 category;
        u32 flags;
        float scale;
        u32 texture[0x14 / sizeof(u32)];
        if (!entry || !entry->is_valid || !standard_skins || !standard_skin_count) return 0;
        template_skin = standard_skins +
                        ((entry->model_type != SKIN_MODEL_SLIM && standard_skin_count > 1) ? 0x38 : 0);
        for (j = 0; j + 1 < sizeof(serialized_name) && entry->name[j]; j++) {
            serialized_name[j] = entry->name[j] == '_' ? '-' : entry->name[j];
        }
        serialized_name[j] = '\0';
        scratch = 0;
        ((StrCtorFn)SEAM_StrCtor)(&s_custom_skin_names[i], serialized_name, &scratch);
        scratch = 0;
        ((StrCtorFn)SEAM_StrCtor)(&s_custom_skin_labels[i], entry->name, &scratch);
        if (!build_texture_resource_path(texture_resource_path, sizeof(texture_resource_path),
                                         entry->texture_path)) return 0;
        zero(texture, sizeof(texture));
        ((ResourceLocationCtorFn)SEAM_ResourceLocation_ctor)(texture, texture_resource_path, 1);
        category = *(const u32 *)(template_skin + 0x14);
        flags = *(const u32 *)(template_skin + 0x18);
        scale = *(const float *)(template_skin + 0x34);
        ((SkinCtorFn)SEAM_Skin_ctor)(skin, &s_custom_skin_names[i], &s_custom_skin_labels[i],
                                     category, texture, 1, flags, scale);
        entry->skin_instance = skin;
    }
    return (unsigned)count;
}

static int append_pack_lookup(void *repo, void *pack)
{
    void ***vector;
    void **begin;
    void **end;
    void **capacity;
    void **replacement;
    u32 count;
    u32 old_capacity;
    u32 new_capacity;
    if (!repo || !pack) return 0;
    vector = (void ***)((u8 *)repo + 0x38);
    begin = vector[0];
    end = vector[1];
    capacity = vector[2];
    if (end != capacity) {
        *end = pack;
        vector[1] = end + 1;
        return 1;
    }
    count = begin ? (u32)(end - begin) : 0;
    old_capacity = begin ? (u32)(capacity - begin) : 0;
    new_capacity = old_capacity + (old_capacity >> 1) + (old_capacity >> 3);
    if (new_capacity < count + 1) new_capacity = count + 1;
    replacement = (void **)((VectorAllocateFn)SEAM_gstd_allocator_allocate)(new_capacity * sizeof(void *), 0);
    if (!replacement) return 0;
    if (count) cp(replacement, begin, count * sizeof(void *));
    replacement[count] = pack;
    if (begin) ((VectorDeallocateFn)SEAM_gstd_allocator_deallocate)(begin, old_capacity, 0);
    vector[0] = replacement;
    vector[1] = replacement + count + 1;
    vector[2] = replacement + new_capacity;
    return 1;
}

static int register_custom_skin_pack(void *repo)
{
    void *standard_vector;
    void *pack_vector;
    void **standard_packs;
    void **pack_begin;
    void **pack_end;
    void **cursor;
    void *custom_pack;
    void *standard_pack;
    u8 *standard_skins;
    u8 *standard_skins_end;
    unsigned standard_skin_count;
    unsigned skin_count;
    struct { void *begin; u32 count; } skins;
    if (!repo || repo == s_registered_repo) return repo != 0;
    standard_vector = ((GetSkinPacksByTypeFn)s_get_skin_packs_by_type_hook.trampoline)(repo, 0);
    standard_packs = standard_vector ? *(void ***)standard_vector : 0;
    standard_pack = standard_packs ? standard_packs[0] : 0;
    standard_skins = standard_pack ? *(u8 **)((u8 *)standard_pack + 0x10) : 0;
    standard_skins_end = standard_pack ? *(u8 **)((u8 *)standard_pack + 0x14) : 0;
    standard_skin_count = standard_skins && standard_skins_end
                            ? (unsigned)(standard_skins_end - standard_skins) / 0x38
                            : 0;
    skin_count = build_custom_native_skins(standard_skins, standard_skin_count);
    if (!skin_count) return 0;
    pack_vector = ((GetSkinPacksByTypeFn)s_get_skin_packs_by_type_hook.trampoline)(repo, 2);
    if (!pack_vector) return 0;
    if (!s_custom_name_str) ((StrCtorFn)SEAM_StrCtor)(&s_custom_name_str, "Custom", &s_custom_name_scratch);
    if (!s_custom_header_str) ((StrCtorFn)SEAM_StrCtor)(&s_custom_header_str, NUMC3DS_SKIN_PACK_NAME, &s_custom_header_scratch);
    skins.begin = s_custom_native_skins[0];
    skins.count = skin_count;
    s_registered_pack = ((HeapAllocFn)SEAM_Heap_allocWithSelector)(0x38, (void *)SEAM_game_alloc_selector);
    if (!s_registered_pack) return 0;
    ((SkinPackCtorFn)SEAM_SkinPack_ctor)(s_registered_pack, 1, &s_custom_name_str,
                                        &s_custom_header_str, 2, &skins);
    custom_pack = s_registered_pack;
    ((PackVectorPushFn)SEAM_SkinRepositoryEntryVector_pushBackMove)(pack_vector, &s_registered_pack);
    pack_begin = *(void ***)pack_vector;
    pack_end = *(void ***)((u8 *)pack_vector + 4);
    if (pack_begin && pack_end > pack_begin + 1) {
        custom_pack = pack_end[-1];
        cursor = pack_end - 1;
        while (cursor != pack_begin) {
            *cursor = cursor[-1];
            cursor--;
        }
        *pack_begin = custom_pack;
    }
    if (!append_pack_lookup(repo, custom_pack)) return 0;
    s_registered_repo = repo;
    return 1;
}

static void* on_get_skin_packs_by_type(void *repo, int type)
{
    if (type == 2 && repo != s_registered_repo) register_custom_skin_pack(repo);
    return ((GetSkinPacksByTypeFn)s_get_skin_packs_by_type_hook.trampoline)(repo, type);
}

static void on_skin_picker_render_button_tips(void *screen)
{
    CustomSkinEntry *hovered;
    u32 cur_pad;
    u32 pressed;
    int hovered_idx;

    /* Call original routine first to render [A] Select and [B] Back */
    ((SkinPickerRenderTipsFn)skin_picker_render_tips_hook.trampoline)(screen);

    if (!screen) return;

    /* Hardware key sampling for KEY_X (bit 10 = 0x00000400) */
    cur_pad = *(volatile u32 *)(0x1FF8101C);
    pressed = cur_pad & ~s_last_pad_buttons;
    s_last_pad_buttons = cur_pad;

    hovered_idx = skin_service_get_hovered_index();
    if (hovered_idx >= 0) {
        hovered = skin_service_get_entry(hovered_idx);
    } else {
        hovered = 0;
    }

    if (!hovered) return;

    /* Advanced 3D skins have custom geometry and do not support 2D slim/normal toggle */
    if (hovered->model_type == SKIN_MODEL_CUSTOM_3D) {
        return;
    }

    /* Handle X button press to toggle Slim vs Regular */
    if (pressed & 0x00000400u) {
        skin_service_toggle_model(hovered_idx >= 0 ? hovered_idx : 0);
    }

    /* Render additional localized controller tooltip for [X] Model: Normal/Slim */
    {
        void *render_ctx = *(void **)((u8 *)screen + 0x60);
        if (render_ctx) {
            u32 str_obj = 0;
            u32 scratch = 0;
            const char *label = (hovered->model_type == SKIN_MODEL_SLIM) ? "Model: Slim" : "Model: Normal";

            ((StrCtorFn)SEAM_StrCtor)(&str_obj, label, &scratch);

            /* Glyph 2 = Button X. Placed at x=180, y=220 on top screen (400x240) */
            ((RenderButtonTipFn)SEAM_Screen_renderButtonTip)(render_ctx, 180, 220, 2, &str_obj);

            ((StrDtorFn)SEAM_StrDtor)(&str_obj);
        }
    }
}

int skin_picker_hooks_install(void)
{
    int result;

    skin_picker_render_tips_hook.target = SEAM_SkinPickerScreen_renderButtonTips;
    skin_picker_render_tips_hook.replacement = (numc3ds_u32)on_skin_picker_render_button_tips;
    skin_picker_render_tips_hook.expected[0] = 0xE92D4FF0u; /* push {r4-r11, lr} */
    skin_picker_render_tips_hook.expected[1] = 0xE1A04000u; /* mov r4, r0 */
    result = s->host.install_hook(&skin_picker_render_tips_hook);
    if (result) return result;

    s_get_skin_packs_by_type_hook.target = SEAM_SkinRepository_getSkinPacksByType;
    s_get_skin_packs_by_type_hook.replacement = (numc3ds_u32)on_get_skin_packs_by_type;
    s_get_skin_packs_by_type_hook.expected[0] = 0xE52D4004u; /* str r4, [sp, #-4]! */
    s_get_skin_packs_by_type_hook.expected[1] = 0xE590302Cu; /* ldr r3, [r0, #0x2C] */
    result = s->host.install_hook(&s_get_skin_packs_by_type_hook);
    if (result) return result;

    return 0;
}

void *skin_picker_get_repository(void)
{
    if (s_registered_repo) return s_registered_repo;
    if (!s || !s->host.minecraft_game) return 0;
    return ((void *(*)(void *))SEAM_MinecraftGame_getSkinRepository)(s->host.minecraft_game);
}
