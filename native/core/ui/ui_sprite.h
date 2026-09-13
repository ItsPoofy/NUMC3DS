#ifndef NUMC3DS_UI_SPRITE_H
#define NUMC3DS_UI_SPRITE_H

#include "ui_runtime.h"

void *ui_sprite_create(void *screen,const char *resource,int x,int y,int width,int height,int source_x,int source_y,int source_width,int source_height);
void ui_sprite_draw(void *sprite);
void ui_sprite_destroy(void *sprite);

#endif
