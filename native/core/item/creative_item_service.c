#include "creative_item_service.h"
#include "map_item.h"
#include "../internal.h"
#include "../state.h"

typedef void (*CreativeItemListInitFn)(void);
typedef void (*ItemInstanceIdCountAuxCtorFn)(void*, int, int, int);
typedef void (*ItemInstanceDtorFn)(void*);
typedef void (*CreativeItemListPushFn)(void*);

#define SEAM_ItemInstance_idCountAuxCtor 0x001D2894u
#define SEAM_ItemInstance_dtor           0x001D295Cu
#define SEAM_CreativeItemList_push       0x0056E108u

static void potion_add_creative_items(void) {
    char inst[0xC0];
    u16 *p_cat, *p_sort;

    /* Long Mundane Potion (ID 373, count 1, aux 2, Category 2 = Misc, Sort 1005) */
    zero(inst, sizeof(inst));
    ((ItemInstanceIdCountAuxCtorFn)SEAM_ItemInstance_idCountAuxCtor)(inst, 373, 1, 2);
    p_cat = (u16*)(inst + 0x14);
    p_sort = (u16*)(inst + 0x16);
    *p_cat = 2;
    *p_sort = 1005;
    ((CreativeItemListPushFn)SEAM_CreativeItemList_push)(inst);
    ((ItemInstanceDtorFn)SEAM_ItemInstance_dtor)(inst);

    /* Splash Long Mundane Potion (ID 438, count 1, aux 2, Category 2 = Misc, Sort 1085) */
    zero(inst, sizeof(inst));
    ((ItemInstanceIdCountAuxCtorFn)SEAM_ItemInstance_idCountAuxCtor)(inst, 438, 1, 2);
    p_cat = (u16*)(inst + 0x14);
    p_sort = (u16*)(inst + 0x16);
    *p_cat = 2;
    *p_sort = 1085;
    ((CreativeItemListPushFn)SEAM_CreativeItemList_push)(inst);
    ((ItemInstanceDtorFn)SEAM_ItemInstance_dtor)(inst);
}

static void on_creative_item_list_initialize(void) {
    ((CreativeItemListInitFn)s->creative_initialize.trampoline)();

    map_item_add_creative_items();
    potion_add_creative_items();
}

int creative_item_service_install_hook(void) {
    NuMC3DS_Hook *hook = &s->creative_initialize;
    zero(hook, sizeof(*hook));
    hook->target = 0x0056E450u; /* CreativeItemList::initialize */
    hook->expected[0] = 0xE92D4FF0u;
    hook->expected[1] = 0xE24DD054u;
    hook->replacement = (numc3ds_u32)on_creative_item_list_initialize;
    return s->host.install_hook(hook);
}
