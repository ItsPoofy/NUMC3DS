#include "title_service.h"
#include "../util/string_util.h"
#include "../rt.h"
#include "../state.h"

void title_apply_local(const char* op, const char* text_arg) {
    if (streq(op, "clear") || streq(op, "reset")) {
        s->title_text[0] = 0;
        s->subtitle_text[0] = 0;
        s->title_ticks = 0;
        return;
    }
    if (streq(op, "title")) {
        copy_text(s->title_text, text_arg, sizeof(s->title_text) - 1);
        s->subtitle_text[0] = 0;
        s->title_ticks = s->title_stay ? s->title_stay : 70;
        return;
    }
    if (streq(op, "subtitle") || streq(op, "actionbar")) {
        copy_text(s->subtitle_text, text_arg, sizeof(s->subtitle_text) - 1);
        if (!s->title_ticks) s->title_ticks = 70;
        return;
    }
}
