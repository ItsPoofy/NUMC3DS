#ifndef NUMC3DS_UI_SCREEN_VTABLES_H
#define NUMC3DS_UI_SCREEN_VTABLES_H

#include "ui_widgets.h"

typedef struct {
    void (*setup)(void *screen);
    void (*render)(void *screen, int touch_x, int touch_y, int mask, float tick);
    void (*press)(void *screen, void *button);
    int (*on_back)(void *screen, int reason);
    void (*closed)(void *screen);
    void (*mapped)(void *screen, int button);
    void (*move)(void *screen, int source, int direction);
    void (*next)(void *screen);
    void (*previous)(void *screen);
    void (*owner_press)(void *owner, void *button);
} UiCustomScreenHooks;

void *ui_screen_vtables_create(void *screen, void *owner_press);
void ui_screen_vtables_apply(void *screen, void *vtables);

int ui_custom_screen_push(void *game, void *client, void **vtable_cache,
                          const UiCustomScreenHooks *hooks, UiShared *held_screen);
void ui_screen_close(void *screen);

#endif
