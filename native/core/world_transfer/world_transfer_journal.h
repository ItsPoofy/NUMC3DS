#ifndef NUMC3DS_WORLD_TRANSFER_JOURNAL_H
#define NUMC3DS_WORLD_TRANSFER_JOURNAL_H

#include "world_transfer_manifest.h"

enum {
    WT_JOURNAL_MAGIC = 0x4A54574Eu,
    WT_JOURNAL_VERSION = 1,
    WT_JOURNAL_IMPORT = 1,
    WT_JOURNAL_EXPORT = 2,
    WT_JOURNAL_SCAN = 1,
    WT_JOURNAL_COPY = 2,
    WT_JOURNAL_VERIFY = 3,
    WT_JOURNAL_COMMIT = 4,
    WT_JOURNAL_CLEANUP = 5,
    WT_JOURNAL_DONE = 6
};

typedef struct {
    u32 magic, version, operation, phase, transaction_id, cleanup_state;
    WtTransferManifest manifest;
    char source[WT_FS_MAX_PATH];
    char staging[WT_FS_MAX_PATH];
    char final_path[WT_FS_MAX_PATH];
    char backup[WT_FS_MAX_PATH];
    char source_fingerprint[32];
} WtTransferJournalRecord;

void wt_journal_reset(WtTransferJournalRecord *record,u32 operation,u32 transaction_id);
int wt_journal_write(const WtTransferJournalRecord *record);
int wt_journal_read(u32 operation,WtTransferJournalRecord *record);
int wt_journal_clear(u32 operation);
int wt_journal_recover(void);
void wt_journal_mark_dirty(void);

#endif
