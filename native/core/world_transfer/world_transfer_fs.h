#ifndef NUMC3DS_WORLD_TRANSFER_FS_H
#define NUMC3DS_WORLD_TRANSFER_FS_H

#include "../rt.h"

/* CTR-SDK's filesystem limits are deliberately mirrored here.  The module
 * never hands a path longer than MAX_FILE_PATH_LENGTH to nn::fs. */
enum {
    WT_FS_MAX_PATH = 512,
    WT_FS_MAX_WIDE = 262,
    WT_FS_MAX_ENTRY_NAME = 262,
    WT_FS_DIRECTORY_BATCH = 4
};

typedef unsigned short wt_u16;

typedef struct {
    u32 type;
    const wt_u16 *data;
    u32 length;
} WtFsLowPath;

/* This is the CTR-SDK DirectoryEntry layout for ARM/LE: 262 UTF-16 code
 * units, 16 bytes of short-name/attribute data, then an s64 size. */
typedef struct {
    wt_u16 entry_name[WT_FS_MAX_ENTRY_NAME];
    u8 short_name[16];
    u8 attributes[4];
    long long entry_size;
} WtFsDirectoryEntry;

typedef struct { void *handle; } WtFsFile;
typedef struct { void *handle; } WtFsDirectory;

int wt_fs_mount_sdmc(void);
int wt_fs_sdmc_ready(void);
int wt_fs_ensure_public_roots(void);
int wt_fs_ensure_private_roots(void);

int wt_fs_open_file(const char *path, int write, WtFsFile *out);
void wt_fs_close_file(WtFsFile *file);
int wt_fs_read(WtFsFile *file, long long offset, void *buffer, u32 size, u32 *out_bytes);
int wt_fs_write(WtFsFile *file, long long offset, const void *buffer, u32 size, u32 *out_bytes);
int wt_fs_create_file(const char *path, long long size);
int wt_fs_delete_file(const char *path);
int wt_fs_file_size(const char *path, long long *out_size);

int wt_fs_open_directory(const char *path, WtFsDirectory *out);
void wt_fs_close_directory(WtFsDirectory *directory);
int wt_fs_read_directory(WtFsDirectory *directory, WtFsDirectoryEntry *entries, int max_entries, int *out_count);
int wt_fs_create_directory(const char *path);
int wt_fs_delete_directory(const char *path);
int wt_fs_delete_tree(const char *path);
int wt_fs_exists(const char *path, int *is_directory);
int wt_fs_rename(const char *current_path, const char *new_path);
int wt_fs_get_free_bytes(const char *archive_path, long long *out_bytes);
int wt_fs_mkdirs(const char *path);

int wt_fs_entry_is_directory(const WtFsDirectoryEntry *entry);
int wt_fs_entry_name_utf8(const WtFsDirectoryEntry *entry, char *out, unsigned cap);
long long wt_fs_entry_size(const WtFsDirectoryEntry *entry);

/* Path and UTF-8 validation is shared by the manifest and service layers. */
int wt_fs_validate_relative(const char *path);
int wt_fs_join(char *out, unsigned cap, const char *parent, const char *child);
int wt_fs_copy_text(char *out, unsigned cap, const char *text);

#endif
