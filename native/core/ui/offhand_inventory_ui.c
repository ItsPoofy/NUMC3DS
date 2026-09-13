#include "offhand_inventory_ui.h"
#include "../internal.h"
#include "../state.h"

#define SEAM_Mob_getArmorSlot 0x00714254u
#define SEAM_ContainerInventoryScreen_setItemSlot 0x0042DF94u
#define SEAM_ContainerInventoryScreen_setupNavigation 0x0042F02Cu
#define SEAM_ContainerInventoryScreen_getItem 0x00706948u
#define SEAM_ContainerInventoryScreen_isValidSlot 0x00706AA4u
#define SEAM_ContainerInventoryScreen_getMaxStackSizeForSlot 0x00706A14u
#define SEAM_BaseSurvivalScreen_container_findOrInsert 0x008E270Cu
#define SEAM_LocalPlayer_setOffhandSlot 0x00194CE0u
#define SEAM_ClearSelection 0x0062D3E0u
#define SEAM_ItemInstance_isNull 0x006B2954u
#define SEAM_ItemInstance_copyCtor 0x001D2768u
#define SEAM_ItemInstance_ctor 0x001D28F4u
#define SEAM_ItemInstance_getMaxStackSize 0x006B1DA0u
#define SEAM_canStack 0x006FC434u

static const char offhand_armor_hook_fail[] = "NuMC3DS UI: offhand getArmorSlot hook failed\n";
static const char offhand_set_slot_hook_fail[] = "NuMC3DS UI: offhand setItemSlot hook failed\n";
static const char offhand_nav_hook_fail[] = "NuMC3DS UI: offhand setupNav hook failed\n";
static const char offhand_get_item_hook_fail[] = "NuMC3DS UI: offhand getItem hook failed\n";
static const char offhand_valid_slot_hook_fail[] = "NuMC3DS UI: offhand isValidSlot hook failed\n";
static const char offhand_max_stack_hook_fail[] = "NuMC3DS UI: offhand getMaxStack hook failed\n";

static NuMC3DS_Hook s_container_inventory_screen_get_item;
static NuMC3DS_Hook s_container_inventory_screen_is_valid_slot;
static NuMC3DS_Hook s_container_inventory_screen_get_max_stack_size_for_slot;

typedef void *(*GetArmorSlotFn)(void *, int);
typedef void (*SetItemSlotFn)(void *, int, const void *);
typedef void (*SetOffhandSlotFn)(void *, const void *);
typedef void (*ClearSelectionFn)(void *, int);
typedef void *(*GetNavNodeFn)(void *, const int *);
typedef void (*SetupNavFn)(void *);
typedef void (*GetItemFn)(void *, void *, int);
typedef int (*IsValidSlotFn)(void *, int);
typedef int (*GetMaxStackFn)(void *, int, const void *, const void *);
typedef int (*ItemIsNullFn)(const void *);
typedef void *(*ItemCopyCtorFn)(void *, const void *);
typedef void *(*ItemCtorFn)(void *);
typedef unsigned char (*ItemGetMaxStackSizeFn)(const void *);
typedef int (*CanStackFn)(void *, const void *, const void *);

static void *on_mob_get_armor_slot(void *mob, int slot) {
    if (slot == 4) {
        /* Slot 4 is the Offhand equipment slot (mob + 0x0F08) */
        return (void *)((char *)mob + 0x0F08);
    }
    return ((GetArmorSlotFn)s->mob_get_armor_slot.trampoline)(mob, slot);
}

static void on_container_inventory_screen_set_item_slot(void *screen, int button_id, const void *item) {
    if (button_id == 104) {
        /* Button 104 is the 5th equipment slot (Offhand) */
        void *player = *(void **)((char *)screen + 0xA8);
        if (player) {
            void *ui_ctrl = *(void **)((char *)screen + 0x04);
            if (ui_ctrl) {
                void *state = *(void **)((char *)ui_ctrl + 0xD4);
                if (state && *(int *)((char *)state + 0x10C) == 104) {
                    ((ClearSelectionFn)SEAM_ClearSelection)(state, -1);
                }
            }
            ((SetOffhandSlotFn)SEAM_LocalPlayer_setOffhandSlot)(player, item);
        }
        return;
    }
    ((SetItemSlotFn)s->container_inventory_screen_set_item_slot.trampoline)(screen, button_id, item);
}

static void on_container_inventory_screen_get_item(void *ret, void *screen, int button_id) {
    if (button_id == 104) {
        void *player = *(void **)((char *)screen + 0xA8);
        if (player) {
            void *offhand_item = (char *)player + 0x0F08;
            if (offhand_item && !((ItemIsNullFn)SEAM_ItemInstance_isNull)(offhand_item)) {
                ((ItemCopyCtorFn)SEAM_ItemInstance_copyCtor)(ret, offhand_item);
                return;
            }
        }
        ((ItemCtorFn)SEAM_ItemInstance_ctor)(ret);
        return;
    }
    ((GetItemFn)s_container_inventory_screen_get_item.trampoline)(ret, screen, button_id);
}

static int on_container_inventory_screen_is_valid_slot(void *screen, int button_id) {
    if (button_id == 104) {
        return 1;
    }
    return ((IsValidSlotFn)s_container_inventory_screen_is_valid_slot.trampoline)(screen, button_id);
}

static int on_container_inventory_screen_get_max_stack_size_for_slot(void *screen, int button_id, const void *src_item, const void *dest_item) {
    if (button_id == 104) {
        void *item;
        if (!src_item || ((ItemIsNullFn)SEAM_ItemInstance_isNull)(src_item)) {
            return 0;
        }
        item = *(void **)((const char *)src_item + 12);
        if (!item || !*(const unsigned char *)((const char *)item + 0x2A)) {
            return 0;
        }
        if (dest_item && !((ItemIsNullFn)SEAM_ItemInstance_isNull)(dest_item)) {
            return 0;
        }
        return 1;
    }
    return ((GetMaxStackFn)s_container_inventory_screen_get_max_stack_size_for_slot.trampoline)(screen, button_id, src_item, dest_item);
}

static void on_container_inventory_screen_setup_navigation(void *screen) {
    ((SetupNavFn)s->container_inventory_screen_setup_navigation.trampoline)(screen);

    void *nav_map = (char *)screen + 0xD8;
    GetNavNodeFn get_node = (GetNavNodeFn)SEAM_BaseSurvivalScreen_container_findOrInsert;

    int k100 = 100, k103 = 103, k104 = 104;
    int k15 = 15, k24 = 24;

    void *n100 = get_node(nav_map, &k100);
    void *n103 = get_node(nav_map, &k103);
    void *n104 = get_node(nav_map, &k104);
    void *n15  = get_node(nav_map, &k15);
    void *n24  = get_node(nav_map, &k24);

    if (n104 && n103 && n100 && n15 && n24) {
        /*
         * NavigationNode direction pointer offsets:
         *   +0x08: Up
         *   +0x0C: Down
         *   +0x10: Left
         *   +0x14: Right
         */
        *(void **)((char *)n104 + 0x08) = n15;
        *(void **)((char *)n104 + 0x0C) = n24;
        *(void **)((char *)n104 + 0x10) = n103;
        *(void **)((char *)n104 + 0x14) = n100;

        /* Boots (103) right goes to offhand (104) */
        *(void **)((char *)n103 + 0x14) = n104;

        /* Helmet (100) left wraps to offhand (104) */
        *(void **)((char *)n100 + 0x10) = n104;

        /* Inventory column 6 top (slot 15) down goes to offhand (104) */
        *(void **)((char *)n15 + 0x0C) = n104;

        /* Inventory column 6 bottom (slot 24) up goes to offhand (104) */
        *(void **)((char *)n24 + 0x08) = n104;
    }
}

int offhand_inventory_ui_install_hooks(void) {
    NuMC3DS_Hook *hook = &s->mob_get_armor_slot;
    hook->target = SEAM_Mob_getArmorSlot;
    hook->replacement = (u32)on_mob_get_armor_slot;
    hook->expected[0] = 0xE0811081u; /* add r1, r1, r1, lsl #1 */
    hook->expected[1] = 0xE0800201u; /* add r0, r0, r1, lsl #4 */
    if (s->host.install_hook(hook)) {
        s->host.debug_string(offhand_armor_hook_fail, sizeof(offhand_armor_hook_fail) - 1);
        return -35;
    }

    hook = &s->container_inventory_screen_set_item_slot;
    hook->target = SEAM_ContainerInventoryScreen_setItemSlot;
    hook->replacement = (u32)on_container_inventory_screen_set_item_slot;
    hook->expected[0] = 0xE92D4070u; /* push {r4, r5, r6, lr} */
    hook->expected[1] = 0xE1A05000u; /* mov r5, r0 */
    if (s->host.install_hook(hook)) {
        s->host.debug_string(offhand_set_slot_hook_fail, sizeof(offhand_set_slot_hook_fail) - 1);
        return -36;
    }

    hook = &s->container_inventory_screen_setup_navigation;
    hook->target = SEAM_ContainerInventoryScreen_setupNavigation;
    hook->replacement = (u32)on_container_inventory_screen_setup_navigation;
    hook->expected[0] = 0xE92D4FF0u; /* push {r4-r11, lr} */
    hook->expected[1] = 0xE24DD02Cu; /* sub sp, sp, #0x2C */
    if (s->host.install_hook(hook)) {
        s->host.debug_string(offhand_nav_hook_fail, sizeof(offhand_nav_hook_fail) - 1);
        return -37;
    }

    hook = &s_container_inventory_screen_get_item;
    hook->target = SEAM_ContainerInventoryScreen_getItem;
    hook->replacement = (u32)on_container_inventory_screen_get_item;
    hook->expected[0] = 0xE92D4010u;
    hook->expected[1] = 0xE1A04000u;
    if (s->host.install_hook(hook)) {
        s->host.debug_string(offhand_get_item_hook_fail, sizeof(offhand_get_item_hook_fail) - 1);
        return -38;
    }

    hook = &s_container_inventory_screen_is_valid_slot;
    hook->target = SEAM_ContainerInventoryScreen_isValidSlot;
    hook->replacement = (u32)on_container_inventory_screen_is_valid_slot;
    hook->expected[0] = 0xE92D4010u;
    hook->expected[1] = 0xE1A04001u;
    if (s->host.install_hook(hook)) {
        s->host.debug_string(offhand_valid_slot_hook_fail, sizeof(offhand_valid_slot_hook_fail) - 1);
        return -39;
    }

    hook = &s_container_inventory_screen_get_max_stack_size_for_slot;
    hook->target = SEAM_ContainerInventoryScreen_getMaxStackSizeForSlot;
    hook->replacement = (u32)on_container_inventory_screen_get_max_stack_size_for_slot;
    hook->expected[0] = 0xE92D41F0u;
    hook->expected[1] = 0xE1A05000u;
    if (s->host.install_hook(hook)) {
        s->host.debug_string(offhand_max_stack_hook_fail, sizeof(offhand_max_stack_hook_fail) - 1);
        return -40;
    }

    return 0;
}
