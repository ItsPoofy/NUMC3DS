#ifndef NUMC3DS_KEYBOARD_LABEL_MESH_H
#define NUMC3DS_KEYBOARD_LABEL_MESH_H
#include "../rt.h"
void keyboard_label_mesh_reset(void);
void keyboard_label_mesh_build(void *screen,void **buttons,unsigned count);
void keyboard_label_mesh_draw(const Color *color);
#endif
