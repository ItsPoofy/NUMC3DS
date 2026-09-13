#include "world_transfer_service.h"
#include "world_summary.h"
#include "../world/world_meta.h"
#include "../world/world_thumbnail.h"
#include "../ui/ui_grid_row.h"
#include "../ui/ui_controls.h"
#include "../ui/ui_localize.h"
#include "../ui/ui_widgets.h"
#include "../ui/ui_types.h"
#include "../ui/world_settings_ui.h"

enum {
    WT_MAX_CANDIDATES = 64,
    WT_MAX_WALK_DEPTH = 16,
    WT_IO_CHUNK = 6144,
    WT_STEPS_PER_TICK = 11,
    WT_WALK_COPY = 1,
    WT_WALK_VERIFY_SOURCE = 2,
    WT_WALK_TARGET = 3,
    WT_WALK_FINAL = 4,
    WT_WALK_METADATA = 5,
    WT_PHASE_IDLE = 0,
    WT_PHASE_IMPORT_PREPARE = 9,
    WT_PHASE_IMPORT_COPY = 10,
    WT_PHASE_IMPORT_VERIFY_SOURCE = 11,
    WT_PHASE_IMPORT_VERIFY_TARGET = 12,
    WT_PHASE_IMPORT_PARSE = 13,
    WT_PHASE_IMPORT_COMMIT = 14,
    WT_PHASE_IMPORT_FINAL_VERIFY = 15,
    WT_PHASE_IMPORT_CLEANUP = 16,
    WT_PHASE_IMPORT_CLEANUP_RETRY = 17,
    WT_PHASE_EXPORT_PREPARE = 19,
    WT_PHASE_EXPORT_COPY = 20,
    WT_PHASE_EXPORT_VERIFY_SOURCE = 21,
    WT_PHASE_EXPORT_VERIFY_TARGET = 22,
    WT_PHASE_EXPORT_COMMIT = 23,
    WT_PHASE_EXPORT_FINAL_VERIFY = 24,
    WT_PHASE_DONE = 30,
    WT_PHASE_FAILED = 31
};

typedef struct {
    WtFsDirectory directory;
    char source[WT_FS_MAX_PATH];
    char target[WT_FS_MAX_PATH];
    char relative[WT_FS_MAX_PATH];
    WtFsDirectoryEntry entries[WT_FS_DIRECTORY_BATCH];
    int count;
    int index;
    int loaded;
} WtWalkFrame;

typedef struct {
    int active;
    int mode;
    WtFsFile source;
    WtFsFile target;
    long long size;
    long long offset;
    u32 source_hash;
    u32 target_hash;
    char relative[WT_FS_MAX_PATH];
    char source_path[WT_FS_MAX_PATH];
    char target_path[WT_FS_MAX_PATH];
} WtWalkFile;

typedef struct {
    int active;
    int mode;
    int error;
    WtWalkFrame frames[WT_MAX_WALK_DEPTH];
    unsigned depth;
    WtWalkFile file;
    WtTransferManifest manifest;
    char root_source[WT_FS_MAX_PATH];
    char root_target[WT_FS_MAX_PATH];
} WtWalk;

typedef struct {
    char name[WT_FS_MAX_PATH];
} WtCandidate;

typedef struct {
    int active;
    int operation;
    int phase;
    int closing;
    int cancel_requested;
    int pending_reopen;
    int completion_hold;
    int replace;
    int replace_armed;
    void *chooser;
    void *route_unused;
    int tab_index;
    void *edit_screen;
    UiControlSet edit_controls;
    void *progress_screen;
    void *cancel_button;
    u32 candidate_count;
    u32 candidate_index;
    u32 imported_count;
    int cleanup_pending;
    WtCandidate candidates[WT_MAX_CANDIDATES];
    char internal_source[WT_FS_MAX_PATH];
    char public_source[WT_FS_MAX_PATH];
    char stage[WT_FS_MAX_PATH];
    char final_path[WT_FS_MAX_PATH];
    char backup[WT_FS_MAX_PATH];
    char message[96];
    WtTransferJournalRecord journal;
    WtTransferJournalRecord cleanup_record;
    WtTransferManifest prepared_manifest;
    WtTransferManifest copied_manifest;
    WtWalk walk;
    NuMC3DS_Hook route_hook;
    NuMC3DS_Hook list_hook;
    NuMC3DS_Hook path_size_hook;
    NuMC3DS_Hook save_marker_hook;
    NuMC3DS_Hook file_exists_hook;
    NuMC3DS_Hook stream_hook;
    u32 next_transaction;
} WtTransferState;

enum { WT_EXPORT_SPACER_ID=-2 };
static const UiControlSpec wt_edit_control_specs[]={
    {
        .id = WT_EXPORT_SPACER_ID,
        .kind = UI_CTRL_LABEL,
        .label_text = ""
    },
    {
        .id = WORLD_TRANSFER_EXPORT_ID,
        .kind = UI_CTRL_GRID_BUTTON,
        .label_key = "numc3ds.transfer.export_action",
        .label_text = "Export World",
        .width = 118,
        .height = 28,
        .visible = 1,
        .screen_mask = UI_SCREEN_GRID_ITEM
    }
};


/* A transfer is deliberately private module state.  The playable State ABI
 * remains unchanged, so an old host cannot observe half-initialized transfer
 * fields or serialize them into a world. */
typedef struct {
    WtTransferState state;
    u8 source_buffer[WT_IO_CHUNK];
    u8 target_buffer[WT_IO_CHUNK];
} WtTransferRuntime;

static WtTransferRuntime wt_storage;
static WtTransferRuntime *wt_runtime;
#define wt (wt_runtime->state)
#define wt_source_buffer (wt_runtime->source_buffer)
#define wt_target_buffer (wt_runtime->target_buffer)

static void *wt_export_button(void){return wt_runtime?ui_controls_find(&wt.edit_controls,WORLD_TRANSFER_EXPORT_ID):0;}
static volatile u32 wt_summary_enumerating;
static int wt_summary_hooks_ready;

static const char wt_importing[]="Importing worlds";
static const char wt_copying[]="Copying world";
static const char wt_verifying[]="Verifying world";
static const char wt_exporting[]="Exporting world";
static const char wt_replacing[]="Replacing export";
static const char wt_invalid[]="Invalid world skipped";
static const char wt_capacity[]="Not enough storage space";
static const char wt_cancelled[]="Transfer cancelled";
static const char wt_cleanup[]="Cleanup needed on SD card";
static const char wt_complete[]="Transfer complete";
static const char wt_export_label[]="Export World";
static const char wt_replace_label[]="Replace Export";
static const char wt_cancel_label[]="Cancel";

static void wt_trace(const char *marker){if(s&&s->host.debug_string)s->host.debug_string(marker,3);}
static void wt_trace_base(void){char line[10];static const char digits[]="0123456789ABCDEF";u32 value=s?s->host.module_base:0;unsigned i;line[0]='B';for(i=0;i<8;i++)line[1+i]=digits[(value>>((7-i)*4))&15u];line[9]='\n';if(s&&s->host.debug_string)s->host.debug_string(line,sizeof(line));}
static unsigned wt_len(const char *text){unsigned n=0;if(text)while(text[n])n++;return n;}
static int wt_copy(char *out,unsigned cap,const char *text){unsigned n=wt_len(text);if(!out||!cap||n+1>cap)return 0;cp(out,text,n+1);return 1;}
static int wt_append(char *out,unsigned cap,const char *text){unsigned a=wt_len(out),b=wt_len(text);if(a+b+1>cap)return 0;cp(out+a,text,b+1);return 1;}
static int wt_append_uint(char *out,unsigned cap,u32 value){char reverse[12];unsigned n=0,i;do{reverse[n++]=(char)('0'+value%10u);value/=10u;}while(value&&n<sizeof(reverse));for(i=0;i<n;i++){char one[2];one[0]=reverse[n-1-i];one[1]=0;if(!wt_append(out,cap,one))return 0;}return 1;}
static int wt_same(const char *a,const char *b){return streq(a,b);}
static int wt_name_is(const char *name,const char *value){return wt_same(name,value);}
static int wt_path_exists_dir(const char *path){int is_dir=0;return wt_fs_exists(path,&is_dir)&&is_dir;}
static const char *wt_edit_world_text(void *screen,u32 offset){return screen?(const char *)((u8*)screen+offset):"";}

typedef void (*WtListByMapFn)(void *,void *,void *);
typedef unsigned long long (*WtPathSizeFn)(void *);
typedef void (*WtSaveMarkerFn)(void *);

static void wt_on_list_by_map(void *source,void *map,void *list){
    if(wt_summary_vector_populate(list)){
        return;
    }
    u32 prior=wt_summary_enumerating;
    if(!prior)wt_summary_begin();
    wt_summary_enumerating=1;
    ((WtListByMapFn)wt.list_hook.trampoline)(source,map,list);
    wt_summary_enumerating=prior;
    if(!prior)wt_summary_end();
    wt_summary_vector_save(list);
}

static unsigned long long wt_on_path_size(void *path){
    unsigned long long cached,size;
    char world_id[128];
    if(wt_summary_lookup_path(path,&cached))return cached;
    if(wt_summary_extract_world_id(path,world_id,sizeof(world_id))){
        size=wt_summary_calculate_world_size(world_id);
        if(size>0){
            wt_summary_record_path(path,size);
            return size;
        }
    }
    size=((WtPathSizeFn)wt.path_size_hook.trampoline)(path);
    if(size>0)wt_summary_record_path(path,size);
    return size;
}

static void wt_on_save_marker(void *storage){
    ((WtSaveMarkerFn)wt.save_marker_hook.trampoline)(storage);
    wt_thumbnail_on_save_marker(storage);
    /* Invalidate only the world currently being saved so all other worlds
     * retain their 0ms instant cached sizes in memory. */
    if(storage){
        char world_id[128];
        void *path_obj=(void*)((char*)storage+0x28);
        if(wt_summary_extract_world_id(path_obj,world_id,sizeof(world_id))){
            wt_summary_invalidate_world(world_id);
            /* Calculate updated world size and commit to sidecar */
            {
                unsigned long long size = wt_summary_calculate_world_size_force_scan(world_id);
                if(size > 0){
                    wt_meta_write(world_id, size, 1);
                    wt_summary_record_path(path_obj, size);
                }
            }
            return;
        }
    }
    wt_summary_invalidate();
}

typedef int (*WtFileExistsFn)(void *);
typedef u32 (*WtAllocateFileStreamFn)(void *, const char *);

static int wt_on_file_exists(void *path_obj){
    if(wt_summary_enumerating){
        const char *text=wt_summary_string_text(path_obj);
        if(text&&wt_summary_text_suffix(text,"import_lock")){
            if(wt.cleanup_pending&&wt.cleanup_record.final_path[0]){
                if(wt_summary_match_substring(text,wt.cleanup_record.final_path))return 1;
            }
            return 0;
        }
    }
    return ((WtFileExistsFn)wt.file_exists_hook.trampoline)(path_obj);
}

static u32 wt_on_allocate_file_stream(void *file_mgr,const char *path){
    if(wt_summary_enumerating&&path&&wt_summary_text_is_resource_pack(path)){
        return 0xFFFFFFFFu;
    }
    return ((WtAllocateFileStreamFn)wt.stream_hook.trampoline)(file_mgr,path);
}

static void wt_set_message(const char *message){
    const char *key=0;
    char localized[96];
    if(message==wt_importing)key="numc3ds.transfer.import";
    else if(message==wt_copying)key="numc3ds.transfer.copying";
    else if(message==wt_verifying)key="numc3ds.transfer.verifying";
    else if(message==wt_exporting)key="numc3ds.transfer.export";
    else if(message==wt_replacing)key="numc3ds.transfer.replacing";
    else if(message==wt_invalid)key="numc3ds.transfer.invalid";
    else if(message==wt_capacity)key="numc3ds.transfer.capacity";
    else if(message==wt_cancelled)key="numc3ds.transfer.cancelled";
    else if(message==wt_cleanup)key="numc3ds.transfer.cleanup";
    else if(message==wt_complete)key="numc3ds.transfer.complete";
    if(key){ui_localized_label(localized,sizeof(localized),key,message);wt_copy(wt.message,sizeof(wt.message),localized);}
    else wt_copy(wt.message,sizeof(wt.message),message);
}

static int wt_compare_names(const char *a,const char *b){unsigned i=0;while(a[i]&&b[i]&&a[i]==b[i])i++;return (unsigned char)a[i]<(unsigned char)b[i]?-1:((unsigned char)a[i]>(unsigned char)b[i]?1:0);}
static void wt_sort_candidates(void){u32 i,j;WtCandidate saved;for(i=1;i<wt.candidate_count;i++){cp(&saved,&wt.candidates[i],sizeof(saved));j=i;while(j&&wt_compare_names(wt.candidates[j-1].name,saved.name)>0){cp(&wt.candidates[j],&wt.candidates[j-1],sizeof(saved));j--;}cp(&wt.candidates[j],&saved,sizeof(saved));}}
static void wt_manifest_copy(WtTransferManifest *out,const WtTransferManifest *in){if(out&&in)cp(out,in,sizeof(*out));}

static int wt_walk_push(const char *source,const char *target,const char *relative){WtWalkFrame *frame;if(wt.walk.depth>=WT_MAX_WALK_DEPTH)return 0;frame=&wt.walk.frames[wt.walk.depth++];zero(frame,sizeof(*frame));if(!wt_copy(frame->source,sizeof(frame->source),source)||!wt_copy(frame->target,sizeof(frame->target),target)||!wt_copy(frame->relative,sizeof(frame->relative),relative))return 0;if(!wt_fs_open_directory(source,&frame->directory)){wt.walk.depth--;return 0;}return 1;}
static void wt_walk_pop(void){if(wt.walk.depth){WtWalkFrame *frame=&wt.walk.frames[wt.walk.depth-1];wt_fs_close_directory(&frame->directory);wt.walk.depth--;}}
static void wt_walk_close_file(void){if(wt.walk.file.source.handle)wt_fs_close_file(&wt.walk.file.source);if(wt.walk.file.target.handle)wt_fs_close_file(&wt.walk.file.target);zero(&wt.walk.file,sizeof(wt.walk.file));}
static void wt_walk_abort(void){while(wt.walk.depth)wt_walk_pop();wt_walk_close_file();wt.walk.active=0;}

static int wt_walk_begin(int mode,const char *source,const char *target){
    int is_dir;
    wt_walk_abort();
    zero(&wt.walk,sizeof(wt.walk));
    wt.walk.mode=mode;
    wt.walk.active=1;
    wt_manifest_reset(&wt.walk.manifest);
    if(!wt_copy(wt.walk.root_source,sizeof(wt.walk.root_source),source)||!wt_copy(wt.walk.root_target,sizeof(wt.walk.root_target),target)){wt.walk.active=0;return 0;}
    /* The root itself is not emitted by a directory enumeration.  Create it
     * before the first root-level file is opened, and require it during every
     * read-back pass. */
    if(mode==WT_WALK_COPY){
        if(!wt_fs_exists(target,&is_dir)){if(!wt_fs_create_directory(target)){wt.walk.active=0;return 0;}}
        else if(!is_dir){wt.walk.active=0;return 0;}
    } else if(mode==WT_WALK_VERIFY_SOURCE||mode==WT_WALK_FINAL){
        if(!wt_path_exists_dir(target)){wt.walk.active=0;return 0;}
    }
    if(!wt_walk_push(source,target,"")){wt.walk.active=0;return 0;}
    return 1;
}
static int wt_walk_is_import_lock(const char *name){return wt_name_is(name,"import_lock");}

static int wt_walk_load(WtWalkFrame *frame){int count,i;char name[WT_FS_MAX_PATH];if(frame->loaded)return 1;if(!wt_fs_read_directory(&frame->directory,frame->entries,WT_FS_DIRECTORY_BATCH,&count))return 0;if(count==0){frame->count=0;frame->loaded=1;return 1;}frame->count=count;frame->index=0;frame->loaded=1;for(i=0;i<count;i++){if(!wt_fs_entry_name_utf8(&frame->entries[i],name,sizeof(name)))return 0;if(wt_walk_is_import_lock(name)){wt.walk.error=2;return 0;}}return 1;}

static int wt_walk_begin_file(WtWalkFrame *frame,WtFsDirectoryEntry *entry,const char *name){WtWalkFile *file=&wt.walk.file;char source[WT_FS_MAX_PATH],target[WT_FS_MAX_PATH],relative[WT_FS_MAX_PATH];long long size=wt_fs_entry_size(entry);if(!wt_fs_join(source,sizeof(source),frame->source,name)||!wt_fs_join(relative,sizeof(relative),frame->relative,name))return 0;if(wt.walk.mode==WT_WALK_METADATA){wt_manifest_add_file(&wt.walk.manifest,relative,size,0);return 1;}if(wt.walk.mode==WT_WALK_TARGET){if(!wt_copy(target,sizeof(target),source))return 0;}else if(!wt_fs_join(target,sizeof(target),frame->target,name))return 0;zero(file,sizeof(*file));file->mode=wt.walk.mode;file->size=size;file->source_hash=2166136261u;file->target_hash=2166136261u;if(!wt_copy(file->source_path,sizeof(file->source_path),source)||!wt_copy(file->target_path,sizeof(file->target_path),target)||!wt_copy(file->relative,sizeof(file->relative),relative))return 0;if(!wt_fs_open_file(source,0,&file->source))return 0;if(wt.walk.mode==WT_WALK_COPY){if(!wt_fs_create_file(target,size)||!wt_fs_open_file(target,1,&file->target)){wt_walk_close_file();return 0;}}else if(wt.walk.mode==WT_WALK_VERIFY_SOURCE||wt.walk.mode==WT_WALK_FINAL){if(!wt_fs_open_file(target,0,&file->target)){wt_walk_close_file();return 0;}}file->active=1;return 1;}

static int wt_walk_finish_file(void){WtWalkFile *file=&wt.walk.file;if(wt.walk.mode==WT_WALK_COPY||wt.walk.mode==WT_WALK_VERIFY_SOURCE||wt.walk.mode==WT_WALK_FINAL)wt_manifest_add_file(&wt.walk.manifest,file->relative,file->size,file->source_hash);else if(wt.walk.mode==WT_WALK_TARGET)wt_manifest_add_file(&wt.walk.manifest,file->relative,file->size,file->source_hash);wt_walk_close_file();return 1;}
static int wt_walk_file_step(void){WtWalkFile *file=&wt.walk.file;u32 want,got_source=0,got_target=0,i;if(file->size==0)return wt_walk_finish_file();want=(u32)((long long)WT_IO_CHUNK<file->size-file->offset?(long long)WT_IO_CHUNK:file->size-file->offset);if(!wt_fs_read(&file->source,file->offset,wt_source_buffer,want,&got_source)||got_source!=want)return 0;if(file->mode==WT_WALK_COPY){if(!wt_fs_write(&file->target,file->offset,wt_source_buffer,want,&got_target)||got_target!=want)return 0;file->source_hash=wt_manifest_bytes_hash(file->source_hash,wt_source_buffer,want);}else if(file->mode==WT_WALK_VERIFY_SOURCE||file->mode==WT_WALK_FINAL){if(!wt_fs_read(&file->target,file->offset,wt_target_buffer,want,&got_target)||got_target!=want)return 0;for(i=0;i<want;i++)if(wt_source_buffer[i]!=wt_target_buffer[i])return 0;file->source_hash=wt_manifest_bytes_hash(file->source_hash,wt_source_buffer,want);file->target_hash=wt_manifest_bytes_hash(file->target_hash,wt_target_buffer,want);}else{file->source_hash=wt_manifest_bytes_hash(file->source_hash,wt_source_buffer,want);}file->offset+=want;if(file->offset>=file->size)return wt_walk_finish_file();return 1;}

/* Returns 1 when the tree is complete, 0 when more bounded work is needed,
 * and -1 on a hard filesystem/content error. */
static int wt_walk_step(void){WtWalkFrame *frame;WtFsDirectoryEntry *entry;char name[WT_FS_MAX_PATH],child_source[WT_FS_MAX_PATH],child_target[WT_FS_MAX_PATH],child_relative[WT_FS_MAX_PATH];if(!wt.walk.active)return -1;if(wt.walk.file.active){if(!wt_walk_file_step()){wt.walk.error=1;return -1;}return wt.walk.active?0:1;}if(wt.walk.depth==0){wt.walk.active=0;return 1;}frame=&wt.walk.frames[wt.walk.depth-1];if(!wt_walk_load(frame)){wt.walk.error=1;return -1;}if(frame->count==0){wt_walk_pop();if(wt.walk.depth==0){wt.walk.active=0;return 1;}return 0;}if(frame->index>=frame->count){frame->loaded=0;frame->count=0;return 0;}entry=&frame->entries[frame->index++];if(!wt_fs_entry_name_utf8(entry,name,sizeof(name))||!wt_fs_validate_relative(name)){wt.walk.error=2;return -1;}if(!wt_fs_join(child_source,sizeof(child_source),frame->source,name)||!wt_fs_join(child_relative,sizeof(child_relative),frame->relative,name))return -1;if(wt_fs_entry_is_directory(entry)){if(wt.walk.mode==WT_WALK_COPY){if(!wt_fs_join(child_target,sizeof(child_target),frame->target,name))return -1;if(!wt_fs_create_directory(child_target)&&!wt_path_exists_dir(child_target))return -1;}else if(wt.walk.mode==WT_WALK_VERIFY_SOURCE||wt.walk.mode==WT_WALK_FINAL){if(!wt_fs_join(child_target,sizeof(child_target),frame->target,name)||!wt_path_exists_dir(child_target))return -1;}else child_target[0]=0;wt_manifest_add_directory(&wt.walk.manifest,child_relative);if(!wt_walk_push(child_source,child_target,child_relative)){wt.walk.error=1;return -1;}return 0;}if(!wt_walk_begin_file(frame,entry,name)){wt.walk.error=1;return -1;}return 0;}

static int wt_direct_candidate_valid(const char *path){WtFsDirectory directory;WtFsDirectoryEntry entries[WT_FS_DIRECTORY_BATCH];int count,i;char name[WT_FS_MAX_PATH];char is_dir;if(!wt_fs_open_directory(path,&directory))return 0;for(;;){if(!wt_fs_read_directory(&directory,entries,WT_FS_DIRECTORY_BATCH,&count)){wt_fs_close_directory(&directory);return 0;}if(count==0)break;for(i=0;i<count;i++){if(!wt_fs_entry_name_utf8(&entries[i],name,sizeof(name))){wt_fs_close_directory(&directory);return 0;}if(wt_name_is(name,"import_lock")){wt_fs_close_directory(&directory);return 0;}if(wt_name_is(name,"level.dat")){is_dir=wt_fs_entry_is_directory(&entries[i]);if(!is_dir&&wt_fs_entry_size(&entries[i])>0){wt_fs_close_directory(&directory);return 1;}}}}wt_fs_close_directory(&directory);return 0;}

static int wt_scan_candidates(void){WtFsDirectory directory;WtFsDirectoryEntry entries[WT_FS_DIRECTORY_BATCH];int count,i;char name[WT_FS_MAX_PATH],path[WT_FS_MAX_PATH];wt.candidate_count=0;if(!wt_fs_open_directory("sdmc:/Minecraft 3DS/ImportedWorlds",&directory))return 0;for(;;){if(!wt_fs_read_directory(&directory,entries,WT_FS_DIRECTORY_BATCH,&count)){wt_fs_close_directory(&directory);return -1;}if(count==0)break;for(i=0;i<count&&wt.candidate_count<WT_MAX_CANDIDATES;i++){if(!wt_fs_entry_is_directory(&entries[i])||!wt_fs_entry_name_utf8(&entries[i],name,sizeof(name))||!wt_fs_validate_relative(name)||wt_name_is(name,"import_lock"))continue;if(!wt_fs_join(path,sizeof(path),"sdmc:/Minecraft 3DS/ImportedWorlds",name))continue;if(wt_direct_candidate_valid(path)){wt_copy(wt.candidates[wt.candidate_count].name,sizeof(wt.candidates[wt.candidate_count].name),name);wt.candidate_count++;}}}wt_fs_close_directory(&directory);wt_sort_candidates();return (int)wt.candidate_count;}

static int wt_capacity_check(const char *archive_path,long long required){long long free_bytes=0;const long long reserve=65536;if(required<0)return 0;/* Azahar currently stubs FSUSER_GetArchiveResource.  An unavailable query
 is not evidence that the archive is full; the checked staging creates/writes
 remain the authoritative failure boundary and cannot expose a partial world. */if(!wt_fs_get_free_bytes(archive_path,&free_bytes)||free_bytes<0)return 1;return free_bytes>=required+reserve;}

static int wt_make_transaction_path(char *out,unsigned cap,const char *prefix,const char *kind,u32 id){if(!wt_copy(out,cap,prefix)||!wt_append(out,cap,"/")||!wt_append(out,cap,kind)||!wt_append(out,cap,"-")||!wt_append_uint(out,cap,id))return 0;return 1;}
static int wt_choose_internal_target(const char *source_name,char *out,unsigned cap){char clean[WT_FS_MAX_PATH],base[WT_FS_MAX_PATH];unsigned suffix;int exists;wt_sanitize_name(source_name,clean,sizeof(clean));if(!wt_fs_join(base,sizeof(base),"extdata:/minecraftWorlds",clean))return 0;if(!wt_fs_exists(base,&exists)){wt_copy(out,cap,base);return 1;}for(suffix=2;suffix<100;suffix++){if(!wt_copy(out,cap,base)||!wt_append(out,cap,"-")||!wt_append_uint(out,cap,suffix))return 0;if(!wt_fs_exists(out,&exists))return 1;}return 0;}

static int wt_validate_staged_level(const char *path){u32 level_data[SEAM_LevelData_size/sizeof(u32)],level_path=0,scratch=0;int result;/* The stock reader accepts a world directory and appends level.dat (with
 * old/new fallbacks) itself.  LevelData owns strings/tags, so its real
 * constructor must run before either the parser or destructor touches it. */((void*(*)(void*))SEAM_LevelData_ctor)(level_data);((StrCtor)SEAM_StrCtor)(&level_path,path,&scratch);result=((int(*)(u32*,void*))SEAM_ExternalFileLevelStorage_readLevelDataFromFile)(&level_path,level_data);((ThisFn)SEAM_LevelData_dtor)(level_data);((StrDtor)SEAM_StrDtor)(&level_path);return result!=0;}


typedef void *(*WtCallbackTableGetFn)(int);
typedef void (*WtBackgroundWorkerSyncFn)(void *);
static int wt_sync_file_worker(void){
    void *worker=((WtCallbackTableGetFn)SEAM_FrameworkCallbackTable_getEntry)(1);
    /* This is the same callback-table entry used by the stock world-load
     * helper immediately before it opens the selected world's files.  Do not
     * substitute an EditWorld field guessed from a decompilation. */
    if(!worker)return 0;
    ((WtBackgroundWorkerSyncFn)SEAM_BackGroundWorker_sync)(worker);
    return 1;
}

typedef void *(*WtTaskConstructor)(void *,void *,int,void *);
typedef void (*WtPushProgress)(void *,const u32 *,const u32 *,void **);
static int wt_push_progress(void *chooser){void *game,*task,*slot;u32 operation=0,message=0,operation_scratch=0,message_scratch=0;GameAllocWithSelector alloc=(GameAllocWithSelector)SEAM_Heap_allocWithSelector;if(!chooser)return 0;game=*(void**)chooser;task=alloc(0x14,(void*)SEAM_game_alloc_selector);if(!task)return 0;((WtTaskConstructor)SEAM_ProgressScreenTask_constructor)(task,game,1,0);slot=task;((StrCtor)SEAM_StrCtor)(&operation,wt.operation==WT_JOURNAL_IMPORT?"numc3ds.transfer.import":"numc3ds.transfer.export",&operation_scratch);((StrCtor)SEAM_StrCtor)(&message,wt.message,&message_scratch);/* Both text arguments are const gstd::string&, not by-value character
 pointers.  Passing the string payload made ProgressScreen treat the first
 four ASCII bytes as a reference-counted string object. */((WtPushProgress)SEAM_ScreenChooser_pushProgressScreen)(chooser,&operation,&message,&slot);drop_str(&operation);drop_str(&message);if(slot)release_obj(slot);return 1;}

static void wt_schedule_close(void){void *chooser=wt.chooser;if(wt.closing)return;/* Give the final message and 100% bar time to render instead of popping the
 * progress screen in the same update that completes verification. */if(wt.phase==WT_PHASE_DONE&&wt.completion_hold==0){wt.completion_hold=90;return;}wt.closing=1;if(chooser)((void(*)(void*,int))SEAM_ScreenChooser_schedulePopScreen)(chooser,1);}

static int wt_start_import_item(void){
    char source[WT_FS_MAX_PATH],final_path[WT_FS_MAX_PATH];
    const char *name;
    if(wt.candidate_index>=wt.candidate_count){wt_set_message(wt_complete);wt.phase=WT_PHASE_DONE;wt_schedule_close();return 1;}
    name=wt.candidates[wt.candidate_index].name;
    if(!wt_fs_join(source,sizeof(source),"sdmc:/Minecraft 3DS/ImportedWorlds",name)||!wt_choose_internal_target(name,final_path,sizeof(final_path))||!wt_make_transaction_path(wt.stage,sizeof(wt.stage),"extdata:/numc3ds-transfer","import",++wt.next_transaction)){wt_set_message(wt_invalid);wt.phase=WT_PHASE_FAILED;wt_schedule_close();return 0;}
    wt_copy(wt.public_source,sizeof(wt.public_source),source);
    wt_copy(wt.internal_source,sizeof(wt.internal_source),source);
    wt_copy(wt.final_path,sizeof(wt.final_path),final_path);
    zero(wt.backup,sizeof(wt.backup));
    wt_journal_reset(&wt.journal,WT_JOURNAL_IMPORT,wt.next_transaction);
    wt_copy(wt.journal.source,sizeof(wt.journal.source),source);
    wt_copy(wt.journal.staging,sizeof(wt.journal.staging),wt.stage);
    wt_copy(wt.journal.final_path,sizeof(wt.journal.final_path),final_path);
    if(!wt_journal_write(&wt.journal)||!wt_walk_begin(WT_WALK_METADATA,source,"")){wt_set_message(wt_invalid);wt.phase=WT_PHASE_FAILED;wt_schedule_close();return 0;}
    wt.phase=WT_PHASE_IMPORT_PREPARE;
    wt_set_message(wt_verifying);
    return 1;
}

static void wt_remove_cleanup_candidate(void){u32 i;for(i=0;i<wt.candidate_count;i++){char source[WT_FS_MAX_PATH];if(!wt_fs_join(source,sizeof(source),"sdmc:/Minecraft 3DS/ImportedWorlds",wt.candidates[i].name))continue;if(streq(source,wt.cleanup_record.source)){for(;i+1<wt.candidate_count;i++)cp(&wt.candidates[i],&wt.candidates[i+1],sizeof(wt.candidates[i]));wt.candidate_count--;break;}}}
static int wt_start_cleanup_retry(void){if(!wt.cleanup_pending)return 0;if(!wt_validate_staged_level(wt.cleanup_record.final_path)){wt_trace("Q8\n");wt.cleanup_pending=0;wt_set_message(wt_cleanup);wt.phase=WT_PHASE_FAILED;wt_schedule_close();return 0;}if(wt_fs_delete_tree(wt.cleanup_record.source)){wt_trace("QA\n");wt.journal.cleanup_state=0;wt_journal_clear(WT_JOURNAL_IMPORT);wt_remove_cleanup_candidate();wt_summary_invalidate();wt_set_message(wt_complete);wt.cleanup_pending=0;wt.phase=WT_PHASE_DONE;wt_schedule_close();return 1;}wt_trace("QB\n");wt.cleanup_pending=0;wt_set_message(wt_cleanup);wt.phase=WT_PHASE_FAILED;wt_schedule_close();return 0;}

static int wt_delete_import_stage(void){if(!wt.stage[0])return 1;return wt_fs_delete_tree(wt.stage);}
static void wt_skip_import_item(const char *message){if(!wt_delete_import_stage()){wt.journal.phase=WT_JOURNAL_CLEANUP;wt.journal.cleanup_state=1;wt_journal_write(&wt.journal);wt_set_message(wt_cleanup);wt.phase=WT_PHASE_FAILED;wt_schedule_close();return;}wt_journal_clear(WT_JOURNAL_IMPORT);if(message)wt_set_message(message);wt.candidate_index++;wt.phase=WT_PHASE_IDLE;}
static int wt_parse_import(void){if(!wt_validate_staged_level(wt.stage)){wt_skip_import_item(wt_invalid);return 0;}return 1;}

static int wt_restore_export_backup(void){int final_is_dir;if(!wt.backup[0]||!wt_path_exists_dir(wt.backup))return 1;if(wt.final_path[0]&&wt_fs_exists(wt.final_path,&final_is_dir)){if(!wt_fs_delete_tree(wt.final_path)&&wt_fs_exists(wt.final_path,&final_is_dir))return 0;}return wt_fs_rename(wt.backup,wt.final_path);}
static void wt_fail_export(const char *message){int stage_clean,restored;wt_walk_abort();stage_clean=!wt.stage[0]||wt_fs_delete_tree(wt.stage);restored=wt_restore_export_backup();if(restored&&stage_clean)wt_journal_clear(WT_JOURNAL_EXPORT);else {wt.journal.phase=WT_JOURNAL_CLEANUP;wt.journal.cleanup_state=1;wt_journal_write(&wt.journal);}wt_set_message(restored&&stage_clean?message:wt_cleanup);wt.phase=WT_PHASE_FAILED;wt_schedule_close();if(!wt.progress_screen){wt.active=0;wt.phase=WT_PHASE_IDLE;wt.closing=0;}}
static void wt_fail_import_after_commit(const char *message){int final_is_dir;wt_walk_abort();wt.journal.phase=WT_JOURNAL_CLEANUP;wt.journal.cleanup_state=2;wt_journal_write(&wt.journal);if(wt_fs_exists(wt.final_path,&final_is_dir)&&wt_fs_delete_tree(wt.final_path))wt_journal_clear(WT_JOURNAL_IMPORT);wt_set_message(message);wt.phase=WT_PHASE_FAILED;wt_schedule_close();}
static void wt_finalize_import(void);
static int wt_commit_import(void){if(!wt_parse_import())return 0;wt.phase=WT_PHASE_IMPORT_COMMIT;wt.journal.phase=WT_JOURNAL_COMMIT;wt_journal_write(&wt.journal);if(!wt_fs_rename(wt.stage,wt.final_path)){wt_skip_import_item(wt_invalid);return 0;}/* Staging was already read back and compared to the bytes fingerprinted during
 copy.  An intra-archive rename does not rewrite file contents. */wt_finalize_import();return 1;}
static void wt_finalize_import(void){wt.journal.phase=WT_JOURNAL_CLEANUP;wt.journal.cleanup_state=1;wt_manifest_fingerprint_text(&wt.copied_manifest,wt.journal.source_fingerprint,sizeof(wt.journal.source_fingerprint));wt_journal_write(&wt.journal);if(!wt_fs_delete_tree(wt.public_source)){wt_set_message(wt_cleanup);wt.phase=WT_PHASE_FAILED;wt_schedule_close();return;}wt.journal.cleanup_state=0;wt_journal_clear(WT_JOURNAL_IMPORT);wt_summary_invalidate();wt.imported_count++;wt.candidate_index++;wt_set_message(wt_complete);wt.phase=WT_PHASE_DONE;wt_schedule_close();}

static int wt_start_export(void *screen){
    void *game;const char *world_id,*display_name;int exists,is_dir;char clean[WT_FS_MAX_PATH];
    if(!screen||wt.active)return 0;
    world_id=wt_edit_world_text(screen,SEAM_EditWorldScreen_worldNameOffset);
    display_name=wt_edit_world_text(screen,SEAM_EditWorldScreen_displayNameOffset);
    if(!world_id||!*world_id)return 0;
    if(!wt_fs_validate_relative(world_id)||!wt_sync_file_worker()||!wt_fs_ensure_public_roots())return 0;
    if(!wt_fs_join(wt.internal_source,sizeof(wt.internal_source),"extdata:/minecraftWorlds",world_id)||!wt_path_exists_dir(wt.internal_source))return 0;
    wt_sanitize_name(display_name,clean,sizeof(clean));
    if(!wt_fs_join(wt.final_path,sizeof(wt.final_path),"sdmc:/Minecraft 3DS/ExportedWorlds",clean))return 0;
    exists=wt_fs_exists(wt.final_path,&is_dir);
    if(exists&&!is_dir)return 0;
    if(exists&&!wt.replace)return 0;
    game=ui_screen_game(screen);
    wt.chooser=game?*(void**)((u8*)game+SEAM_MinecraftGame_screenChooserOffset):0;
    wt.operation=WT_JOURNAL_EXPORT;wt.active=1;wt.closing=0;wt.cancel_requested=0;wt.pending_reopen=0;wt.completion_hold=0;wt.edit_screen=screen;wt.public_source[0]=0;wt.backup[0]=0;wt.next_transaction++;
    if(!wt_make_transaction_path(wt.stage,sizeof(wt.stage),"sdmc:/Minecraft 3DS/.numc3ds-transfer","export",wt.next_transaction)){wt.active=0;return 0;}
    if(exists&&wt.replace&&!wt_make_transaction_path(wt.backup,sizeof(wt.backup),"sdmc:/Minecraft 3DS/.numc3ds-transfer","export-backup",wt.next_transaction)){wt.active=0;return 0;}
    wt_journal_reset(&wt.journal,WT_JOURNAL_EXPORT,wt.next_transaction);
    wt_copy(wt.journal.source,sizeof(wt.journal.source),wt.internal_source);wt_copy(wt.journal.staging,sizeof(wt.journal.staging),wt.stage);wt_copy(wt.journal.final_path,sizeof(wt.journal.final_path),wt.final_path);wt_copy(wt.journal.backup,sizeof(wt.journal.backup),wt.backup);
    if(!wt_journal_write(&wt.journal)||!wt_walk_begin(WT_WALK_METADATA,wt.internal_source,"")){wt_fail_export(wt_invalid);return 0;}
    wt.phase=WT_PHASE_EXPORT_PREPARE;wt_set_message(wt_verifying);
    if(!wt_push_progress(wt.chooser)){wt_fail_export(wt_invalid);return 0;}
    return 1;
}

static void wt_finalize_export(void);
static int wt_commit_export(void){if(wt.replace){if(!wt_fs_rename(wt.final_path,wt.backup)){wt_fail_export(wt_invalid);return 0;}if(!wt_fs_rename(wt.stage,wt.final_path)){wt_restore_export_backup();wt_fail_export(wt_invalid);return 0;}}else if(!wt_fs_rename(wt.stage,wt.final_path)){wt_fail_export(wt_invalid);return 0;}wt.journal.phase=WT_JOURNAL_COMMIT;if(!wt_journal_write(&wt.journal)){wt_fail_export(wt_invalid);return 0;}wt_finalize_export();return 1;}
static void wt_finalize_export(void){int cleanup_ok=1;if(wt.backup[0]&&!wt_fs_delete_tree(wt.backup)){wt.journal.cleanup_state=1;wt.journal.phase=WT_JOURNAL_CLEANUP;wt_journal_write(&wt.journal);wt_set_message(wt_cleanup);cleanup_ok=0;}else wt_journal_clear(WT_JOURNAL_EXPORT);if(cleanup_ok)wt_set_message(wt_complete);wt.phase=WT_PHASE_DONE;wt_schedule_close();}

static void wt_cancel_now(void){int stage_clean=1,backup_clean=1;if(!wt.active||wt.phase==WT_PHASE_IMPORT_COMMIT||wt.phase==WT_PHASE_IMPORT_FINAL_VERIFY||wt.phase==WT_PHASE_EXPORT_COMMIT||wt.phase==WT_PHASE_EXPORT_FINAL_VERIFY)return;wt_walk_abort();if(wt.stage[0])stage_clean=wt_fs_delete_tree(wt.stage);if(wt.operation==WT_JOURNAL_EXPORT)backup_clean=wt_restore_export_backup();if(wt.phase!=WT_PHASE_IMPORT_CLEANUP_RETRY&&stage_clean&&backup_clean)wt_journal_clear(wt.operation);else if(wt.phase!=WT_PHASE_IMPORT_CLEANUP_RETRY){wt.journal.phase=WT_JOURNAL_CLEANUP;wt.journal.cleanup_state=1;wt_journal_write(&wt.journal);}wt.cancel_requested=0;wt_set_message(stage_clean&&backup_clean?wt_cancelled:wt_cleanup);wt.phase=WT_PHASE_FAILED;wt_schedule_close();}

static void wt_tick_active(void){
    int step;
    switch(wt.phase){
    case WT_PHASE_IDLE:
        if(wt.operation==WT_JOURNAL_IMPORT){if(wt.cleanup_pending)wt_start_cleanup_retry();else wt_start_import_item();}
        else wt.phase=WT_PHASE_EXPORT_PREPARE;
        break;
    case WT_PHASE_IMPORT_PREPARE:
        step=wt_walk_step();
        if(step<0){wt_skip_import_item(wt_invalid);}
        else if(step>0){
            wt_manifest_copy(&wt.prepared_manifest,&wt.walk.manifest);
            if(!wt_capacity_check("extdata:/minecraftWorlds",wt.prepared_manifest.bytes)){wt_skip_import_item(wt_capacity);break;}
            wt_manifest_copy(&wt.journal.manifest,&wt.prepared_manifest);wt.journal.phase=WT_JOURNAL_COPY;wt_journal_write(&wt.journal);
            if(!wt_walk_begin(WT_WALK_COPY,wt.public_source,wt.stage)){wt_set_message(wt_invalid);wt.phase=WT_PHASE_FAILED;wt_schedule_close();}
            else {wt.phase=WT_PHASE_IMPORT_COPY;wt_set_message(wt_copying);}
        }
        break;
    case WT_PHASE_IMPORT_COPY:
        step=wt_walk_step();
        if(step<0){if(wt.walk.error==2)wt_skip_import_item(wt_invalid);else {wt_set_message(wt_invalid);wt.phase=WT_PHASE_FAILED;wt_schedule_close();}}
        else if(step>0){
            wt_manifest_copy(&wt.copied_manifest,&wt.walk.manifest);wt_manifest_copy(&wt.journal.manifest,&wt.copied_manifest);wt.journal.phase=WT_JOURNAL_VERIFY;wt_journal_write(&wt.journal);
            if(!wt_walk_begin(WT_WALK_VERIFY_SOURCE,wt.public_source,wt.stage)){wt_set_message(wt_invalid);wt.phase=WT_PHASE_FAILED;wt_schedule_close();}
            else {wt.phase=WT_PHASE_IMPORT_VERIFY_SOURCE;wt_set_message(wt_verifying);}
        }
        break;
    case WT_PHASE_IMPORT_VERIFY_SOURCE:
        step=wt_walk_step();
        if(step<0){wt_set_message(wt_invalid);wt.phase=WT_PHASE_FAILED;wt_schedule_close();}
        else if(step>0){
            if(!wt_manifest_equal(&wt.copied_manifest,&wt.walk.manifest)){wt_set_message(wt_invalid);wt.phase=WT_PHASE_FAILED;wt_schedule_close();}
            else if(!wt_walk_begin(WT_WALK_TARGET,wt.stage,"")){wt_set_message(wt_invalid);wt.phase=WT_PHASE_FAILED;wt_schedule_close();}
            else wt.phase=WT_PHASE_IMPORT_VERIFY_TARGET;
        }
        break;
    case WT_PHASE_IMPORT_VERIFY_TARGET:
        step=wt_walk_step();
        if(step<0){wt_set_message(wt_invalid);wt.phase=WT_PHASE_FAILED;wt_schedule_close();}
        else if(step>0){
            if(!wt_manifest_equal(&wt.copied_manifest,&wt.walk.manifest)){wt_set_message(wt_invalid);wt.phase=WT_PHASE_FAILED;wt_schedule_close();}
            else {wt.phase=WT_PHASE_IMPORT_PARSE;wt_set_message(wt_verifying);wt_commit_import();}
        }
        break;
    case WT_PHASE_IMPORT_FINAL_VERIFY:
        step=wt_walk_step();
        if(step<0){wt_fail_import_after_commit(wt_invalid);}
        else if(step>0)wt_finalize_import();
        break;
    case WT_PHASE_EXPORT_PREPARE:
        step=wt_walk_step();
        if(step<0)wt_fail_export(wt_invalid);
        else if(step>0){
            wt_manifest_copy(&wt.prepared_manifest,&wt.walk.manifest);
            if(!wt_capacity_check("sdmc:/Minecraft 3DS/ExportedWorlds",wt.prepared_manifest.bytes)){wt_fail_export(wt_capacity);break;}
            wt_manifest_copy(&wt.journal.manifest,&wt.prepared_manifest);wt.journal.phase=WT_JOURNAL_COPY;wt_journal_write(&wt.journal);
            if(!wt_walk_begin(WT_WALK_COPY,wt.internal_source,wt.stage))wt_fail_export(wt_invalid);
            else {wt.phase=WT_PHASE_EXPORT_COPY;wt_set_message(wt.replace?wt_replacing:wt_exporting);}
        }
        break;
    case WT_PHASE_EXPORT_COPY:
        step=wt_walk_step();
        if(step<0)wt_fail_export(wt_invalid);
        else if(step>0){
            wt_manifest_copy(&wt.copied_manifest,&wt.walk.manifest);wt_manifest_copy(&wt.journal.manifest,&wt.copied_manifest);wt.phase=WT_PHASE_EXPORT_COMMIT;wt_set_message(wt.replace?wt_replacing:wt_exporting);wt_commit_export();
        }
        break;
    default:break;
    }
}

static int wt_install_summary_hooks(void);
static int wt_prepare_cleanup_receipt(void){WtTransferJournalRecord record;u32 i;int is_dir;wt.cleanup_pending=0;zero(&wt.cleanup_record,sizeof(wt.cleanup_record));if(!wt_journal_read(WT_JOURNAL_IMPORT,&record)||record.cleanup_state!=1||!record.source[0]||!record.final_path[0]){wt_trace("Q0\n");return 0;}if(!wt_fs_exists(record.final_path,&is_dir)||!is_dir){wt_trace("Q2\n");return 0;}for(i=0;i<wt.candidate_count;i++){char source[WT_FS_MAX_PATH];if(wt_fs_join(source,sizeof(source),"sdmc:/Minecraft 3DS/ImportedWorlds",wt.candidates[i].name)&&streq(source,record.source)){cp(&wt.cleanup_record,&record,sizeof(record));wt.cleanup_pending=1;wt_trace("Q3\n");return 1;}}wt_trace("Q4\n");return 0;}
static void wt_on_world_selection_open(void *chooser,void *unused,int tab_index){int found;if(wt.active)return;wt.chooser=chooser;wt.route_unused=unused;wt.tab_index=tab_index;if(wt_summary_vector_valid()){((void(*)(void*,void*,int))wt.route_hook.trampoline)(wt.chooser,wt.route_unused,wt.tab_index);return;}if(!wt_fs_mount_sdmc()){((void(*)(void*,void*,int))wt.route_hook.trampoline)(wt.chooser,wt.route_unused,wt.tab_index);return;}wt_journal_recover();found=wt_scan_candidates();if(found<=0){((void(*)(void*,void*,int))wt.route_hook.trampoline)(wt.chooser,wt.route_unused,wt.tab_index);return;}wt_journal_recover();wt.operation=WT_JOURNAL_IMPORT;wt.active=1;wt.closing=0;wt.cancel_requested=0;wt.pending_reopen=1;wt.completion_hold=0;wt.replace=0;wt.replace_armed=0;wt.candidate_index=0;wt.imported_count=0;wt.next_transaction++;wt_set_message(wt_importing);wt_prepare_cleanup_receipt();if(!wt_push_progress(wt.chooser)){wt.active=0;((void(*)(void*,void*,int))wt.route_hook.trampoline)(wt.chooser,wt.route_unused,wt.tab_index);return;}wt.phase=WT_PHASE_IDLE;}

static int wt_prepare_hook(NuMC3DS_Hook *hook,u32 target,u32 replacement,u32 first,u32 second){if(!hook)return 0;zero(hook,sizeof(*hook));hook->target=target;hook->replacement=replacement;hook->expected[0]=first;hook->expected[1]=second;return s->host.install_hook(hook)==0;}
static void wt_remove_hook(NuMC3DS_Hook *hook){if(hook&&hook->trampoline&&s->host.remove_hook)s->host.remove_hook(hook);if(hook)zero(hook,sizeof(*hook));}
static void wt_remove_summary_hooks(void){
    wt_remove_hook(&wt.stream_hook);
    wt_remove_hook(&wt.file_exists_hook);
    wt_remove_hook(&wt.save_marker_hook);
    wt_remove_hook(&wt.path_size_hook);
    wt_remove_hook(&wt.list_hook);
    wt_summary_hooks_ready=0;
}
static int wt_install_summary_hooks(void){
    int list_ok=0,size_ok=0,save_ok=0,exists_ok=0,stream_ok=0;
    if(!s->host.remove_hook)return 0;
    list_ok=wt_prepare_hook(&wt.list_hook,SEAM_ExternalFileLevelStorageSource_getLevelListByMap,(u32)wt_on_list_by_map,0xE92D4070u,0xE1A04001u);
    if(!list_ok)goto fail;
    size_ok=wt_prepare_hook(&wt.path_size_hook,SEAM_File_getPathSize,(u32)wt_on_path_size,0xE92D4030u,0xE24DD024u);
    if(!size_ok)goto fail;
    save_ok=wt_prepare_hook(&wt.save_marker_hook,SEAM_FixedSlotStorage_writeSaveMarker,(u32)wt_on_save_marker,0xE92D4010u,0xE1A04000u);
    if(!save_ok)goto fail;
    exists_ok=wt_prepare_hook(&wt.file_exists_hook,SEAM_File_exists,(u32)wt_on_file_exists,0xE92D4030u,0xE24DDF43u);
    if(!exists_ok)goto fail;
    stream_ok=wt_prepare_hook(&wt.stream_hook,SEAM_FileManager_allocateFileStreamFromPath,(u32)wt_on_allocate_file_stream,0xE92D4030u,0xE24DD00Cu);
    if(!stream_ok)goto fail;
    wt_summary_hooks_ready=1;
    return 1;
fail:
    (void)list_ok;(void)size_ok;(void)save_ok;(void)exists_ok;(void)stream_ok;
    wt_remove_summary_hooks();
    return 0;
}

int world_transfer_install_hook(void){
    const char route_error[]="NuMC3DS transfer: world-list hook failed\n";
    const char summary_error[]="NuMC3DS transfer: summary optimizations disabled\n";
    const char thumbnail_error[]="NuMC3DS transfer: world thumbnails disabled\n";
    wt_runtime=&wt_storage;
    zero(wt_runtime,sizeof(*wt_runtime));
    wt_summary_hooks_ready=wt_install_summary_hooks();
    if(!wt_summary_hooks_ready&&s->host.debug_string)s->host.debug_string(summary_error,sizeof(summary_error)-1);
    wt.route_hook.target=SEAM_WorldSelection_openExisting;
    wt.route_hook.replacement=(u32)wt_on_world_selection_open;
    wt.route_hook.expected[0]=0xE92D4FF0u;
    wt.route_hook.expected[1]=0xE24DD03Cu;
    if(s->host.install_hook(&wt.route_hook)){
        wt_remove_summary_hooks();
        s->host.debug_string(route_error,sizeof(route_error)-1);
        wt_runtime=0;
        return -20;
    }
    if(wt_thumbnail_install_ui_hooks() && s->host.debug_string){
        s->host.debug_string(thumbnail_error,sizeof(thumbnail_error)-1);
    }
    return 0;
}

void world_transfer_remove_hooks(void){
    if(!wt_runtime)return;
    wt_thumbnail_remove_ui_hooks();
    wt_remove_hook(&wt.route_hook);
    wt_remove_summary_hooks();
    wt_summary_invalidate();
    ui_controls_destroy(&wt.edit_controls);
    wt_runtime=0;
}

void world_transfer_edit_setup(void *screen){
    UiWorldSettingsScreenView*view;void*grid;UiShared*spacer,*button;
    UiContainerView *container;
    UiSharedVector *children;
    UiElementView *grid_elem;
    int max_bottom;
    unsigned child_count, i;

    if(!screen)return;
    ui_controls_destroy(&wt.edit_controls);
    wt.edit_controls.specs=wt_edit_control_specs;
    wt.edit_controls.spec_count=sizeof(wt_edit_control_specs)/sizeof(wt_edit_control_specs[0]);
    wt.edit_screen=screen;
    wt.replace=0;
    wt.replace_armed=0;
    view=(UiWorldSettingsScreenView*)screen;
    grid=view->option_grid.control?view->option_grid.object:0;
    if(!grid||ui_controls_build(screen,&wt.edit_controls)<0)return;
    spacer=(UiShared*)ui_controls_shared(&wt.edit_controls,WT_EXPORT_SPACER_ID);
    button=(UiShared*)ui_controls_shared(&wt.edit_controls,WORLD_TRANSFER_EXPORT_ID);

    container=(UiContainerView*)grid;
    children=&container->children;
    grid_elem=(UiElementView*)grid;

    max_bottom=grid_elem->y;
    if(children->begin && children->end){
        child_count=(unsigned)(children->end - children->begin);
        for(i=0;i<child_count;i++){
            UiElementView *e=(UiElementView*)children->begin[i].object;
            if(e && e->visible){
                int b=e->y+e->height;
                if(b>max_bottom)max_bottom=b;
            }
        }
    }

    if(button && button->object){
        UiElementView *btn=(UiElementView*)button->object;
        btn->width=118;
        btn->height=28;
        btn->x=grid_elem->x+(grid_elem->width-btn->width)/2;
        btn->y=max_bottom+6;
    }
    if(spacer && spacer->object){
        UiElementView *sp=(UiElementView*)spacer->object;
        sp->x=grid_elem->x;
        sp->y=max_bottom+6;
        sp->width=0;
        sp->height=0;
        sp->visible=0;
    }

    if(!ui_grid_append_row(screen,grid,&view->row_widths,spacer,button,0))return;
    relayout_world_options(screen,grid);
}

int world_transfer_edit_pressed(void *screen,void *event){int id=ui_button_id(event);char label[96];void *button=wt_export_button();if(id!=WORLD_TRANSFER_EXPORT_ID||!button)return 0;if(wt.active)return 1;if(wt.replace_armed){wt.replace=1;wt.replace_armed=0;return wt_start_export(screen);}/* Collision is deliberately confirmed inline so the existing EditWorld
 * hook remains the only trampoline owner. */
    {const char *display=wt_edit_world_text(screen,SEAM_EditWorldScreen_displayNameOffset);char clean[WT_FS_MAX_PATH],destination[WT_FS_MAX_PATH];int exists,is_dir;wt_sanitize_name(display,clean,sizeof(clean));if(!wt_fs_ensure_public_roots()||!wt_fs_join(destination,sizeof(destination),"sdmc:/Minecraft 3DS/ExportedWorlds",clean))return 1;exists=wt_fs_exists(destination,&is_dir);if(exists&&is_dir){wt.replace_armed=1;ui_localized_label(label,sizeof(label),"numc3ds.transfer.replace","Replace Export");ui_button_set_label(button,label);ui_play_button_sound(screen);return 1;}}
    wt.replace=0;return wt_start_export(screen);
}

void world_transfer_tick(void *screen){unsigned step;if(!wt.active)return;if(wt.progress_screen!=screen){wt_trace("Q5\n");wt.progress_screen=screen;}if(wt.cancel_requested)wt_cancel_now();if(wt.phase==WT_PHASE_DONE&&wt.completion_hold>0){if(--wt.completion_hold==0){wt.completion_hold=-1;wt_schedule_close();}return;}/* Keep the same bounded 64 KiB of work per update, but issue four 16 KiB
 * operations instead of sixteen 4 KiB operations to cut FS dispatch cost
 * while staying within the native module's fixed memory allocation. */for(step=0;step<WT_STEPS_PER_TICK&&wt.active&&!wt.closing;step++)wt_tick_active();}
void world_transfer_cancel(void){if(!wt.active)return;if(wt.phase==WT_PHASE_IMPORT_COMMIT||wt.phase==WT_PHASE_IMPORT_FINAL_VERIFY||wt.phase==WT_PHASE_EXPORT_COMMIT||wt.phase==WT_PHASE_EXPORT_FINAL_VERIFY)return;wt.cancel_requested=1;}
int world_transfer_is_active(void){return wt.active!=0;}
const char *world_transfer_progress_message(void){return wt.message;}
static float wt_progress_ratio(unsigned long long done,unsigned long long total){while(total>0x00FFFFFFu){done>>=1;total>>=1;}if(!total)return 0.0f;if(done>total)done=total;return (float)(u32)done/(float)(u32)total;}
float world_transfer_progress_percent(void){unsigned long long total=(unsigned long long)wt.prepared_manifest.bytes,done=(unsigned long long)wt.walk.manifest.bytes;float ratio;if(wt.walk.file.active&&wt.walk.file.offset>0)done+=(unsigned long long)wt.walk.file.offset;ratio=wt_progress_ratio(done,total);switch(wt.phase){case WT_PHASE_IMPORT_PREPARE:case WT_PHASE_EXPORT_PREPARE:return 0.0f;case WT_PHASE_IMPORT_COPY:return 5.0f+ratio*65.0f;case WT_PHASE_IMPORT_VERIFY_TARGET:return 70.0f+ratio*25.0f;case WT_PHASE_IMPORT_PARSE:return 97.0f;case WT_PHASE_IMPORT_COMMIT:case WT_PHASE_IMPORT_FINAL_VERIFY:case WT_PHASE_IMPORT_CLEANUP:return 99.0f;case WT_PHASE_EXPORT_COPY:return 5.0f+ratio*90.0f;case WT_PHASE_EXPORT_COMMIT:return 99.0f;case WT_PHASE_DONE:return 100.0f;default:return 0.0f;}}
void world_transfer_summary_invalidate(void){wt_summary_invalidate();}

int world_transfer_on_progress_destroy(void *screen){int reopen=0;char label[96];void*button=wt_export_button();if(wt.progress_screen==screen){wt.progress_screen=0;wt.cancel_button=0;if(wt.operation==WT_JOURNAL_EXPORT&&button){ui_localized_label(label,sizeof(label),"numc3ds.transfer.export_action","Export World");ui_button_set_label(button,label);wt.replace=0;wt.replace_armed=0;}if(wt.pending_reopen){reopen=1;wt.pending_reopen=0;}else if(wt.phase==WT_PHASE_DONE||wt.phase==WT_PHASE_FAILED){wt.active=0;wt.phase=WT_PHASE_IDLE;wt.closing=0;}}return reopen;}
void world_transfer_reopen_after_progress(void){if(wt.route_hook.trampoline&&wt.chooser)((void(*)(void*,void*,int))wt.route_hook.trampoline)(wt.chooser,0,wt.tab_index);wt.chooser=0;wt.active=0;wt.phase=WT_PHASE_IDLE;wt.closing=0;wt.cancel_requested=0;}
