#include "world_transfer_journal.h"

static const char *wt_journal_path(u32 operation){return operation==WT_JOURNAL_IMPORT?"extdata:/numc3ds-transfer/import.journal":"sdmc:/Minecraft 3DS/.numc3ds-transfer/export.journal";}
static const char *wt_journal_temp_path(u32 operation){return operation==WT_JOURNAL_IMPORT?"extdata:/numc3ds-transfer/import.journal.tmp":"sdmc:/Minecraft 3DS/.numc3ds-transfer/export.journal.tmp";}
static int wt_owned_path(const char *path,u32 operation){const char *prefix=operation==WT_JOURNAL_IMPORT?"extdata:/numc3ds-transfer/":"sdmc:/Minecraft 3DS/.numc3ds-transfer/";unsigned i=0;while(prefix[i]){if(!path||path[i]!=prefix[i])return 0;i++;}if(!path[i]||path[i]=='.'||path[i]=='/'||path[i]=='\\')return 0;return wt_fs_validate_relative(path+i);}
static int wt_owned_import_final(const char *path){const char prefix[]="extdata:/minecraftWorlds/";unsigned i=0;while(prefix[i]){if(!path||path[i]!=prefix[i])return 0;i++;}return path[i]&&wt_fs_validate_relative(path+i);}
static int wt_write_all(const char *path,const void *data,u32 size){WtFsFile file;u32 wrote;long long offset=0;if(!wt_fs_create_file(path,(long long)size)||!wt_fs_open_file(path,1,&file))return 0;while(offset<(long long)size){u32 chunk=(u32)((long long)size-offset);if(chunk>4096u)chunk=4096u;if(!wt_fs_write(&file,offset,(const u8*)data+offset,chunk,&wrote)||wrote!=chunk){wt_fs_close_file(&file);return 0;}offset+=chunk;}wt_fs_close_file(&file);return 1;}
static int wt_read_all(const char *path,void *data,u32 size){WtFsFile file;u32 got;long long offset=0;if(!wt_fs_open_file(path,0,&file))return 0;while(offset<(long long)size){u32 chunk=(u32)((long long)size-offset);if(chunk>4096u)chunk=4096u;if(!wt_fs_read(&file,offset,(u8*)data+offset,chunk,&got)||got!=chunk){wt_fs_close_file(&file);return 0;}offset+=chunk;}wt_fs_close_file(&file);return 1;}

static int s_journal_needs_recovery = 1;

void wt_journal_mark_dirty(void){
    s_journal_needs_recovery = 1;
}

void wt_journal_reset(WtTransferJournalRecord *record,u32 operation,u32 transaction_id){if(!record)return;zero(record,sizeof(*record));record->magic=WT_JOURNAL_MAGIC;record->version=WT_JOURNAL_VERSION;record->operation=operation;record->phase=WT_JOURNAL_SCAN;record->transaction_id=transaction_id;}
int wt_journal_write(const WtTransferJournalRecord *record){const char *temp,*final;if(!record||record->magic!=WT_JOURNAL_MAGIC||record->version!=WT_JOURNAL_VERSION)return 0;if(record->operation!=WT_JOURNAL_IMPORT&&record->operation!=WT_JOURNAL_EXPORT)return 0;if(!wt_fs_ensure_private_roots())return 0;if(record->operation==WT_JOURNAL_EXPORT&&!wt_fs_sdmc_ready())return 0;temp=wt_journal_temp_path(record->operation);final=wt_journal_path(record->operation);s_journal_needs_recovery=1;wt_fs_delete_file(temp);if(!wt_write_all(temp,record,sizeof(*record)))return 0;if(!wt_fs_rename(temp,final)){wt_fs_delete_file(temp);return 0;}return 1;}
int wt_journal_read(u32 operation,WtTransferJournalRecord *record){if(!record||(operation!=WT_JOURNAL_IMPORT&&operation!=WT_JOURNAL_EXPORT))return 0;zero(record,sizeof(*record));if(!wt_read_all(wt_journal_path(operation),record,sizeof(*record)))return 0;if(record->magic!=WT_JOURNAL_MAGIC||record->version!=WT_JOURNAL_VERSION||record->operation!=operation)return 0;return 1;}
int wt_journal_clear(u32 operation){const char *path;if(operation!=WT_JOURNAL_IMPORT&&operation!=WT_JOURNAL_EXPORT)return 0;path=wt_journal_path(operation);if(!wt_fs_delete_file(path)){int dir;if(!wt_fs_exists(path,&dir))return 1;return 0;}return 1;}
int wt_journal_recover(void){
    WtTransferJournalRecord record;
    u32 operation;
    int any_active = 0;
    if(!s_journal_needs_recovery)return 1;
    for(operation=WT_JOURNAL_IMPORT;operation<=WT_JOURNAL_EXPORT;operation++){
        int final_is_dir=0,backup_is_dir=0,cleanup_ok;
        if(!wt_journal_read(operation,&record))continue;
        any_active = 1;
        if(operation==WT_JOURNAL_IMPORT){
            if(record.cleanup_state==2){
                /* A final-tree verification failed after the staging rename.
                 * Remove only the exact journaled target; leave the inbox
                 * source untouched so it can be retried as a fresh import. */
                if(record.final_path[0]&&wt_owned_import_final(record.final_path)&&wt_fs_exists(record.final_path,&final_is_dir)&&final_is_dir){
                    if(!wt_fs_delete_tree(record.final_path))continue;
                }
                if(!record.final_path[0]||!wt_owned_import_final(record.final_path))continue;
                wt_journal_clear(operation);
                continue;
            }
            /* A committed import whose source cleanup did not finish is a
             * receipt, not an abandoned copy.  Keep it so the next world-list
             * entry can compare the source before retrying deletion. */
            if(record.cleanup_state==1||record.phase==WT_JOURNAL_CLEANUP||record.phase==WT_JOURNAL_COMMIT){
                cleanup_ok=1;
                if(record.staging[0]&&wt_owned_path(record.staging,operation)&&!wt_fs_delete_tree(record.staging)&&wt_fs_exists(record.staging,&final_is_dir))cleanup_ok=0;
                if(record.backup[0]&&wt_owned_path(record.backup,operation)&&!wt_fs_delete_tree(record.backup)&&wt_fs_exists(record.backup,&backup_is_dir))cleanup_ok=0;
                if(record.final_path[0]&&!wt_owned_import_final(record.final_path))continue;
                if(record.final_path[0]&&wt_fs_exists(record.final_path,&final_is_dir)&&final_is_dir){
                    if(record.cleanup_state!=1){record.cleanup_state=1;record.phase=WT_JOURNAL_CLEANUP;wt_journal_write(&record);}
                    continue;
                }
                if(!cleanup_ok)continue;
            }
            cleanup_ok=1;
            if(record.staging[0]&&wt_owned_path(record.staging,operation)&&!wt_fs_delete_tree(record.staging)&&wt_fs_exists(record.staging,&final_is_dir))cleanup_ok=0;
            if(record.backup[0]&&wt_owned_path(record.backup,operation)&&!wt_fs_delete_tree(record.backup)&&wt_fs_exists(record.backup,&backup_is_dir))cleanup_ok=0;
            if(!cleanup_ok)continue;
            wt_journal_clear(operation);
            continue;
        }
        /* Export replacement recovery is conservative: a private backup is
         * restored only when the public destination is absent.  If the new
         * destination exists, the backup is merely obsolete and may be
         * removed after the public tree is intact. */
        if(record.backup[0]&&wt_owned_path(record.backup,operation)&&wt_fs_exists(record.backup,&backup_is_dir)&&backup_is_dir){
            if(record.final_path[0]&&!wt_fs_exists(record.final_path,&final_is_dir)){
                if(wt_fs_rename(record.backup,record.final_path)){
                    wt_journal_clear(operation);
                    continue;
                }
            }else if(record.final_path[0]&&final_is_dir){
                if(wt_fs_delete_tree(record.backup)){
                    wt_journal_clear(operation);
                    continue;
                }
            }
            continue;
        }
        if(record.staging[0]&&wt_owned_path(record.staging,operation)&&!wt_fs_delete_tree(record.staging)&&wt_fs_exists(record.staging,&final_is_dir))continue;
        wt_journal_clear(operation);
    }
    if(!any_active)s_journal_needs_recovery=0;
    return 1;
}
