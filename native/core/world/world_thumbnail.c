#include "world_thumbnail.h"
#include "../world_transfer/world_transfer_fs.h"
#include "../ui/world_thumbnail_ui.h"
#include "../ui/hand_visibility.h"
#include "../util/string_util.h"
#include "../state.h"
#include "../seams.h"

#define SEAM_Texture_getImageBuffer            0x001B340Cu
#define SEAM_Core_imageBuffer                  0x004F0C44u
#define SEAM_Texture_flush                     0x001B34B4u

static u8 s_tile_row[WT_THUMBNAIL_ROW_SIZE];
static u8 s_clean_cache[WT_THUMBNAIL_CACHE_SIZE];
static int s_has_cache = 0;
static char s_cached_world_id[64] = {0};
static int s_save_pending = 0;

static int s_clean_frame_ready = 0;
static int s_save_screen_deferred = 0;
static int s_capture_wait_frames = 0;
static int s_saved_hand_hidden = 0;
static int s_saved_hud_hidden = 0;
static void *s_deferred_chooser = 0;
static int s_deferred_exit_after_save = 0;
static NuMC3DS_Hook s_push_saving_screen_hook;
static NuMC3DS_Hook s_frame_submit_hook;
static NuMC3DS_Hook s_pause_render_hook;

static void wt_thumbnail_restore_visibility(void){
    if(!s_save_screen_deferred) return;
    hand_visibility_set_hidden(s_saved_hand_hidden);
    top_hud_visibility_set_hidden(s_saved_hud_hidden);
}

static int wt_thumbnail_build_path(char *out, unsigned cap, const char *world_id){
    char world_dir[WT_FS_MAX_PATH];
    if(!out || !cap || !world_id || !*world_id) return 0;
    if(!wt_fs_join(world_dir, sizeof(world_dir), "extdata:/minecraftWorlds", world_id)) return 0;
    return wt_fs_join(out, cap, world_dir, "world_icon.3dst");
}

static int wt_thumbnail_extract_world_id(void *storage, char *out, unsigned cap){
    const char *text = 0;
    unsigned length = 0, start = 0, end, i, j;
    static const char prefix[] = "minecraftWorlds/";
    if(!storage || !out || cap < 2) return 0;
    cp(&text, (u8*)storage + 0x28, sizeof(text));
    if(!text) return 0;
    while(length < WT_FS_MAX_PATH && text[length]) length++;
    if(!length || length >= WT_FS_MAX_PATH) return 0;
    for(i = 0; i + sizeof(prefix) - 1 <= length; i++){
        if(i && text[i - 1] != '/') continue;
        for(j = 0; j < sizeof(prefix) - 1 && text[i + j] == prefix[j]; j++){}
        if(j == sizeof(prefix) - 1){
            start = i + j;
            break;
        }
    }
    if(!start) return 0;
    while(start < length && text[start] == '/') start++;
    end = start;
    while(end < length && text[end] != '/') end++;
    if(end == start || end - start >= cap) return 0;
    cp(out, text + start, end - start);
    out[end - start] = 0;
    return wt_fs_validate_relative(out);
}

void wt_thumbnail_on_save_marker(void *storage){
    char world_id[64];
    if(wt_thumbnail_extract_world_id(storage, world_id, sizeof(world_id)))
        wt_thumbnail_save_pending(world_id);
}

int wt_thumbnail_exists(const char *world_id){
    char path[WT_FS_MAX_PATH];
    int is_dir = 0;
    if(!wt_thumbnail_build_path(path, sizeof(path), world_id)) return 0;
    return wt_fs_exists(path, &is_dir) && !is_dir;
}

int wt_thumbnail_has_cache(void){
    return s_has_cache;
}

int wt_thumbnail_is_cached_world(const char *world_id){
    return s_has_cache && s_cached_world_id[0] && world_id && streq(s_cached_world_id, world_id);
}

void wt_thumbnail_invalidate_cache(void){
    wt_thumbnail_restore_visibility();
    s_has_cache = 0;
    s_cached_world_id[0] = '\0';
    s_save_pending = 0;
    s_clean_frame_ready = 0;
    s_save_screen_deferred = 0;
    s_capture_wait_frames = 0;
    s_deferred_chooser = 0;
    s_deferred_exit_after_save = 0;
    wt_thumbnail_ui_invalidate(0);
}

static int wt_thumbnail_capture_frame(void){
    void *disp_ctx;
    u32 buf_idx;
    const u8 *fb;
    int y, x;

    disp_ctx = *(void**)0x00A35884;
    if(!disp_ctx) return 0;
    buf_idx = *(u32*)((u8*)disp_ctx + 0x194);
    if(buf_idx > 2) return 0;
    fb = *(const u8**)((u8*)disp_ctx + 0x16C + (buf_idx * 4));
    if(!fb || (u32)fb < 0x10000000u) return 0;

    for(y = 0; y < WT_THUMBNAIL_PIC_H; y++){
        int srcY = 5 + (y * 5) / 2;
        if(srcY > 239) srcY = 239;
        for(x = 0; x < WT_THUMBNAIL_PIC_W; x++){
            int srcX = (x * 5) / 2;
            const u8 *p_src;
            u8 *p_dst;
            if(srcX > 399) srcX = 399;
            p_src = fb + (srcX * 240 + (239 - srcY)) * 3;
            p_dst = s_clean_cache + (y * WT_THUMBNAIL_PIC_W + x) * 3;
            p_dst[0] = p_src[0];
            p_dst[1] = p_src[1];
            p_dst[2] = p_src[2];
        }
    }

    s_has_cache = 1;
    s_cached_world_id[0] = '\0';
    return 1;
}

static int wt_thumbnail_save_from_cache(const char *world_id){
    char path[WT_FS_MAX_PATH];
    u32 header[8];
    WtFsFile file;
    u32 wrote = 0;
    int r, sy, x;

    if(!world_id || !*world_id) return 0;
    if(!s_has_cache || !s_clean_cache) return 0;
    if(!wt_thumbnail_build_path(path, sizeof(path), world_id)) return 0;

    wt_fs_delete_file(path);
    if(!wt_fs_create_file(path, (long long)WT_THUMBNAIL_FILE_SIZE)) return 0;
    if(!wt_fs_open_file(path, 1, &file)) return 0;

    header[0] = 0x54534433u;         /* "3DST" */
    header[1] = 3u;                  /* version 3 */
    header[2] = 1u;                  /* mode 1: BGR888 */
    header[3] = WT_THUMBNAIL_TEX_W;  /* 256 */
    header[4] = WT_THUMBNAIL_TEX_H;  /* 128 */
    header[5] = WT_THUMBNAIL_PIC_W;  /* 160 */
    header[6] = WT_THUMBNAIL_PIC_H;  /* 92 */
    header[7] = 1u;                  /* mip_count */

    if(!wt_fs_write(&file, 0LL, header, sizeof(header), &wrote) || wrote != sizeof(header)){
        wt_fs_close_file(&file);
        wt_fs_delete_file(path);
        return 0;
    }

    for(r = 0; r < 16; r++){
        long long file_offset = 32LL + ((long long)r * (long long)WT_THUMBNAIL_ROW_SIZE);
        zero(s_tile_row, sizeof(s_tile_row));

        if(r >= 4){
            for(sy = 0; sy < 8; sy++){
                int y_tex = r * 8 + sy;
                int y_screen = (WT_THUMBNAIL_TEX_H - 1) - y_tex;
                if(y_screen >= 0 && y_screen < WT_THUMBNAIL_PIC_H){
                    for(x = 0; x < WT_THUMBNAIL_PIC_W; x++){
                        int tx = x & ~7;
                        int sx = x & 7;
                        int morton = (sx & 1) | ((sy & 1) << 1) | ((sx & 2) << 1) | ((sy & 2) << 2) | ((sx & 4) << 2) | ((sy & 4) << 3);
                        int pixel_idx = (tx * 8) + morton;

                        const u8 *p_src = s_clean_cache + (y_screen * WT_THUMBNAIL_PIC_W + x) * 3;
                        u8 *p_dst = s_tile_row + (pixel_idx * 3);

                        p_dst[0] = p_src[0]; /* B */
                        p_dst[1] = p_src[1]; /* G */
                        p_dst[2] = p_src[2]; /* R */
                    }
                }
            }
        }

        if(!wt_fs_write(&file, file_offset, s_tile_row, sizeof(s_tile_row), &wrote) || wrote != sizeof(s_tile_row)){
            wt_fs_close_file(&file);
            wt_fs_delete_file(path);
            return 0;
        }
    }

    wt_fs_close_file(&file);
    wt_fs_copy_text(s_cached_world_id, sizeof(s_cached_world_id), world_id);
    wt_thumbnail_ui_invalidate(world_id);

    return 1;
}

int wt_thumbnail_save_pending(const char *world_id){
    int pending;
    if(!world_id || !*world_id) return 0;
    pending = s_save_pending;
    s_save_pending = 0;
    if(!pending) return 0;
    if(!s_clean_frame_ready) return 0;
    s_clean_frame_ready = 0;
    return wt_thumbnail_save_from_cache(world_id);
}

int wt_thumbnail_load_into_texture(void *tex, const char *world_id){
    void *core;
    u32 *dst;
    char path[WT_FS_MAX_PATH];
    WtFsFile file;
    u32 read_bytes = 0;
    u32 header[8];
    int r, sy, x;

    if(!tex || !world_id || !*world_id) return 0;
    core = ((void*(*)(void*))SEAM_Texture_getImageBuffer)(tex);
    if(!core || (u32)core < 0x00100000u || *(void**)core == 0) return 0;
    dst = (u32*)((void*(*)(void*, int, int))SEAM_Core_imageBuffer)(core, 0, 0);
    if(!dst || (u32)dst < 0x00100000u) return 0;

    zero(dst, WT_THUMBNAIL_TEX_W * WT_THUMBNAIL_TEX_H * 4);

    if(s_has_cache && s_clean_cache && wt_thumbnail_is_cached_world(world_id)){
        for(sy = 0; sy < WT_THUMBNAIL_PIC_H; sy++){
            for(x = 0; x < WT_THUMBNAIL_PIC_W; x++){
                const u8 *p = s_clean_cache + (sy * WT_THUMBNAIL_PIC_W + x) * 3;
                dst[sy * WT_THUMBNAIL_TEX_W + x] = 0xFF000000u | ((u32)p[0] << 16) | ((u32)p[1] << 8) | (u32)p[2];
            }
        }
        ((void(*)(void*))SEAM_Texture_flush)(tex);
        return 1;
    }

    if(!wt_thumbnail_build_path(path, sizeof(path), world_id)) return 0;
    if(!wt_fs_open_file(path, 0, &file)) return 0;

    if(!wt_fs_read(&file, 0LL, header, sizeof(header), &read_bytes) || read_bytes != sizeof(header)){
        wt_fs_close_file(&file);
        return 0;
    }
    if(header[0] != 0x54534433u || header[1] != 3u || header[2] != 1u ||
       header[3] != WT_THUMBNAIL_TEX_W || header[4] != WT_THUMBNAIL_TEX_H ||
       header[5] != WT_THUMBNAIL_PIC_W || header[6] != WT_THUMBNAIL_PIC_H || header[7] != 1u){
        wt_fs_close_file(&file);
        return 0;
    }

    for(r = 4; r < 16; r++){
        long long file_offset = 32LL + ((long long)r * (long long)WT_THUMBNAIL_ROW_SIZE);
        if(!wt_fs_read(&file, file_offset, s_tile_row, sizeof(s_tile_row), &read_bytes) || read_bytes != sizeof(s_tile_row)){
            wt_fs_close_file(&file);
            return 0;
        }
        for(sy = 0; sy < 8; sy++){
            int y_tex = r * 8 + sy;
            int y_screen = (WT_THUMBNAIL_TEX_H - 1) - y_tex;
            if(y_screen >= 0 && y_screen < WT_THUMBNAIL_PIC_H){
                for(x = 0; x < WT_THUMBNAIL_PIC_W; x++){
                    int tx = x & ~7;
                    int sx = x & 7;
                    int morton = (sx & 1) | ((sy & 1) << 1) | ((sx & 2) << 1) | ((sy & 2) << 2) | ((sx & 4) << 2) | ((sy & 4) << 3);
                    int pixel_idx = (tx * 8) + morton;
                    const u8 *p = s_tile_row + (pixel_idx * 3);
                    dst[y_screen * WT_THUMBNAIL_TEX_W + x] = 0xFF000000u | ((u32)p[0] << 16) | ((u32)p[1] << 8) | (u32)p[2];
                }
            }
        }
    }

    wt_fs_close_file(&file);
    ((void(*)(void*))SEAM_Texture_flush)(tex);
    return 1;
}

static void on_push_saving_screen(void *chooser, int exit_after_save){
    if(s_save_screen_deferred) return;
    s_saved_hand_hidden = hand_visibility_hidden();
    s_saved_hud_hidden = top_hud_visibility_hidden();
    hand_visibility_set_hidden(1);
    top_hud_visibility_set_hidden(1);
    s_deferred_chooser = chooser;
    s_deferred_exit_after_save = exit_after_save;
    s_save_screen_deferred = 1;
    s_capture_wait_frames = 0;
    s_save_pending = 1;
    s_clean_frame_ready = 0;
}

static void wt_thumbnail_finish_deferred_save(void){
    void *chooser = s_deferred_chooser;
    int exit_after_save = s_deferred_exit_after_save;
    wt_thumbnail_restore_visibility();
    s_save_screen_deferred = 0;
    s_deferred_chooser = 0;
    ((void(*)(void*,int))s_push_saving_screen_hook.trampoline)(chooser,exit_after_save);
}

int wt_thumbnail_capture_active(void){
    return s_save_screen_deferred;
}

static void on_frame_submit(void *context){
    ((void(*)(void*))s_frame_submit_hook.trampoline)(context);
    if(!s_save_screen_deferred || ++s_capture_wait_frames < 2) return;
    ((void(*)(void))0x007C6DA8u)();
    s_clean_frame_ready = wt_thumbnail_capture_frame();
    wt_thumbnail_finish_deferred_save();
}

static void on_pause_render(void *screen, u32 touch_x, u32 touch_y, u32 mask){
    if(!s_save_screen_deferred)
        ((void(*)(void*,u32,u32,u32))s_pause_render_hook.trampoline)(screen,touch_x,touch_y,mask);
}

int wt_thumbnail_install_ui_hooks(void){
    s_pause_render_hook.target = SEAM_PauseScreen_render;
    s_pause_render_hook.replacement = (u32)on_pause_render;
    s_pause_render_hook.expected[0] = 0xE92D41F0u;
    s_pause_render_hook.expected[1] = 0xE3130040u;
    if(s->host.install_hook(&s_pause_render_hook)) return -1;

    s_frame_submit_hook.target = 0x004F87C4u;
    s_frame_submit_hook.replacement = (u32)on_frame_submit;
    s_frame_submit_hook.expected[0] = 0xE92D47F0u;
    s_frame_submit_hook.expected[1] = 0xE1A04000u;
    if(s->host.install_hook(&s_frame_submit_hook)){
        s->host.remove_hook(&s_pause_render_hook);
        zero(&s_pause_render_hook, sizeof(s_pause_render_hook));
        return -2;
    }

    s_push_saving_screen_hook.target = SEAM_PushSavingScreen;
    s_push_saving_screen_hook.replacement = (u32)on_push_saving_screen;
    s_push_saving_screen_hook.expected[0] = 0xE92D43F0u;
    s_push_saving_screen_hook.expected[1] = 0xE24DD00Cu;
    if(s->host.install_hook(&s_push_saving_screen_hook)){
        s->host.remove_hook(&s_frame_submit_hook);
        s->host.remove_hook(&s_pause_render_hook);
        zero(&s_frame_submit_hook, sizeof(s_frame_submit_hook));
        zero(&s_pause_render_hook, sizeof(s_pause_render_hook));
        return -3;
    }

    if(wt_thumbnail_ui_install()){
        s->host.remove_hook(&s_push_saving_screen_hook);
        s->host.remove_hook(&s_frame_submit_hook);
        s->host.remove_hook(&s_pause_render_hook);
        zero(&s_push_saving_screen_hook, sizeof(s_push_saving_screen_hook));
        zero(&s_frame_submit_hook, sizeof(s_frame_submit_hook));
        zero(&s_pause_render_hook, sizeof(s_pause_render_hook));
        return -4;
    }
    return 0;
}

void wt_thumbnail_remove_ui_hooks(void){
    wt_thumbnail_ui_remove();
    if(s_push_saving_screen_hook.trampoline) s->host.remove_hook(&s_push_saving_screen_hook);
    if(s_frame_submit_hook.trampoline) s->host.remove_hook(&s_frame_submit_hook);
    if(s_pause_render_hook.trampoline) s->host.remove_hook(&s_pause_render_hook);
    zero(&s_push_saving_screen_hook, sizeof(s_push_saving_screen_hook));
    zero(&s_frame_submit_hook, sizeof(s_frame_submit_hook));
    zero(&s_pause_render_hook, sizeof(s_pause_render_hook));
    wt_thumbnail_invalidate_cache();
}
