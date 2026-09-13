#ifndef NUMC3DS_MAP_PREVIEW_TEXTURE_H
#define NUMC3DS_MAP_PREVIEW_TEXTURE_H
#include "../rt.h"
typedef struct {u32 saved[8];void *sprite;} MapPreviewTexture;
int map_preview_texture_begin(MapPreviewTexture *binding,void *game,void *sprite,void *data);
void map_preview_texture_end(MapPreviewTexture *binding);
void map_preview_texture_reset(void);
void map_preview_texture_dirty(void *data);
#endif
