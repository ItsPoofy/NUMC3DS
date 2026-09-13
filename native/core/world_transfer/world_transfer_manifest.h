#ifndef NUMC3DS_WORLD_TRANSFER_MANIFEST_H
#define NUMC3DS_WORLD_TRANSFER_MANIFEST_H

#include "world_transfer_fs.h"

typedef struct {
    u32 entries;
    u32 files;
    u32 directories;
    long long bytes;
    u32 path_fingerprint;
    u32 content_fingerprint;
} WtTransferManifest;

void wt_manifest_reset(WtTransferManifest *manifest);
void wt_manifest_add_directory(WtTransferManifest *manifest,const char *relative_path);
void wt_manifest_add_file(WtTransferManifest *manifest,const char *relative_path,long long size,u32 content_hash);
int wt_manifest_equal(const WtTransferManifest *left,const WtTransferManifest *right);
u32 wt_manifest_bytes_hash(u32 seed,const void *data,u32 size);
int wt_manifest_fingerprint_text(const WtTransferManifest *manifest,char *out,unsigned cap);

/* Replaces invalid filesystem characters, removes trailing dots/spaces, and
 * truncates on code-point boundaries.  The result is always non-empty. */
int wt_sanitize_name(const char *source,char *out,unsigned cap);

#endif
