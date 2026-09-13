#include "../include/numc3ds_abi.h"
#include "internal.h"
#include "state.h"
#include "hook_manager.h"
#include "resource_lifecycle.h"
#include "ui/keyboard_input.h"
#include "ui/chat_ui.h"
#include "ui/command_block_screen.h"
#include "ui/ui_text_style.h"
#include "ui/world_settings_ui.h"
#include "ui/achievement_banner.h"
#include "ui/options_ui.h"
#include "ui/hand_visibility.h"
#include "ui/progress_screen_ui.h"
#include "ui/map_ui.h"
#include "ui/minimap_fix.h"
#include "ui/offhand_inventory_ui.h"
#include "item/map_item.h"
#include "item/creative_item_service.h"
#include "entity/boat_control_fix.h"
#include "entity/fishing_hook_collision_fix.h"
#include "entity/remote_player.h"
#include "entity/minecart_command_block.h"
#include "world_transfer/world_transfer_service.h"
#include "world/extended_chunk_storage.h"
#include "render/gui_item_render_guard.h"
#include "opt/opt.h"
#include "skin/custom_skin_service.h"
#include "network/network_module.h"
#include "extension.h"

static State module_state;
State *s = (State*)0xFFFFFFFFu;

static const char ready[] = "NuMC3DS touch chat ready\n";
static const char bad[] = "NuMC3DS chat: incompatible host ABI\n";

static void rollback_installed_hooks(State *state) {
    if (!state || !state->host.remove_hook) return;
    world_transfer_remove_hooks();
    hook_manager_rollback_all();
}

__attribute__((section(".entry"))) numc3ds_s32 numc3ds_entry(const NuMC3DS_HostAbi *h) {
    State *n;
    int result;
    if (!h || h->magic != NUMC3DS_HOST_MAGIC || h->abi_version != NUMC3DS_HOST_ABI_VERSION ||
        h->struct_size < sizeof(*h) || !h->heap_alloc || !h->heap_free || !h->debug_string) {
        if (h && h->debug_string) h->debug_string(bad, sizeof(bad) - 1);
        return -1;
    }
    if (s != (State*)0xFFFFFFFFu) return (s && s->magic == MAGIC) ? 0 : -2;
    n = &module_state;
    zero(n, sizeof(*n));
    n->magic = MAGIC;
    n->keyboard.text = 0;
    n->keyboard.input_limit = MAX_TEXT;
    n->size = sizeof(*n);
    cp(&n->host, h, sizeof(*h));
    n->host.install_hook = hook_manager_install;
    n->host.remove_hook = hook_manager_remove;
    s = n;
    hook_manager_reset();

    result = keyboard_input_install_hook();
    if (result) goto failed;
    result = chat_ui_install_hooks();
    if (result) goto failed;
    result = numc3ds_extension_install_early_hooks();
    if (result) goto failed;
    result = command_block_screen_install();
    if (result) goto failed;
    result = ui_text_style_install_hooks();
    if (result) goto failed;
    result = world_transfer_install_hook();
    if (result) goto failed;
    result = world_settings_ui_install_hooks();
    if (result) goto failed;
    result = achievement_banner_install_hooks();
    if (result) goto failed;
    result = options_ui_install_hooks();
    if (result) goto failed;
    result = hand_visibility_install_hook();
    if (result) goto failed;
    result = progress_screen_ui_install_hooks();
    if (result) goto failed;
    result = native_command_system_install_hook();
    if (result) goto failed;
    result = resource_lifecycle_install_hook();
    if (result) goto failed;
    result = map_ui_install_hooks();
    if (result) goto failed;
    result = minimap_fix_install_hooks();
    if (result) goto failed;
    result = map_item_install_hooks();
    if (result) goto failed;
    result = creative_item_service_install_hook();
    if (result) goto failed;
    result = minecart_command_block_install_hooks();
    if (result) goto failed;
    result = boat_control_fix_install_hook();
    if (result) goto failed;
    result = fishing_hook_collision_fix_install_hook();
    if (result) goto failed;
    result = remote_player_install_hooks();
    if (result) goto failed;
    result = numc3ds_extension_install_hooks();
    if (result) goto failed;
    result = offhand_inventory_ui_install_hooks();
    if (result) goto failed;
    result = opt_install_hooks();
    if (result) goto failed;
    result = extended_chunk_storage_install_hooks();
    if (result) goto failed;
    result = gui_item_render_guard_install_hook();
    if (result) goto failed;
    result = custom_skin_service_install_hooks();
    if (result) goto failed;
    result = network_module_install();
    if (result) goto failed;

    h->debug_string(ready, sizeof(ready) - 1);
    return 0;

failed:
    rollback_installed_hooks(n);
    s = 0;
    return result;
}
