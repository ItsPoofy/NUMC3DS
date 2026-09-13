#ifndef NUMC3DS_PROGRESS_TIPS_H
#define NUMC3DS_PROGRESS_TIPS_H

#include "ui_runtime.h"

void progress_tips_begin(void *screen,UiProgressTips *tips);
void progress_tips_draw(void *screen,UiProgressTips *tips);
void progress_tips_reset(UiProgressTips *tips);

#endif
