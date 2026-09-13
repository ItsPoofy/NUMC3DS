#include "fps_overlay.h"
#include "ui_widgets.h"
#include "ui_runtime.h"
#include "../seams.h"
#include "../rt.h"
#include "../util/string_util.h"

static int s_fps_enabled = 0;
static u32 s_fps_frames = 0;
static u32 s_fps_last_time_ms = 0;
static u32 s_fps_display_value = 0;
static char s_fps_text[16] = "FPS: --";
static UiCachedText s_fps_cache;
static const Color s_fps_color = {1.0f, 1.0f, 1.0f, 1.0f};

int fps_overlay_enabled(void) {
    return s_fps_enabled;
}

void fps_overlay_set_enabled(int enabled) {
    s_fps_enabled = enabled ? 1 : 0;
    if (!s_fps_enabled) {
        s_fps_frames = 0;
        s_fps_last_time_ms = 0;
        s_fps_display_value = 0;
        s_fps_text[0] = 'F';
        s_fps_text[1] = 'P';
        s_fps_text[2] = 'S';
        s_fps_text[3] = ':';
        s_fps_text[4] = ' ';
        s_fps_text[5] = '-';
        s_fps_text[6] = '-';
        s_fps_text[7] = '\0';
    }
}

static void format_fps_text(char *out, u32 fps) {
    char digits[10];
    int count = 0;
    int pos = 0;

    out[pos++] = 'F';
    out[pos++] = 'P';
    out[pos++] = 'S';
    out[pos++] = ':';
    out[pos++] = ' ';

    if (fps == 0) {
        digits[count++] = '0';
    } else {
        while (fps > 0 && count < 10) {
            digits[count++] = (char)('0' + (fps % 10));
            fps /= 10;
        }
    }
    while (count > 0) {
        out[pos++] = digits[--count];
    }
    out[pos] = '\0';
}

void fps_overlay_render(void *screen) {
    if (!s_fps_enabled || !screen) return;

    u32 now = ((u32(*)(void))SEAM_RakNet_GetTimeMS)();
    s_fps_frames++;

    if (s_fps_last_time_ms == 0) {
        s_fps_last_time_ms = now;
    } else {
        u32 elapsed = now - s_fps_last_time_ms;
        if (elapsed >= 500) {
            s_fps_display_value = (s_fps_frames * 1000u + (elapsed / 2u)) / elapsed;
            s_fps_frames = 0;
            s_fps_last_time_ms = now;
            format_fps_text(s_fps_text, s_fps_display_value);
        }
    }

    ui_cached_text_set(screen, &s_fps_cache, s_fps_text);
    if (s_fps_cache.ready && s_fps_cache.text[0]) {
        float x = 400.0f - (float)s_fps_cache.width - 6.0f;
        float y = 4.0f;
        ui_cached_text_draw(screen, &s_fps_cache, x, y, 1.0f, &s_fps_color);
    }
}
