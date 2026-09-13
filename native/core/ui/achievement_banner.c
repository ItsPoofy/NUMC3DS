#include "achievement_banner.h"
#include "ui_types.h"
#include "ui_localize.h"
#include "ui_text_layout.h"
#include "ui_widgets.h"
#include "ui_runtime.h"
#include "../seams.h"
#include "../state.h"
#include "../internal.h"

static void *s_banner_vtable[48];
static int s_banner_vtable_ready = 0;

static UiCachedText s_eligible_lines[2];
static unsigned s_eligible_line_count = 0;

static UiCachedText s_ineligible_lines[2];
static unsigned s_ineligible_line_count = 0;

static int s_banner_eligible = 1;

static void achievement_banner_render(void *element, void *game, int touch_x, int touch_y) {
    static const Color black = {0.0f, 0.0f, 0.0f, 1.0f};
    static const Color dark_bg = {0.188235f, 0.188235f, 0.188235f, 1.0f};
    static const Color white = {1.0f, 1.0f, 1.0f, 1.0f};
    UiElementView *e = (UiElementView*)element;
    void *screen = s ? s->world_screen : 0;
    UiCachedText *lines;
    unsigned line_count, i;
    int x, y, w, h, line_x, start_y;

    (void)game; (void)touch_x; (void)touch_y;
    if (!e || !e->visible || !screen) return;

    x = e->x;
    y = e->y;
    w = e->width;
    h = e->height;

    ui_fill(screen, x, y, x + w, y + 1, &black);
    ui_fill(screen, x, y + h - 1, x + w, y + h, &black);
    ui_fill(screen, x, y, x + 1, y + h, &black);
    ui_fill(screen, x + w - 1, y, x + w, y + h, &black);
    ui_fill(screen, x + 1, y + 1, x + w - 1, y + h - 1, &dark_bg);

    if (s_banner_eligible) {
        lines = s_eligible_lines;
        line_count = s_eligible_line_count;
    } else {
        lines = s_ineligible_lines;
        line_count = s_ineligible_line_count;
    }

    if (!line_count) return;
    line_x = x + 10;
    start_y = y + (h - (line_count == 2 ? 21 : 8)) / 2;
    for (i = 0; i < line_count; i++) {
        ui_cached_text_draw(screen, &lines[i], (float)line_x, (float)(start_y + (int)i * 13), 1.0f, &white);
    }
}

static void achievement_banner_draw_outline_noop(void *element, int offset) {
    (void)element; (void)offset;
}

static void init_banner_vtable(void) {
    if (s_banner_vtable_ready) return;
    cp(s_banner_vtable, (void*)0x00996958u, 40 * sizeof(void*));
    s_banner_vtable[7] = (void*)achievement_banner_render;
    s_banner_vtable[27] = (void*)achievement_banner_draw_outline_noop;
    s_banner_vtable[30] = (void*)achievement_banner_draw_outline_noop;
    s_banner_vtable_ready = 1;
}

static void *s_banner_font = 0;
static int s_banner_cached = 0;

void achievement_banner_invalidate_cache(void) {
    s_banner_cached = 0;
    s_banner_font = 0;
}

void achievement_banner_setup_cache(void *screen) {
    char raw_text[192];
    char line_buf[2][96];
    void *font;
    unsigned line_count, at, next, i;
    int max_w = 216;

    if (!screen || !ui_resources_ready()) return;
    font = ui_screen_font(screen);
    if (!font) return;
    if (s_banner_cached && s_banner_font == font) return;

    /* 1. Pre-cache Eligible text */
    for (i = 0; i < 2; i++) ui_cached_text_reset(&s_eligible_lines[i]);
    ui_localized_label(raw_text, sizeof(raw_text), "options.achievementsDisabled.notSignedIn", "Achievements can be earned in this world.");
    if (streq(raw_text, "Achievements can be earned in this world.")) {
        ui_cached_text_set(screen, &s_eligible_lines[0], "Achievements can be earned");
        ui_cached_text_set(screen, &s_eligible_lines[1], "in this world.");
        s_eligible_line_count = 2;
    } else {
        at = 0; line_count = 0;
        while (at < sizeof(raw_text) && raw_text[at] && line_count < 2) {
            if (!ui_text_wrap_line(font, raw_text, at, max_w, line_buf[line_count], sizeof(line_buf[line_count]), &next)) break;
            line_count++;
            if (next <= at) break;
            at = next;
        }
        for (i = 0; i < line_count; i++) {
            ui_cached_text_set(screen, &s_eligible_lines[i], line_buf[i]);
        }
        s_eligible_line_count = line_count;
    }

    /* 2. Pre-cache Ineligible text */
    for (i = 0; i < 2; i++) ui_cached_text_reset(&s_ineligible_lines[i]);
    ui_localized_label(raw_text, sizeof(raw_text), "options.achievementsDisabled", "Achievements cannot be earned in this world.");
    if (streq(raw_text, "Achievements cannot be earned in this world.")) {
        ui_cached_text_set(screen, &s_ineligible_lines[0], "Achievements cannot be earned");
        ui_cached_text_set(screen, &s_ineligible_lines[1], "in this world.");
        s_ineligible_line_count = 2;
    } else {
        at = 0; line_count = 0;
        while (at < sizeof(raw_text) && raw_text[at] && line_count < 2) {
            if (!ui_text_wrap_line(font, raw_text, at, max_w, line_buf[line_count], sizeof(line_buf[line_count]), &next)) break;
            line_count++;
            if (next <= at) break;
            at = next;
        }
        for (i = 0; i < line_count; i++) {
            ui_cached_text_set(screen, &s_ineligible_lines[i], line_buf[i]);
        }
        s_ineligible_line_count = line_count;
    }
    s_banner_font = font;
    s_banner_cached = 1;
}

void achievement_banner_set_eligible(int eligible) {
    s_banner_eligible = eligible;
}

int ui_achievement_banner_create_shared(void *screen, UiShared *out_shared) {
    void *memory;
    UiElementView *elem;
    GameAllocWithSelector alloc = (GameAllocWithSelector)SEAM_Heap_allocWithSelector;

    (void)screen;
    if (!out_shared) return 0;
    out_shared->object = out_shared->control = 0;

    init_banner_vtable();

    memory = alloc(0x40, (void*)SEAM_game_alloc_selector);
    if (!memory) return 0;
    zero(memory, 0x40);

    elem = (UiElementView*)memory;
    elem->vtable = s_banner_vtable;
    *(void**)((u8*)memory + 4) = (void*)(0x00996958u + 0x88);
    elem->active = 1;
    elem->visible = 1;
    elem->x = 0;
    elem->y = 0;
    elem->width = 236;
    elem->height = 44;
    elem->screen_mask = 0x80;

    return ui_shared_from_object(memory, out_shared);
}

int achievement_banner_install_hooks(void) {
    return 0;
}
