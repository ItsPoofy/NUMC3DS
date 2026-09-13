#ifndef NUMC3DS_XP_PROGRESS_BAR_H
#define NUMC3DS_XP_PROGRESS_BAR_H

#include "ui_runtime.h"

int xp_progress_bar_build(void *screen,UiXpProgressBar *bar);
void xp_progress_bar_draw(void *screen,UiXpProgressBar *bar,float percent,int touch_x,int touch_y);
void xp_progress_bar_reset(UiXpProgressBar *bar);

#endif
