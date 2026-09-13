#ifndef NUMC3DS_UI_TYPES_H
#define NUMC3DS_UI_TYPES_H

#include "../rt.h"

typedef struct { void *object, *control; } UiShared;
typedef struct { UiShared *begin, *end, *capacity; } UiSharedVector;
typedef struct { int *begin, *end, *capacity; } UiIntVector;

typedef struct {
    void *vtable;
    u32 reserved04;
    u8 active;
    u8 visible;
    u8 reserved0a[2];
    int x, y, width, height;
    u8 reserved1c[0x18];
    int screen_mask;
} UiElementView;

typedef struct {
    UiElementView element;
    u8 background_mode;
    u8 reserved39[3];
    Color normal_label;
    Color pressed_label;
    Color inactive_label;
    u8 reserved6c[4];
    u32 label;
    void *game;
    int id;
    u8 visual_press_on_drag;
    u8 dragging;
    u8 hidden_from_screen_pass;
    u8 flag7f;
    u32 drop_shadow;
    void *owner;
    int cached_label_width;
    u8 reserved8c[0x0C];
    void *normal_background;
    void *pressed_background;
    void *inactive_background;
} UiButtonView;

typedef struct {
    float x1, x2, y1, y2, z, u1, u2, v1, v2;
} UiNinePatchQuad;

typedef struct {
    u8 reserved00[0x38];
    float width, height;
    u8 reserved40[0x38];
    u8 texture[0x20];
    u32 hidden_quads;
    UiNinePatchQuad quads[9];
} UiNinePatchLayerView;

typedef struct {
    u8 reserved00[0xF8];
    u8 enabled;
} UiSwitchButtonView;

typedef struct {
    u8 reserved00[0x50];
    float position;
    u8 reserved54[0x0C];
    float minimum, maximum;
} UiSliderView;

typedef struct {
    u8 reserved00[4];
    void *game;
    u8 reserved08[0x10];
    void *client;
    UiSharedVector buttons;
    UiSharedVector elements;
    UiSharedVector tabs;
    UiSharedVector components;
    UiSharedVector selectable_components;
    int selected_tab;
    int selectable_selection;
    u32 reserved60;
    void *font;
    UiShared dragging;
} UiScreenView;

typedef struct {
    u8 reserved00[0xB8];
    u32 hud_delay_timer;
    u8 reservedBC[0x10];
    void *pause_button;
} UiInGamePlayScreenView;

typedef struct {
    u8 reserved00[0x38];
    UiSharedVector children;
} UiContainerView;

typedef struct {
    u8 reserved00[0x38];
    UiSharedVector children;
} UiOptionItemChildrenView;

typedef struct {
    u8 reserved00[0x38];
    UiSharedVector children;
} UiOptionItemView;

typedef struct {
    u8 reserved00[0x6C];
    u32 localized_label;
} UiOptionsRowView;

typedef struct {
    u8 reserved00[0x94];
    UiShared option_grid;
    UiIntVector row_widths;
    u8 reservedA8[4];
    int max_selection;
    int grid_item_count;
    u8 reservedB4[0x60];
    int game_mode;
    u8 reserved118[0xAC];
    u8 restricted_setting;
    u8 reserved1C5[0x1F];
    UiShared always_day;
    u8 reserved1EC[0x0C];
    UiShared world_type;
} UiWorldSettingsScreenView;

typedef struct {
    u8 reserved00[0xA0];
    void *progress_handler;
    u8 reservedA4[0x2C];
    void *top_progress_widget;
    void *bottom_progress_widget;
    u32 localized_label;
    int top_label_y;
    int bottom_label_y;
    int top_label_x;
    int bottom_label_x;
    int countdown;
    float timer;
} UiProgressScreenView;

typedef struct {
    UiElementView element;
    u8 reserved38[0xB4];
} UiScrollBarView;

typedef struct {
    UiElementView element;
    u8 reserved38[0x3C];
    int content_height;
    u8 reserved78[0x98];
    float content_offset;
    u8 reserved114[0xD8];
    UiScrollBarView scroll_bar;
} UiScrollingPaneView;

#define UI_ASSERT(name, expression) typedef char name[(expression) ? 1 : -1]
UI_ASSERT(ui_element_active_offset, __builtin_offsetof(UiElementView, active) == 0x08);
UI_ASSERT(ui_element_mask_offset, __builtin_offsetof(UiElementView, screen_mask) == 0x34);
UI_ASSERT(ui_button_label_offset, __builtin_offsetof(UiButtonView, label) == 0x70);
UI_ASSERT(ui_button_id_offset, __builtin_offsetof(UiButtonView, id) == 0x78);
UI_ASSERT(ui_button_owner_offset, __builtin_offsetof(UiButtonView, owner) == 0x84);
UI_ASSERT(ui_button_width_offset, __builtin_offsetof(UiButtonView, cached_label_width) == 0x88);
UI_ASSERT(ui_button_drop_shadow_offset, __builtin_offsetof(UiButtonView, drop_shadow) == 0x80);
UI_ASSERT(ui_button_normal_label_offset, __builtin_offsetof(UiButtonView, normal_label) == 0x3C);
UI_ASSERT(ui_button_pressed_label_offset, __builtin_offsetof(UiButtonView, pressed_label) == 0x4C);
UI_ASSERT(ui_button_inactive_label_offset, __builtin_offsetof(UiButtonView, inactive_label) == 0x5C);
UI_ASSERT(ui_button_normal_background_offset, __builtin_offsetof(UiButtonView, normal_background) == 0x98);
UI_ASSERT(ui_nine_patch_texture_offset, __builtin_offsetof(UiNinePatchLayerView, texture) == 0x78);
UI_ASSERT(ui_nine_patch_mask_offset, __builtin_offsetof(UiNinePatchLayerView, hidden_quads) == 0x98);
UI_ASSERT(ui_nine_patch_quads_offset, __builtin_offsetof(UiNinePatchLayerView, quads) == 0x9C);
UI_ASSERT(ui_slider_position_offset, __builtin_offsetof(UiSliderView, position) == 0x50);
UI_ASSERT(ui_slider_minimum_offset, __builtin_offsetof(UiSliderView, minimum) == 0x60);
UI_ASSERT(ui_screen_buttons_offset, __builtin_offsetof(UiScreenView, buttons) == 0x1C);
UI_ASSERT(ui_screen_tabs_offset, __builtin_offsetof(UiScreenView, tabs) == 0x34);
UI_ASSERT(ui_screen_client_offset, __builtin_offsetof(UiScreenView, client) == 0x18);
UI_ASSERT(ui_screen_components_offset, __builtin_offsetof(UiScreenView, components) == 0x40);
UI_ASSERT(ui_screen_selectables_offset, __builtin_offsetof(UiScreenView, selectable_components) == 0x4C);
UI_ASSERT(ui_screen_selected_tab_offset, __builtin_offsetof(UiScreenView, selected_tab) == 0x58);
UI_ASSERT(ui_screen_selectable_selection_offset, __builtin_offsetof(UiScreenView, selectable_selection) == 0x5C);
UI_ASSERT(ui_screen_font_offset, __builtin_offsetof(UiScreenView, font) == 0x64);
UI_ASSERT(ui_ingame_hud_delay_offset, __builtin_offsetof(UiInGamePlayScreenView, hud_delay_timer) == 0xB8);
UI_ASSERT(ui_ingame_pause_offset, __builtin_offsetof(UiInGamePlayScreenView, pause_button) == 0xCC);
UI_ASSERT(ui_container_children_offset, __builtin_offsetof(UiContainerView, children) == 0x38);
UI_ASSERT(ui_option_item_children_offset, __builtin_offsetof(UiOptionItemChildrenView, children) == 0x38);
UI_ASSERT(ui_options_row_label_offset, __builtin_offsetof(UiOptionsRowView, localized_label) == 0x6C);
UI_ASSERT(ui_world_grid_offset, __builtin_offsetof(UiWorldSettingsScreenView, option_grid) == 0x94);
UI_ASSERT(ui_world_rows_offset, __builtin_offsetof(UiWorldSettingsScreenView, row_widths) == 0x9C);
UI_ASSERT(ui_world_mode_offset, __builtin_offsetof(UiWorldSettingsScreenView, game_mode) == 0x114);
UI_ASSERT(ui_world_always_day_offset, __builtin_offsetof(UiWorldSettingsScreenView, always_day) == 0x1E4);
UI_ASSERT(ui_world_type_offset, __builtin_offsetof(UiWorldSettingsScreenView, world_type) == 0x1F8);
UI_ASSERT(ui_progress_handler_offset, __builtin_offsetof(UiProgressScreenView, progress_handler) == 0xA0);
UI_ASSERT(ui_progress_top_widget_offset, __builtin_offsetof(UiProgressScreenView, top_progress_widget) == 0xD0);
UI_ASSERT(ui_progress_label_offset, __builtin_offsetof(UiProgressScreenView, localized_label) == 0xD8);
UI_ASSERT(ui_progress_countdown_offset, __builtin_offsetof(UiProgressScreenView, countdown) == 0xEC);
UI_ASSERT(ui_progress_timer_offset, __builtin_offsetof(UiProgressScreenView, timer) == 0xF0);
UI_ASSERT(ui_pane_content_height_offset, __builtin_offsetof(UiScrollingPaneView, content_height) == 0x74);
UI_ASSERT(ui_pane_content_offset_offset, __builtin_offsetof(UiScrollingPaneView, content_offset) == 0x110);
UI_ASSERT(ui_pane_scroll_bar_offset, __builtin_offsetof(UiScrollingPaneView, scroll_bar) == 0x1EC);
UI_ASSERT(ui_pane_size, sizeof(UiScrollingPaneView) == 0x2D8);
UI_ASSERT(ui_scroll_bar_size, sizeof(UiScrollBarView) == 0xEC);

#endif
