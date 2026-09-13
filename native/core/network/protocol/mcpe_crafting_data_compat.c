#include "mcpe_crafting_data_compat.h"
#include "../../../diagnostics/network_debug.h"
#include "../../hook_manager.h"
#include "../../seams.h"
#include "../../state.h"

typedef u32 (*GetUnsignedVarIntFn)(void *);
typedef int (*ReadVarIntFn)(void *);
typedef void (*ReadItemInstanceFn)(void *, void *);
typedef void (*ItemInstanceDtorFn)(void *);
typedef void (*GetTypeUUIDFn)(void *, void *);
typedef void *(*VectorAllocateFn)(u32, u32);
typedef void (*VectorDeallocateFn)(void *, u32, void *);
typedef void *(*ShapelessRecipeCtorFn)(void *, void *, void *, int, int, void *);
typedef void *(*ShapedRecipeCtorFn)(void *, int, int, void *, void *, int, int, void *);

#define ITEM_INSTANCE_SIZE 0x30u
#define MAX_RECIPE_ITEMS 64u
#define MAX_SHAPED_DIMENSION 3

typedef struct {
    u8 bytes[ITEM_INSTANCE_SIZE];
} NativeItemInstance;

typedef struct {
    NativeItemInstance *begin;
    NativeItemInstance *end;
    NativeItemInstance *capacity;
} NativeItemVector;

static NuMC3DS_Hook shaped_recipe_read_hook;
static NuMC3DS_Hook shapeless_recipe_read_hook;
static NuMC3DS_Hook clear_recipes_hook;
static NuMC3DS_Hook clear_furnace_recipes_hook;

static void on_clear_recipes(void *recipes)
{
    (void)recipes;
}

static void on_clear_furnace_recipes(void *recipes)
{
    (void)recipes;
}

static void recipe_vector_init(NativeItemVector *vector)
{
    vector->begin = 0;
    vector->end = 0;
    vector->capacity = 0;
}

static void recipe_vector_destroy(NativeItemVector *vector)
{
    NativeItemInstance *item;
    u32 capacity;

    if (!vector->begin) return;
    for (item = vector->begin; item != vector->end; ++item) {
        ((ItemInstanceDtorFn)SEAM_ItemInstance_dtor)(item);
    }
    capacity = (u32)(vector->capacity - vector->begin);
    ((VectorDeallocateFn)SEAM_gstd_allocator_deallocate)(vector->begin, capacity, 0);
    recipe_vector_init(vector);
}

static void reject_stream(void *stream, const char *field, u32 value)
{
    u32 *words = (u32 *)stream;

    net_log_open(NET_LOG_INFO, "crafting", "invalid protocol-113 recipe");
    net_log_text("field", field);
    net_log_dec("value", value);
    net_log_close();

    if (words && words[3]) {
        u32 owner = *(u32 *)words[3];
        if (owner) words[1] = *(u32 *)(owner - 4u);
    }
}

static int read_recipe_items(void *stream, NativeItemVector *vector, u32 count)
{
    u32 index;

    recipe_vector_init(vector);
    if (count > MAX_RECIPE_ITEMS) {
        reject_stream(stream, "item_count", count);
        return 0;
    }
    if (!count) return 1;

    vector->begin = (NativeItemInstance *)((VectorAllocateFn)SEAM_gstd_allocator_allocate)(
        count * ITEM_INSTANCE_SIZE, 0);
    if (!vector->begin) {
        reject_stream(stream, "allocation", count);
        return 0;
    }
    vector->end = vector->begin;
    vector->capacity = vector->begin + count;

    for (index = 0; index < count; ++index) {
        ((ReadItemInstanceFn)SEAM_ReadOnlyBinaryStream_readItemInstance)(vector->end, stream);
        ++vector->end;
    }
    return 1;
}

static int read_recipe_vector(void *stream, NativeItemVector *vector)
{
    u32 count = ((GetUnsignedVarIntFn)SEAM_ReadOnlyBinaryStream_getUnsignedVarInt)(stream);
    return read_recipe_items(stream, vector, count);
}

static void on_read_shapeless_recipe(void *out, void *stream)
{
    NativeItemVector ingredients;
    NativeItemVector results;
    u32 uuid[4] = {0, 0, 0, 0};

    if (!out || !stream) return;

    recipe_vector_init(&ingredients);
    recipe_vector_init(&results);
    if (read_recipe_vector(stream, &ingredients) &&
        read_recipe_vector(stream, &results)) {
        ((GetTypeUUIDFn)SEAM_ReadOnlyBinaryStream_getTypeUUID)(uuid, stream);
    }
    ((ShapelessRecipeCtorFn)SEAM_ShapelessRecipe_ctorFromNetwork)(
        out, &ingredients, &results, 0, 0, uuid);
    recipe_vector_destroy(&results);
    recipe_vector_destroy(&ingredients);
}

static void on_read_shaped_recipe(void *out, void *stream)
{
    NativeItemVector ingredients;
    NativeItemVector results;
    u32 uuid[4] = {0, 0, 0, 0};
    int width;
    int height;

    if (!out || !stream) return;

    recipe_vector_init(&ingredients);
    recipe_vector_init(&results);
    width = ((ReadVarIntFn)SEAM_ReadOnlyBinaryStream_readVarInt)(stream);
    height = ((ReadVarIntFn)SEAM_ReadOnlyBinaryStream_readVarInt)(stream);
    if (width < 1 || width > MAX_SHAPED_DIMENSION ||
        height < 1 || height > MAX_SHAPED_DIMENSION) {
        reject_stream(stream, "shaped_dimensions",
                      ((u32)(u16)width << 16) | (u32)(u16)height);
        width = 0;
        height = 0;
    } else if (read_recipe_items(stream, &ingredients, (u32)(width * height)) &&
               read_recipe_vector(stream, &results)) {
        ((GetTypeUUIDFn)SEAM_ReadOnlyBinaryStream_getTypeUUID)(uuid, stream);
    }
    ((ShapedRecipeCtorFn)SEAM_ShapedRecipe_ctorFromNetwork)(
        out, width, height, &ingredients, &results, 0, 0, uuid);
    recipe_vector_destroy(&results);
    recipe_vector_destroy(&ingredients);
}

int mcpe_crafting_data_compat_install(void)
{
    zero(&shaped_recipe_read_hook, sizeof(shaped_recipe_read_hook));
    shaped_recipe_read_hook.target = SEAM_ReadOnlyBinaryStream_readTypeShapedRecipe;
    shaped_recipe_read_hook.replacement = (u32)on_read_shaped_recipe;
    shaped_recipe_read_hook.expected[0] = 0xE92D4FF3u;
    shaped_recipe_read_hook.expected[1] = 0xE24DDF4Bu;
    if (s->host.install_hook(&shaped_recipe_read_hook)) return -90;

    zero(&shapeless_recipe_read_hook, sizeof(shapeless_recipe_read_hook));
    shapeless_recipe_read_hook.target = SEAM_ReadOnlyBinaryStream_readTypeShapelessRecipe;
    shapeless_recipe_read_hook.replacement = (u32)on_read_shapeless_recipe;
    shapeless_recipe_read_hook.expected[0] = 0xE92D41F0u;
    shapeless_recipe_read_hook.expected[1] = 0xE24DD0A8u;
    if (s->host.install_hook(&shapeless_recipe_read_hook)) return -91;

    zero(&clear_recipes_hook, sizeof(clear_recipes_hook));
    clear_recipes_hook.target = SEAM_Recipes_clearRecipes;
    clear_recipes_hook.replacement = (u32)on_clear_recipes;
    clear_recipes_hook.expected[0] = 0xE92D47F0u;
    clear_recipes_hook.expected[1] = 0xE1A07000u;
    if (s->host.install_hook(&clear_recipes_hook)) return -92;

    zero(&clear_furnace_recipes_hook, sizeof(clear_furnace_recipes_hook));
    clear_furnace_recipes_hook.target = SEAM_FurnaceRecipes_clearRecipes;
    clear_furnace_recipes_hook.replacement = (u32)on_clear_furnace_recipes;
    clear_furnace_recipes_hook.expected[0] = 0xE92D4010u;
    clear_furnace_recipes_hook.expected[1] = 0xE590C010u;
    if (s->host.install_hook(&clear_furnace_recipes_hook)) return -93;

    return 0;
}
