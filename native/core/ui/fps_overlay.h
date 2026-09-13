#ifndef NUMC3DS_FPS_OVERLAY_H
#define NUMC3DS_FPS_OVERLAY_H

int fps_overlay_enabled(void);
void fps_overlay_set_enabled(int enabled);
void fps_overlay_render(void *screen);

#endif
