#ifndef NUMC3DS_UI_RUNTIME_H
#define NUMC3DS_UI_RUNTIME_H

#include "ui_types.h"

void *ui_screen_game(void *screen);
void *ui_screen_client(void *screen);
void *ui_screen_font(void *screen);
void *ui_game_small_font(void *screen);
void *ui_screen_button_owner(void *screen);
void *ui_ingame_pause_button(void *screen);
int ui_ingame_hud_ready(void *screen);
int ui_resources_ready(void);
void ui_element_set_visible(void *element, int visible);
void ui_element_set_screen(void *element, int screen_mask);
void ui_element_capture(void *element, u8 *active, u8 *visible);
void ui_element_restore(void *element, u8 active, u8 visible);
void ui_element_copy_horizontal_bounds(void *target, const void *source);
void *ui_option_item_first_child(void *item);
const char *ui_option_item_label(void *item);
void ui_option_item_set_label(void *item, const char *text);
float ui_slider_value(const void *slider);
int ui_button_id(void *button);
void ui_screen_add_button(void *screen, UiShared *button);
void ui_screen_add_tab(void *screen, UiShared *tab_button);
int ui_screen_selected_tab(void *screen);
void ui_screen_set_tab(void *screen, int tab_index);
int ui_screen_cycle_tab(void *screen, int delta);
void ui_screen_update_tab_buttons(void *screen);
unsigned ui_screen_tab_count(void *screen);
void *ui_screen_tab_at(void *screen, unsigned index);
void ui_screen_add_selectable(void *screen, void *object);
void ui_screen_set_selection(void *screen, unsigned index);
void *ui_screen_selected(void *screen);
enum { UI_NAV_UP=1, UI_NAV_DOWN=2, UI_NAV_LEFT=3, UI_NAV_RIGHT=4 };
int ui_screen_navigate(void *screen,int direction);
void ui_container_add_raw(void *container, void *child, int selectable);
void ui_container_add_shared(void *container, UiShared *child, int selectable);
unsigned ui_container_child_count(void *container);
void ui_int_vector_push(void *vector, int value);
int ui_grid_move_appended_row_before(void *container, UiIntVector *rows, void *first, void *second, void *target);
void ui_shared_release(UiShared *shared);
void ui_capture_vector(UiSharedVector *vector, void **objects, u8 *active, u8 *visible, u32 *count, u32 capacity);

#endif
