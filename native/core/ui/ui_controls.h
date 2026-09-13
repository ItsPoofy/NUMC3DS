#ifndef NUMC3DS_UI_CONTROLS_H
#define NUMC3DS_UI_CONTROLS_H

#include "ui_widgets.h"

#define UI_SCREEN_TOP_WIDTH         400
#define UI_SCREEN_BOTTOM_WIDTH      320
#define UI_SCREEN_HEIGHT            240

#define UI_SCREEN_TOP               0x40
#define UI_SCREEN_BOTTOM            0x80
#define UI_SCREEN_GRID_ITEM         0x40
#define UI_SCREEN_BOTH              (UI_SCREEN_TOP | UI_SCREEN_BOTTOM)
#define UI_SCREEN_DEFAULT           UI_SCREEN_BOTTOM

#define UI_COORD_CENTER_X(width, screen_width) (((screen_width) - (width)) / 2)
#define UI_CENTER_X_BOTTOM(width)   UI_COORD_CENTER_X(width, UI_SCREEN_BOTTOM_WIDTH)
#define UI_CENTER_X_TOP(width)      UI_COORD_CENTER_X(width, UI_SCREEN_TOP_WIDTH)
#define UI_CENTER_X(width)          UI_CENTER_X_BOTTOM(width)
#define UI_ALIGN_RIGHT(width, screen_width, pad) ((screen_width) - (width) - (pad))
#define UI_ALIGN_RIGHT_BOTTOM(width, pad) UI_ALIGN_RIGHT(width, UI_SCREEN_BOTTOM_WIDTH, pad)

enum { UI_CONTROL_CAPACITY=48 };

typedef enum {
    UI_CTRL_BUTTON,
    UI_CTRL_GRID_BUTTON,
    UI_CTRL_SWITCH,
    UI_CTRL_LABEL,
    UI_CTRL_SLIDER,
    UI_CTRL_DROPDOWN,
    UI_CTRL_TAB
} UiControlKind;

typedef void (*UiControlCallback)(void *screen, void *control_object);

typedef struct {
    int id;
    UiControlKind kind;
    const char *label_key;
    const char *label_text;
    int x, y, width, height;
    int visible;
    int screen_mask;
    int initial_value;
    UiControlCallback on_press;
    const void *native_seam;
} UiControlSpec;

typedef struct {
    const UiControlSpec *spec;
    void *object;
    UiShared shared;
    u8 adopted;
} UiControlInstance;

typedef struct {
    const UiControlSpec *specs;
    unsigned spec_count;
    UiControlInstance instances[UI_CONTROL_CAPACITY];
    unsigned instance_count;
    void *screen;
    void *context;
} UiControlSet;

int ui_controls_build(void *screen, UiControlSet *set);
void ui_controls_destroy(UiControlSet *set);
int ui_controls_dispatch(UiControlSet *set, void *button_or_event);
void *ui_controls_find(const UiControlSet *set, int id);
const UiControlSpec *ui_controls_spec(const UiControlSet *set, int id);
const UiShared *ui_controls_shared(const UiControlSet *set, int id);
void ui_controls_release_shared(UiControlSet *set, int id);
void ui_controls_adopt(UiControlSet *set, int id);
void ui_controls_set_context(UiControlSet *set, void *context);

#endif
