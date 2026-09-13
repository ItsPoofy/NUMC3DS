#include "map_item.h"
#include "../util/string_util.h"
#include "../rt.h"
#include "../state.h"

typedef void (*RegisterRecipesFn)(void*);
typedef void (*GetMapUuidFn)(u32*, void*);
typedef int  (*LevelDoesMapExistFn)(void*, int, u32, u32);
typedef void* (*LevelGetMapSavedDataFn)(void*, int, u32, u32);
typedef void  (*TexturePtrAssignFn)(void*, void*);
typedef void* (*TextureGroupGetTextureFn)(void*, void*);
typedef void* (*CreateDynamicTextureFn)(void*, void*, int, int);
typedef void* (*ResourceLocationCtorFn)(void*, const char*, int);
typedef void* (*GstdStringCopyCtorFn)(void*, void*);
typedef void* (*GstdStringConcatFn)(void*, const char*, void*);
typedef void  (*GstdStringDtorFn)(void*);
typedef void  (*ItemInstanceIdCountAuxCtorFn)(void*, int, int, int);
typedef void  (*ItemInstanceDtorFn)(void*);
typedef void  (*RecipesDefinitionItemFn)(void*, char, void*);
typedef void  (*RecipesDefinitionPairFn)(void*, char, void*, char, void*);
typedef void  (*RecipesAddShapedRecipe3RowsFn)(void*, void*, void*, void*, void*, void*, int, int);
typedef void  (*VecClearFn)(void*);
typedef void  (*CreativeItemListPushFn)(void*);
typedef void  (*CreativeItemListInitFn)(void);
typedef void* (*TextureGetImageBufferFn)(void*);
typedef void* (*ImageBufferFn)(void*, int, int);
typedef void  (*FlushTextureFn)(void*);
typedef int   (*CompoundTagGetBoolFn)(void*, void*);
typedef void* (*EntityGetLevelFn)(void*);
typedef void* (*LevelCreateMapSavedDataInternalFn)(void*, const u32*);
typedef void  (*ItemInstanceCopyAssignFn)(void*, const void*);
typedef void  (*MapItemSetMapIdFn)(void*, int);
typedef void  (*MapItemSetMapSavedDataFn)(void*, void*);
typedef void  (*CompoundTagPutBoolFn)(void*, void*, int);
typedef int   (*PlayerAddFn)(void*, void*, int);
typedef int   (*PlayerGetDimFn)(void*);

#define SEAM_GetMapUuid                  0x0062F3F4u
#define SEAM_Level_doesMapExist          0x005C5DF0u
#define SEAM_Level_getMapSavedData       0x005C869Cu
#define SEAM_TextureGroup_getTexture     0x004F3F88u
#define SEAM_TextureGroup_createDynamicTexture 0x004F4038u
#define SEAM_ResourceLocation_ctor       0x0033C630u
#define SEAM_GstdString_copyCtor         0x002FF261u
#define SEAM_GstdString_concat           0x00917B28u
#define SEAM_GstdString_dtor             0x002FEBBDu
#define SEAM_ItemInstance_idCountAuxCtor 0x001D2894u
#define SEAM_ItemInstance_dtor           0x001D295Cu
#define SEAM_Recipes_definitionItem      0x007CA74Cu
#define SEAM_Recipes_definitionPair      0x007CA97Cu
#define SEAM_Recipes_addShaped3Rows      0x006367C0u
#define SEAM_Recipes_defVecClear         0x008FF4F0u
#define SEAM_CreativeItemList_push       0x0056E108u
#define SEAM_Texture_getImageBuffer      0x001B340Cu
#define SEAM_Core_imageBuffer            0x004F0C44u
#define SEAM_Texture_flush               0x001B34B4u
#define SEAM_MapTexturePrefix            0x00677684u
#define SEAM_Entity_getLevel                  0x005F741Cu
#define SEAM_Level_createMapSavedDataInternal 0x005C9838u
#define SEAM_ItemInstance_copyAssign          0x001D2C3Cu
#define SEAM_MapItem_setMapId                 0x0062F1BCu
#define SEAM_MapItem_setMapSavedData          0x0062F23Cu
#define SEAM_CompoundTag_putBoolean           0x0018184Cu

void map_log3(char tag, u32 a, u32 b) {
    char line[22];
    unsigned i, j;
    static const char digits[] = "0123456789ABCDEF";
    if (!s || !s->host.debug_string) return;
    line[0] = tag;
    line[1] = ' ';
    for (i = 0; i < 2; i++) {
        u32 v = i ? b : a;
        for (j = 0; j < 8; j++) line[2 + i * 9 + j] = digits[(v >> ((7 - j) * 4)) & 15u];
        line[10 + i * 9] = ' ';
    }
    line[20] = '\n';
    line[21] = 0;
    s->host.debug_string(line, 20);
}

static int map_uuid_valid(u32 *uuid) {
    if (!uuid) return 0;
    if (uuid[0] == 0 && uuid[1] == 0) return 0;
    if (uuid[0] == 0xFFFFFFFFu && uuid[1] == 0xFFFFFFFFu) return 0;
    return 1;
}

void* map_savedata_for(void *level, void *item) {
    u32 uuid[2];
    int exists;
    if (!level || !item) return 0;
    uuid[0] = 0; uuid[1] = 0;
    ((GetMapUuidFn)SEAM_GetMapUuid)(uuid, item);
    if (!map_uuid_valid(uuid)) return 0;
    exists = ((LevelDoesMapExistFn)SEAM_Level_doesMapExist)(level, 0, uuid[0], uuid[1]);
    if (!exists) return 0;
    return ((LevelGetMapSavedDataFn)SEAM_Level_getMapSavedData)(level, 0, uuid[0], uuid[1]);
}

int is_locator_map(void *item) {
    void *tag;
    if (!item) return 0;
    if (*(short*)((char*)item + 2) == 2) return 1;
    tag = *(void**)((char*)item + 8);
    if (tag) {
        #define SEAM_CompoundTag_getBoolean 0x006A0A4Cu
        #define SEAM_Tag_map_display_players 0x00A34330u
        return ((CompoundTagGetBoolFn)SEAM_CompoundTag_getBoolean)(tag, (void*)SEAM_Tag_map_display_players);
    }
    return 0;
}

void* map_upload_pixels(void *mc, void *mapData) {
    char suuid[32], comb[32], rl[32];
    void *texGroup, *tex, *core;
    u32 *dst, *src, chk, cur, px;
    int row, col;
    const char *comb_str;

    if (!mapData) return 0;
    src = *(u32**)((char*)mapData + 0x20);
    if (!src) return 0;

    zero(suuid, sizeof(suuid));
    ((GstdStringCopyCtorFn)SEAM_GstdString_copyCtor)(suuid, (char*)mapData + 8);
    zero(comb, sizeof(comb));
    ((GstdStringConcatFn)SEAM_GstdString_concat)(comb, (const char*)SEAM_MapTexturePrefix, suuid);

    comb_str = *(const char**)comb;
    if (!comb_str) {
        ((GstdStringDtorFn)SEAM_GstdString_dtor)(comb);
        ((GstdStringDtorFn)SEAM_GstdString_dtor)(suuid);
        return 0;
    }

    zero(rl, sizeof(rl));
    ((ResourceLocationCtorFn)SEAM_ResourceLocation_ctor)(rl, comb_str, 1);

    texGroup = mc ? *(void**)((char*)mc + 0x58) : 0;
    tex = 0;
    if (texGroup) {
        tex = ((TextureGroupGetTextureFn)SEAM_TextureGroup_getTexture)(texGroup, rl);
        if (!tex || tex == texGroup || (u32)tex < 0x00100000u) {
            tex = ((CreateDynamicTextureFn)SEAM_TextureGroup_createDynamicTexture)(texGroup, rl, 128, 128);
        }
    }

    ((GstdStringDtorFn)SEAM_GstdString_dtor)(comb);
    ((GstdStringDtorFn)SEAM_GstdString_dtor)(suuid);

    if (!tex || tex == texGroup || (u32)tex < 0x00100000u) return 0;

    core = ((TextureGetImageBufferFn)SEAM_Texture_getImageBuffer)(tex);
    if (!core || (u32)core < 0x00100000u || *(void**)core == 0) return tex;
    dst = (u32*)((ImageBufferFn)SEAM_Core_imageBuffer)(core, 0, 0);
    if (!dst || (u32)dst < 0x00100000u) return tex;

    int changed = 0;
    chk = 0x10000000u;
    for (row = 0; row < 128; row++) {
        for (col = 0; col < 128; col++) {
            cur = chk;
            px = *src;
            if (px == 0) px = cur;
            if (*dst != px) {
                *dst = px;
                changed = 1;
            }
            dst++;
            src++;
            chk = cur ^ 0x08000000u;
        }
        chk ^= 0x08000000u;
    }
    if(changed)((FlushTextureFn)SEAM_Texture_flush)(tex);
    return tex;
}

void map_copy_pixels_to_texture(void *tex, void *mapData) {
    u32 *dst, *src, chk, cur, px;
    int row, col, changed=0;
    void *core;

    if (!tex || !mapData) return;
    src = *(u32**)((char*)mapData + 0x20);
    if (!src) return;

    core = ((TextureGetImageBufferFn)SEAM_Texture_getImageBuffer)(tex);
    if (!core || (u32)core < 0x00100000u || *(void**)core == 0) return;
    dst = (u32*)((ImageBufferFn)SEAM_Core_imageBuffer)(core, 0, 0);
    if (!dst || (u32)dst < 0x00100000u) return;

    chk = 0x10000000u;
    for (row = 0; row < 128; row++) {
        for (col = 0; col < 128; col++) {
            cur = chk;
            px = *src;
            if (px == 0) px = cur;
            if (*dst != px) {
                *dst = px;
                changed=1;
            }
            dst++;
            src++;
            chk = cur ^ 0x08000000u;
        }
        chk ^= 0x08000000u;
    }
    if(changed)((FlushTextureFn)SEAM_Texture_flush)(tex);
}

void map_fill_blank_texture(void *tex) {
    u32 *dst, chk, cur;
    int row, col, changed=0;
    void *core;

    if (!tex) return;
    core = ((TextureGetImageBufferFn)SEAM_Texture_getImageBuffer)(tex);
    if (!core || (u32)core < 0x00100000u || *(void**)core == 0) return;
    dst = (u32*)((ImageBufferFn)SEAM_Core_imageBuffer)(core, 0, 0);
    if (!dst || (u32)dst < 0x00100000u) return;

    chk = 0x10000000u;
    for (row = 0; row < 128; row++) {
        for (col = 0; col < 128; col++) {
            cur = chk;
            if(*dst!=cur){*dst=cur;changed=1;}
            dst++;
            chk = cur ^ 0x08000000u;
        }
        chk ^= 0x08000000u;
    }
    if(changed)((FlushTextureFn)SEAM_Texture_flush)(tex);
}

void map_upload_held_pixels(void *mc, void *sectorData, void *heldData) {
    (void)sectorData;
    map_upload_pixels(mc, heldData);
}

void register_map_items(void) {
    /* Vanilla Item::registerItems (0x00563DB0) registers items[395] (EmptyMapItem)
       and items[358] (MapItem) with their authentic native vtables. */
}

void map_item_add_creative_items(void) {
    char inst[0xC0];
    u16 *p_cat, *p_sort;

    map_log3('C', (u32)((void**)0x00B0CEF0u)[395], (u32)((void**)0x00B0CEF0u)[358]);
    { void **cv = (void**)0x00B0D744u; map_log3('V', (u32)cv[0], (u32)cv[1]); }

    /* 1. Empty Map (ID 395, count 1, aux 0) */
    zero(inst, sizeof(inst));
    ((ItemInstanceIdCountAuxCtorFn)SEAM_ItemInstance_idCountAuxCtor)(inst, 395, 1, 0);
    p_cat = (u16*)(inst + 0x14);
    p_sort = (u16*)(inst + 0x16);
    *p_cat = 5; /* Category 5 = Tools */
    *p_sort = 105;
    ((CreativeItemListPushFn)SEAM_CreativeItemList_push)(inst);
    ((ItemInstanceDtorFn)SEAM_ItemInstance_dtor)(inst);

    /* 2. Locator Map (ID 395, count 1, aux 2) */
    zero(inst, sizeof(inst));
    ((ItemInstanceIdCountAuxCtorFn)SEAM_ItemInstance_idCountAuxCtor)(inst, 395, 1, 2);
    p_cat = (u16*)(inst + 0x14);
    p_sort = (u16*)(inst + 0x16);
    *p_cat = 5; /* Category 5 = Tools */
    *p_sort = 106;
    ((CreativeItemListPushFn)SEAM_CreativeItemList_push)(inst);
    ((ItemInstanceDtorFn)SEAM_ItemInstance_dtor)(inst);

    { void **cv = (void**)0x00B0D744u; map_log3('V', (u32)cv[0], (u32)cv[1]); }
}

static void register_map_recipes(void *recipes) {
    char result_map[0xC0], paper[0xC0], compass[0xC0], def_vec[12];
    u32 row1 = 0, row2 = 0, row3 = 0, scratch = 0;
    if (!recipes) return;

    /* 1. Empty Map recipe: 9 Paper (3x3) -> 1 Empty Map */
    ((ItemInstanceIdCountAuxCtorFn)SEAM_ItemInstance_idCountAuxCtor)(result_map, 395, 1, 0);
    ((ItemInstanceIdCountAuxCtorFn)SEAM_ItemInstance_idCountAuxCtor)(paper, 339, 1, 0);
    row1 = 0; scratch = 0; ((StrCtor)SEAM_StrCtor)(&row1, "###", &scratch);
    row2 = 0; scratch = 0; ((StrCtor)SEAM_StrCtor)(&row2, "###", &scratch);
    row3 = 0; scratch = 0; ((StrCtor)SEAM_StrCtor)(&row3, "###", &scratch);
    ((RecipesDefinitionItemFn)SEAM_Recipes_definitionItem)(def_vec, '#', paper);
    ((RecipesAddShapedRecipe3RowsFn)SEAM_Recipes_addShaped3Rows)(recipes, result_map, &row1, &row2, &row3, def_vec, 6, 0);
    ((VecClearFn)SEAM_Recipes_defVecClear)(def_vec);
    ((StrDtor)SEAM_StrDtor)(&row3);
    ((StrDtor)SEAM_StrDtor)(&row2);
    ((StrDtor)SEAM_StrDtor)(&row1);
    ((ItemInstanceDtorFn)SEAM_ItemInstance_dtor)(paper);
    ((ItemInstanceDtorFn)SEAM_ItemInstance_dtor)(result_map);

    /* 2. Locator Map recipe: 8 Paper + 1 Compass -> 1 Locator Map */
    ((ItemInstanceIdCountAuxCtorFn)SEAM_ItemInstance_idCountAuxCtor)(result_map, 395, 1, 2);
    ((ItemInstanceIdCountAuxCtorFn)SEAM_ItemInstance_idCountAuxCtor)(paper, 339, 1, 0);
    ((ItemInstanceIdCountAuxCtorFn)SEAM_ItemInstance_idCountAuxCtor)(compass, 345, 1, 0);
    row1 = 0; scratch = 0; ((StrCtor)SEAM_StrCtor)(&row1, "###", &scratch);
    row2 = 0; scratch = 0; ((StrCtor)SEAM_StrCtor)(&row2, "#X#", &scratch);
    row3 = 0; scratch = 0; ((StrCtor)SEAM_StrCtor)(&row3, "###", &scratch);
    ((RecipesDefinitionPairFn)SEAM_Recipes_definitionPair)(def_vec, '#', paper, 'X', compass);
    ((RecipesAddShapedRecipe3RowsFn)SEAM_Recipes_addShaped3Rows)(recipes, result_map, &row1, &row2, &row3, def_vec, 6, 0);
    ((VecClearFn)SEAM_Recipes_defVecClear)(def_vec);
    ((StrDtor)SEAM_StrDtor)(&row3);
    ((StrDtor)SEAM_StrDtor)(&row2);
    ((StrDtor)SEAM_StrDtor)(&row1);
    ((ItemInstanceDtorFn)SEAM_ItemInstance_dtor)(compass);
    ((ItemInstanceDtorFn)SEAM_ItemInstance_dtor)(paper);
    ((ItemInstanceDtorFn)SEAM_ItemInstance_dtor)(result_map);
}

void on_recipes_register_recipes(void *recipes) {
    ((RegisterRecipesFn)s->recipes_register.trampoline)(recipes);
    register_map_recipes(recipes);
}

void* map_player_server_level(void *player) {
    void *bs;
    if (!player) return 0;
    bs = *(void**)((char*)player + 0x210);
    if (!bs) return 0;
    return *(void**)((char*)bs + 0x0C);
}

void* on_empty_map_use(void *item_ptr, void *held, void *player) {
    void *level;
    u32 *p_map_counter;
    u32 map_id_low, map_id_high;
    u32 uuid[2];
    u32 *p_player_map_id;
    u32 player_map_id;
    void *mapSavedData;
    int is_locator;
    float *p_pos;
    float px, pz;
    int dimension;
    char new_map_inst[0xC0];
    u8 count;

    if (!item_ptr || !held || !player) return held;

    level = map_player_server_level(player);
    if (!level) {
        level = ((EntityGetLevelFn)SEAM_Entity_getLevel)(player);
    }
    if (!level) return held;

    is_locator = is_locator_map(held);

    /* Allocate new unique map ID on the server level */
    p_map_counter = (u32*)((char*)level + 0x48);
    map_id_low = p_map_counter[0] + 1;
    map_id_high = p_map_counter[1];
    if (map_id_low == 0) map_id_high++;
    p_map_counter[0] = map_id_low;
    p_map_counter[1] = map_id_high;
    uuid[0] = map_id_low;
    uuid[1] = map_id_high;

    /* Increment player map index */
    p_player_map_id = (u32*)((char*)player + 0x1978);
    player_map_id = *p_player_map_id;
    *p_player_map_id = player_map_id + 1;

    /* Create MapItemSavedData in server level's mMapData */
    mapSavedData = ((LevelCreateMapSavedDataInternalFn)SEAM_Level_createMapSavedDataInternal)(level, uuid);
    if (!mapSavedData) return held;

    /* Initialize MapItemSavedData fields: scale 2 */
    *(u8*)((char*)mapSavedData + 28) = 2; /* mScale = 2 */

    /* Player position: Vec3 at player + 0x1C0 */
    p_pos = (float*)((char*)player + 0x1C0);
    px = p_pos[0];
    pz = p_pos[2];

    /* Authentic MapItemSavedData::init math (0x0032790C): 128 << scale grid centering */
    {
        int step = 128 << 2; /* 512 for scale 2 */
        float fx = (px + 64.0f) / (float)step;
        int gx = (int)fx;
        if (fx < (float)gx) gx--;
        *(int*)((char*)mapSavedData + 12) = gx * step + (step / 2) - 64; /* mCenterX */

        float fz = (pz + 64.0f) / (float)step;
        int gz = (int)fz;
        if (fz < (float)gz) gz--;
        *(int*)((char*)mapSavedData + 20) = gz * step + (step / 2) - 64; /* mCenterZ */
    }

    /* Player dimension: field at player + 0x198 */
    dimension = *(int*)((char*)player + 0x198);
    *(int*)((char*)mapSavedData + 24) = dimension;
    *(u8*)((char*)mapSavedData + 4) = 1; /* mIsDirty = 1 */

    /* Construct filled map ItemInstance (ID 358, count 1, aux 2 if locator else 0) */
    zero(new_map_inst, sizeof(new_map_inst));
    ((ItemInstanceIdCountAuxCtorFn)SEAM_ItemInstance_idCountAuxCtor)(new_map_inst, 358, 1, is_locator ? 2 : 0);

    /* Set map ID & ensure NBT tag exists */
    ((MapItemSetMapIdFn)SEAM_MapItem_setMapId)(new_map_inst, (int)player_map_id);

    /* If locator map, tag with map_display_players = 1 */
    if (is_locator) {
        void *tag = *(void**)((char*)new_map_inst + 8);
        if (tag) {
            ((CompoundTagPutBoolFn)SEAM_CompoundTag_putBoolean)(tag, (void*)SEAM_Tag_map_display_players, 1);
        }
    }

    /* Bind mapSavedData UUID to new_map_inst */
    ((MapItemSetMapSavedDataFn)SEAM_MapItem_setMapSavedData)(new_map_inst, mapSavedData);

    /* Update player inventory */
    count = *(u8*)((char*)held + 0);
    if (count <= 1) {
        /* Replace held empty map with the filled map */
        ((ItemInstanceCopyAssignFn)SEAM_ItemInstance_copyAssign)(held, new_map_inst);
    } else {
        void *supplies = *(void**)((char*)player + 0x1264);
        int added = 0;
        *(u8*)((char*)held + 0) = count - 1;
        if (supplies) {
            #define SEAM_Player_add 0x003F94A0u
            added = ((int(*)(void*, void*, int))SEAM_Player_add)(supplies, new_map_inst, 1);
        }
        if (!added) {
            void **vtab = *(void***)player;
            PlayerAddFn player_drop = vtab ? (PlayerAddFn)vtab[161] : 0;
            if (player_drop) {
                player_drop(player, new_map_inst, 0);
            }
        }
    }

    /* Destroy temporary stack ItemInstance */
    ((ItemInstanceDtorFn)SEAM_ItemInstance_dtor)(new_map_inst);

    return held;
}

#include "map_shading.h"
#include "map_sample_height.h"
#include "map_update_schedule.h"
#include "map_marker_orientation.h"
#include "../render/item_frame_map.h"

int map_item_install_hooks(void) {
    NuMC3DS_Hook *hook;
    int result=map_shading_install();
    if(result)return result;
    result=map_update_schedule_install();
    if(result)return result;
    result=map_sample_height_install();
    if(result)return result;
    result=map_marker_orientation_install();
    if(result)return result;
    result=item_frame_map_install();
    if(result)return result;

    hook = &s->recipes_register;
    zero(hook, sizeof(*hook));
    hook->target = 0x00638CC0u; /* Recipes::registerRecipes */
    hook->expected[0] = 0xE92D4FF0u;
    hook->expected[1] = 0xE3A06000u;
    hook->replacement = (numc3ds_u32)on_recipes_register_recipes;
    result = s->host.install_hook(hook);
    if (result) return result;

    hook = &s->empty_map_use;
    zero(hook, sizeof(*hook));
    hook->target = 0x006AF990u; /* EmptyMapItem::use */
    hook->expected[0] = 0xE92D4FF0u;
    hook->expected[1] = 0xE24DD0FCu;
    hook->replacement = (numc3ds_u32)on_empty_map_use;
    return s->host.install_hook(hook);
}
