#ifndef NUMC3DS_WORLD_SETTINGS_UI_H
#define NUMC3DS_WORLD_SETTINGS_UI_H

#include "../rt.h"

int world_settings_ui_install_hooks(void);
void world_settings_register_gamerule_control(void *control);
void relayout_world_options(void *screen, void *grid);

#endif
