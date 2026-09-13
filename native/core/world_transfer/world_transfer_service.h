#ifndef NUMC3DS_WORLD_TRANSFER_SERVICE_H
#define NUMC3DS_WORLD_TRANSFER_SERVICE_H

#include "world_transfer_journal.h"

enum {
    WORLD_TRANSFER_EXPORT_ID = 0x3F4,
    WORLD_TRANSFER_CANCEL_ID = 0x3F5
};

int world_transfer_install_hook(void);
void world_transfer_remove_hooks(void);
void world_transfer_edit_setup(void *screen);
int world_transfer_edit_pressed(void *screen,void *event);
void world_transfer_tick(void *screen);
int world_transfer_on_progress_destroy(void *screen);
void world_transfer_reopen_after_progress(void);
void world_transfer_cancel(void);
int world_transfer_is_active(void);
const char *world_transfer_progress_message(void);
float world_transfer_progress_percent(void);
void world_transfer_summary_invalidate(void);

#endif
