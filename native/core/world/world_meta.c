#include "world_meta.h"
#include "../world_transfer/world_transfer_fs.h"

static int wt_meta_build_path(char *out, unsigned cap, const char *world_id){
    char world_dir[WT_FS_MAX_PATH];
    if(!out || !cap || !world_id || !*world_id) return 0;
    if(!wt_fs_join(world_dir, sizeof(world_dir), "extdata:/minecraftWorlds", world_id)) return 0;
    return wt_fs_join(out, cap, world_dir, "world_meta.bin");
}

int wt_meta_read(const char *world_id, WtWorldMeta *out){
    char path[WT_FS_MAX_PATH];
    WtFsFile file;
    u32 read_bytes = 0;
    WtWorldMeta meta;

    if(!out || !world_id || !*world_id) return 0;
    zero(out, sizeof(*out));
    if(!wt_meta_build_path(path, sizeof(path), world_id)) return 0;
    if(!wt_fs_open_file(path, 0, &file)) return 0;
    if(!wt_fs_read(&file, 0LL, &meta, sizeof(meta), &read_bytes) || read_bytes != sizeof(meta)){
        wt_fs_close_file(&file);
        return 0;
    }
    wt_fs_close_file(&file);

    if(meta.magic != WT_WORLD_META_MAGIC || meta.version != WT_WORLD_META_VERSION || meta.total_size == 0){
        return 0;
    }
    cp(out, &meta, sizeof(meta));
    return 1;
}

int wt_meta_write(const char *world_id, unsigned long long total_size, u32 has_thumbnail){
    char path[WT_FS_MAX_PATH];
    WtFsFile file;
    u32 wrote = 0;
    WtWorldMeta meta;

    if(!world_id || !*world_id || total_size == 0) return 0;
    if(!wt_meta_build_path(path, sizeof(path), world_id)) return 0;

    zero(&meta, sizeof(meta));
    meta.magic = WT_WORLD_META_MAGIC;
    meta.version = WT_WORLD_META_VERSION;
    meta.total_size = total_size;
    meta.has_thumbnail = has_thumbnail;

    wt_fs_delete_file(path);
    if(!wt_fs_create_file(path, (long long)sizeof(meta))) return 0;
    if(!wt_fs_open_file(path, 1, &file)) return 0;
    if(!wt_fs_write(&file, 0LL, &meta, sizeof(meta), &wrote) || wrote != sizeof(meta)){
        wt_fs_close_file(&file);
        wt_fs_delete_file(path);
        return 0;
    }
    wt_fs_close_file(&file);
    return 1;
}

int wt_meta_invalidate(const char *world_id){
    char path[WT_FS_MAX_PATH];
    if(!world_id || !*world_id) return 0;
    if(!wt_meta_build_path(path, sizeof(path), world_id)) return 0;
    return wt_fs_delete_file(path);
}
