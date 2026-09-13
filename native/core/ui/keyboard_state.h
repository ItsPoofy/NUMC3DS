#ifndef NUMC3DS_KEYBOARD_STATE_H
#define NUMC3DS_KEYBOARD_STATE_H
#include "../rt.h"
typedef struct {
    void *keyboard_screen, *keyboard_control, *keyboard_vtable, *buttons[KEYS+CONTROLS], *keyboard_symbols[KEYBOARD_SYMBOLS];
    numc3ds_u32 count, active, keyboard_closing, caps, page, length, cursor, blink;
    s32 keyboard_binding_up, keyboard_binding_down, keyboard_binding_left, keyboard_binding_right;
    s32 keyboard_binding_ok, keyboard_binding_select, keyboard_binding_cancel, keyboard_binding_clear, keyboard_binding_mode, keyboard_binding_done;
    s32 keyboard_binding_snap_left, keyboard_binding_snap_right, keyboard_binding_cursor_left, keyboard_binding_cursor_right;
    void *keyboard_controller;
    u32 keyboard_repeat_action, keyboard_repeat_ticks;
    char *text;
    unsigned input_limit;
    UiCachedText keyboard_runs[KEYBOARD_LABEL_RUNS];
    UiMeshCache keyboard_background, keyboard_focus_background;
    void *keyboard_focus_button;
    int keyboard_run_x[KEYBOARD_LABEL_RUNS], keyboard_run_y[KEYBOARD_LABEL_RUNS];
    numc3ds_u32 keyboard_run_count;
    void *keyboard_cache_font;
} UiKeyboardState;
#endif
