#ifndef NUMC3DS_HAND_VISIBILITY_H
#define NUMC3DS_HAND_VISIBILITY_H

#include "../rt.h"

int hand_visibility_install_hook(void);
int hand_visibility_hidden(void);
void hand_visibility_set_hidden(int hidden);
int top_hud_visibility_hidden(void);
void top_hud_visibility_set_hidden(int hidden);

#endif
