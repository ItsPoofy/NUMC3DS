#ifndef NUMC3DS_UI_FORM_STYLE_H
#define NUMC3DS_UI_FORM_STYLE_H
#include "ui_widgets.h"
extern const Color ui_form_background,ui_form_white;
void ui_form_field(void *screen,int x,int y,int width,int height,int editable);
typedef struct { void *layer; } UiFormHeader;
void ui_form_header(void *screen,int width,UiCachedText *title,UiFormHeader *header);
void ui_form_header_reset(UiFormHeader *header);
#endif
