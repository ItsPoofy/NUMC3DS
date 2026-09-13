#ifndef NUMC3DS_ACHIEVEMENT_BANNER_H
#define NUMC3DS_ACHIEVEMENT_BANNER_H

#include "../rt.h"
#include "ui_types.h"

enum {
    ACHIEVEMENT_SCREEN_CREATE = 1,
    ACHIEVEMENT_SCREEN_EDIT   = 2
};

int  achievement_banner_install_hooks(void);
void achievement_banner_setup_cache(void *screen);
void achievement_banner_invalidate_cache(void);
void achievement_banner_set_eligible(int eligible);
int  ui_achievement_banner_create_shared(void *screen, UiShared *out_shared);

#endif
