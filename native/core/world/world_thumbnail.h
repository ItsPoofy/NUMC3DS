#ifndef NUMC3DS_WORLD_THUMBNAIL_H
#define NUMC3DS_WORLD_THUMBNAIL_H

#include "../rt.h"

#define WT_THUMBNAIL_TEX_W      256
#define WT_THUMBNAIL_TEX_H      128
#define WT_THUMBNAIL_PIC_W      160
#define WT_THUMBNAIL_PIC_H      92
#define WT_THUMBNAIL_FILE_SIZE  (32 + (WT_THUMBNAIL_TEX_W * WT_THUMBNAIL_TEX_H * 3))
#define WT_THUMBNAIL_ROW_SIZE   (WT_THUMBNAIL_TEX_W * 8 * 3)
#define WT_THUMBNAIL_CACHE_SIZE (WT_THUMBNAIL_PIC_W * WT_THUMBNAIL_PIC_H * 3)

int wt_thumbnail_save_pending(const char *world_id);
void wt_thumbnail_on_save_marker(void *storage);
int wt_thumbnail_has_cache(void);
int wt_thumbnail_capture_active(void);
int wt_thumbnail_is_cached_world(const char *world_id);
void wt_thumbnail_invalidate_cache(void);
int wt_thumbnail_exists(const char *world_id);

int wt_thumbnail_install_ui_hooks(void);
void wt_thumbnail_remove_ui_hooks(void);
int wt_thumbnail_load_into_texture(void *texture, const char *world_id);

#endif
