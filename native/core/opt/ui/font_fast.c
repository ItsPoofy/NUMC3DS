#include "font_fast.h"
#include "../../state.h"

#define SEAM_Font_getTextWidth 0x0071AD28u
#define SEAM_utf8_decode_char  0x0011E394u

typedef struct {
    void *font;
    float base_scale;
    u8 widths[128];
    u8 space_width;
    u8 initialized;
} FontAsciiCache;

static FontAsciiCache s_font_cache;
static NuMC3DS_Hook s_font_hook;

static int font_lookup_glyph_width(void *font, u32 codepoint)
{
    if (!font) return 0;
    void **tree_root_ptr = (void**)((u8*)font + 0x2B4);
    void *header = *tree_root_ptr;
    if (!header) return 0;

    void *node = *((void**)header + 1); /* [header + 4] = root */
    void *found = header;

    while (node) {
        u32 key = *((u32*)node + 4); /* [node + 0x10] */
        if (key >= codepoint) {
            found = node;
            node = *((void**)node + 2); /* [node + 8] = left */
        } else {
            node = *((void**)node + 3); /* [node + 12] = right */
        }
    }

    if (found != header && *((u32*)found + 4) == codepoint) {
        int raw_w = *((int*)found + 11); /* [found + 0x2C] */
        float base_scale = *(float*)((u8*)font + 0x2C4);
        return (int)((float)raw_w * base_scale);
    }
    return 0;
}

static void font_cache_update(void *font, float base_scale)
{
    s_font_cache.font = font;
    s_font_cache.base_scale = base_scale;
    s_font_cache.space_width = (u8)((int)(base_scale * 4.0f));

    for (u32 c = 0; c < 128; c++) {
        s_font_cache.widths[c] = (u8)font_lookup_glyph_width(font, c);
    }
    s_font_cache.widths[' '] = s_font_cache.space_width;
    s_font_cache.widths[0] = s_font_cache.space_width;
    s_font_cache.initialized = 1;
}

int __attribute__((pcs("aapcs-vfp"))) font_fast_getTextWidth(void *font, const void *str_obj, int flag, float scale)
{
    if (!font || !str_obj) return 0;
    const char *src = *(const char**)str_obj;
    if (!src) return 0;
    int remaining = *(const int*)(src - 4);
    if (remaining <= 0) return 0;

    float base_scale = *(float*)((u8*)font + 0x2C4);
    if (!s_font_cache.initialized || s_font_cache.font != font || s_font_cache.base_scale != base_scale) {
        font_cache_update(font, base_scale);
    }

    int current_line_width = 0;
    int max_line_width = 0;
    int bold = 0;

    while (remaining > 0) {
        u8 b = (u8)*src;
        u32 codepoint;
        int consumed;

        if (b < 0x80) {
            codepoint = b;
            consumed = 1;
        } else {
            consumed = ((int (*)(const char*, int, u32*))SEAM_utf8_decode_char)(src, remaining, &codepoint);
            if (consumed <= 0) break;
        }

        src += consumed;
        remaining -= consumed;

        /* Formatting code handling: when flag == 0 and codepoint == 0x1E */
        if (flag == 0 && codepoint == 0x1E) {
            if (remaining >= 1) {
                char fmt = *src;
                if (fmt == 'l') bold = 1;
                else if (fmt == 'r') bold = 0;
                src++;
                remaining--;
            }
            continue;
        }

        int w;
        if (codepoint == 0x20 || codepoint == 0xA0 || codepoint == 0) {
            w = s_font_cache.space_width;
        } else if (codepoint < 128) {
            w = s_font_cache.widths[codepoint];
        } else {
            w = font_lookup_glyph_width(font, codepoint);
        }

        if (codepoint == 0x0A) { /* \n newline */
            if (current_line_width > max_line_width) {
                max_line_width = current_line_width;
            }
            current_line_width = 0;
            continue;
        }

        if (codepoint == 0x1E) {
            if (remaining >= 1) {
                char fmt = *src;
                if (fmt == 'l') bold = 1;
                else if (fmt == 'r') bold = 0;
            }
        }

        if (w > 0) {
            current_line_width += w;
            if (bold) {
                current_line_width += 1;
            }
        }
    }

    float f_max = (float)max_line_width * scale;
    float f_cur = (float)current_line_width * scale;
    int s_max = (int)f_max;
    int s_cur = (int)f_cur;
    return (s_max > s_cur) ? s_max : s_cur;
}

int font_fast_install_hooks(void)
{
    s_font_hook.target = SEAM_Font_getTextWidth;
    s_font_hook.replacement = (u32)font_fast_getTextWidth;
    s_font_hook.expected[0] = 0xE92D4FF0u;
    s_font_hook.expected[1] = 0xE3A04000u;
    return s->host.install_hook(&s_font_hook) ? -80 : 0;
}
