#include "gui_item_render_guard.h"
#include "../hook_manager.h"
#include "../state.h"

static NuMC3DS_Hook render_gui_item_hook;

static void on_render_gui_item(void *this_ptr, void *font, const void *item,
                               float x, float y, float alpha, float scale, int param7) {
    if (!item) return;
    /* ItemInstance: mCount is at offset 0, mItem pointer is at offset 12 (0xC) */
    if (*(const signed char *)item <= 0) return;
    if (*(void *const *)((const char *)item + 12) == 0) return;

    typedef void (*fn_t)(void *, void *, const void *, float, float, float, float, int);
    ((fn_t)render_gui_item_hook.trampoline)(this_ptr, font, item, x, y, alpha, scale, param7);
}

int gui_item_render_guard_install_hook(void) {
    render_gui_item_hook.target = 0x006B734Cu;
    render_gui_item_hook.replacement = (u32)on_render_gui_item;
    render_gui_item_hook.expected[0] = 0xE92D41F0u;
    render_gui_item_hook.expected[1] = 0xE1A05003u;
    return s->host.install_hook(&render_gui_item_hook);
}
