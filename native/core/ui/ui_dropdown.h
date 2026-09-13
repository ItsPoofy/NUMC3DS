#ifndef NUMC3DS_UI_DROPDOWN_H
#define NUMC3DS_UI_DROPDOWN_H
#include "ui_widgets.h"
typedef struct { void **vtable; } UiDropdownListener;
typedef struct { const char *const *keys; unsigned count; UiDropdownListener *listener; } UiDropdownOptions;
int ui_dropdown_create(void *screen,int id,int x,int y,int width,int height,const UiDropdownOptions *options,UiShared *out);
int ui_dropdown_value(void *control);
void ui_dropdown_set_value(void *control,int value);
int ui_dropdown_opened(void *control);
void ui_dropdown_input(void *screen,void *control,int binding);
void ui_dropdown_draw_popup(void *screen,void *control,int x,int y);
#endif
