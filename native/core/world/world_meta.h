#ifndef NUMC3DS_WORLD_META_H
#define NUMC3DS_WORLD_META_H

#include "../rt.h"

#define WT_WORLD_META_MAGIC 0x54454D57u /* "WMET" */
#define WT_WORLD_META_VERSION 1u

typedef struct {
    u32 magic;
    u32 version;
    unsigned long long total_size;
    unsigned long long level_mtime;
    u32 has_thumbnail;
    u32 reserved[3];
} WtWorldMeta;

int wt_meta_read(const char *world_id, WtWorldMeta *out);
int wt_meta_write(const char *world_id, unsigned long long total_size, u32 has_thumbnail);
int wt_meta_invalidate(const char *world_id);

#endif
