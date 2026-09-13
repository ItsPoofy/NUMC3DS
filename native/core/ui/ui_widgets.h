#ifndef NUMC3DS_UI_WIDGETS_H
#define NUMC3DS_UI_WIDGETS_H

#include "ui_runtime.h"

void *ui_create_button(void *screen,int id,int x,int y,int width,int height,const char *label,int visible,int screen_mask);
void *ui_create_grid_button(void *screen,int id,int x,int y,int width,int height,const char *label,int visible,int screen_mask);
void *ui_create_hud_button(void *screen,int id,int x,int y,int width,int height,int visible,int chat_icon);
void ui_button_set_label(void *button,const char *label);
void ui_button_set_latched(void *button,int latched);
void ui_button_set_manual_render(void *button,int manual);
void ui_play_button_sound(void *screen);
void ui_button_draw_native(void *screen,void *button,int touch_x,int touch_y);
int ui_button_label_width(void *screen,void *button);
void ui_button_sync_label_width(void *screen,void *button);
int ui_button_label_y(void *screen,void *button);
void ui_mesh_cache_reset(UiMeshCache *cache);
void *ui_buttons_draw_native_cached(UiMeshCache *cache,void **buttons,unsigned count,void *pressed);
void ui_button_draw_background_cached(UiMeshCache *cache,void *button,int pressed);
void ui_button_draw_highlight(void *screen,void *button);
const char *ui_button_label(void *button);
void ui_fill(void *screen,int x1,int y1,int x2,int y2,const Color *color);
void ui_cached_text_set(void *screen,UiCachedText *cache,const char *text);
void ui_cached_text_draw(void *screen,UiCachedText *cache,float x,float y,float scale,const Color *color);
void ui_cached_text_reset(UiCachedText *cache);
int ui_text_width(void *screen,const char *text);
void *ui_create_world_switch(void *screen,int enabled,int id);
int ui_create_world_switch_shared(void *screen,int enabled,int id,UiShared *shared);
int ui_shared_from_object(void *object,UiShared *shared);
void *ui_create_option_label(void *screen,const char *text);
void ui_warning_set_message(void *warning,const char *message);
int ui_install_button_color_hook(void);

#endif
