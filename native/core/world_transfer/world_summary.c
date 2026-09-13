#include "world_summary.h"
#include "world_transfer_fs.h"
#include "../world/world_meta.h"
#include "../world/world_thumbnail.h"
#include "../seams.h"

enum {
    WT_SUMMARY_WORLD_ID_MAX = 128u,
    WT_SUMMARY_CACHE_CAP = 128u,
    WT_SUMMARY_MAX_WORLDS = 64u,
    WT_LEVEL_SUMMARY_SIZE = 0x80u
};

typedef void *(*LevelSummaryCopyFn)(void *dest, const void *src);
typedef void (*LevelSummaryDtorFn)(void *summary);
typedef void (*VectorInsertAuxFn)(void *vector, void *position, const void *item);

typedef struct {
    void *start;
    void *finish;
    void *end_of_storage;
} WtStdVector;

static u8 s_cached_level_summaries[WT_SUMMARY_MAX_WORLDS][WT_LEVEL_SUMMARY_SIZE];
static unsigned s_cached_level_summary_count = 0;
static int s_level_summaries_valid = 0;

typedef struct {
    char world_id[WT_SUMMARY_WORLD_ID_MAX];
    unsigned long long size;
    u32 valid;
} WtSummaryMemoryEntry;

static WtSummaryMemoryEntry s_summary_cache[WT_SUMMARY_CACHE_CAP];
static unsigned s_summary_cache_count = 0;

static unsigned wt_summary_len(const char *text){
    unsigned n=0;
    if(text)while(text[n]&&n<WT_FS_MAX_PATH)n++;
    return n;
}

static int wt_summary_copy(char *out,unsigned cap,const char *text){
    unsigned n=wt_summary_len(text);
    if(!out||!cap||n+1>cap)return 0;
    cp(out,text,n+1);
    return 1;
}

const char *wt_summary_string_text(void *object){
    const char *text=0;
    if(object)cp(&text,object,sizeof(text));
    return text;
}

static int wt_summary_text_length(const char *text,unsigned *out_length){
    unsigned n;
    if(!text)return 0;
    for(n=0;n<WT_FS_MAX_PATH;n++){
        if(text[n]==0){if(out_length)*out_length=n;return n>0;}
    }
    return 0;
}

static int wt_summary_match_at(const char *text,unsigned offset,const char *needle){
    unsigned i;
    for(i=0;needle[i];i++)if(text[offset+i]!=needle[i])return 0;
    return 1;
}

int wt_summary_text_suffix(const char *text,const char *suffix);

int wt_summary_match_substring(const char *haystack, const char *needle){
    unsigned h_len = 0, n_len = 0, i;
    if(!haystack || !needle) return 0;
    if(!wt_summary_text_length(haystack, &h_len) || !wt_summary_text_length(needle, &n_len)) return 0;
    if(n_len > h_len) return 0;
    for(i = 0; i <= h_len - n_len; i++){
        if(wt_summary_match_at(haystack, i, needle)) return 1;
    }
    return 0;
}

int wt_summary_extract_world_id(void *path_object,char *out,unsigned cap){
    const char *text=wt_summary_string_text(path_object);
    unsigned length,i,start=0,end;
    if(!out||cap<2||!wt_summary_text_length(text,&length)||length>=WT_FS_MAX_PATH)return 0;
    for(i=0;i<length;i++){
        if(i==0||text[i-1]=='/'){
            if(i+16<=length&&wt_summary_match_at(text,i,"minecraftWorlds/")){
                start=i+16;
                break;
            }
            if(i+7<=length&&wt_summary_match_at(text,i,"worlds/")){
                start=i+7;
                break;
            }
        }
    }
    if(!start)return 0;
    while(start<length&&text[start]=='/')start++;
    end=start;
    while(end<length&&text[end]!='/')end++;
    if(end==start||end-start>=cap)return 0;
    cp(out,text+start,end-start);
    out[end-start]=0;
    return wt_fs_validate_relative(out);
}

static int wt_summary_suffix(void *path_object,const char *suffix){
    const char *text=wt_summary_string_text(path_object);
    return wt_summary_text_suffix(text,suffix);
}

int wt_summary_text_suffix(const char *text,const char *suffix){
    unsigned length,suffix_length,i;
    if(!suffix||!wt_summary_text_length(text,&length))return 0;
    suffix_length=wt_summary_len(suffix);
    if(!suffix_length||suffix_length>length)return 0;
    i=length-suffix_length;
    if(i==0||text[i-1]!='/')return 0;
    return wt_summary_match_at(text,i,suffix);
}

void wt_summary_begin(void){
}

void wt_summary_end(void){
    /* In-memory cache needs no file flushing */
}

void wt_summary_invalidate(void){
    unsigned i;
    for(i = 0; i < s_summary_cache_count; i++){
        s_summary_cache[i].valid = 0;
    }
    s_summary_cache_count = 0;
    if(s_level_summaries_valid){
        for(i = 0; i < s_cached_level_summary_count; i++){
            ((LevelSummaryDtorFn)SEAM_LevelSummary_dtor)(&s_cached_level_summaries[i][0]);
        }
        s_cached_level_summary_count = 0;
        s_level_summaries_valid = 0;
    }
}

void wt_summary_invalidate_world(const char *world_id){
    unsigned i;
    if(s_level_summaries_valid){
        for(i = 0; i < s_cached_level_summary_count; i++){
            ((LevelSummaryDtorFn)SEAM_LevelSummary_dtor)(&s_cached_level_summaries[i][0]);
        }
        s_cached_level_summary_count = 0;
        s_level_summaries_valid = 0;
    }
    if(!world_id||!*world_id)return;
    for(i=0;i<s_summary_cache_count;i++){
        if(s_summary_cache[i].valid&&streq(s_summary_cache[i].world_id,world_id)){
            s_summary_cache[i].valid=0;
            return;
        }
    }
}

int wt_summary_lookup_path(void *path_object,unsigned long long *out_size){
    char world_id[WT_SUMMARY_WORLD_ID_MAX];
    unsigned i;
    if(out_size)*out_size=0;
    if(!wt_summary_extract_world_id(path_object,world_id,sizeof(world_id)))return 0;
    for(i=0;i<s_summary_cache_count;i++){
        if(s_summary_cache[i].valid&&streq(s_summary_cache[i].world_id,world_id)){
            if(out_size)*out_size=s_summary_cache[i].size;
            return 1;
        }
    }
    return 0;
}

void wt_summary_record_path(void *path_object,unsigned long long size){
    char world_id[WT_SUMMARY_WORLD_ID_MAX];
    unsigned i;
    if(!wt_summary_extract_world_id(path_object,world_id,sizeof(world_id)))return;
    for(i=0;i<s_summary_cache_count;i++){
        if(streq(s_summary_cache[i].world_id,world_id)){
            s_summary_cache[i].size=size;
            s_summary_cache[i].valid=1;
            return;
        }
    }
    if(s_summary_cache_count>=WT_SUMMARY_CACHE_CAP)return;
    zero(&s_summary_cache[s_summary_cache_count],sizeof(WtSummaryMemoryEntry));
    if(!wt_summary_copy(s_summary_cache[s_summary_cache_count].world_id,WT_SUMMARY_WORLD_ID_MAX,world_id))return;
    s_summary_cache[s_summary_cache_count].size=size;
    s_summary_cache[s_summary_cache_count].valid=1;
    s_summary_cache_count++;
}

#define WT_SUMMARY_SCAN_BATCH 64

static unsigned long long wt_summary_scan_dir_fast(const char *dir_path, unsigned depth, WtFsDirectoryEntry *entries, int batch_cap){
    WtFsDirectory dir;
    char subdirs[8][32];
    int subdir_count = 0;
    unsigned long long total = 0;
    char child[WT_FS_MAX_PATH], name[WT_FS_MAX_PATH];
    int count, i, s;
    unsigned n;

    if(depth > 8 || !dir_path || !*dir_path || !entries || batch_cap <= 0) return 0;
    if(!wt_fs_open_directory(dir_path, &dir)) return 0;

    for(;;){
        if(!wt_fs_read_directory(&dir, entries, batch_cap, &count) || count <= 0) break;
        for(i = 0; i < count; i++){
            if(wt_fs_entry_is_directory(&entries[i])){
                if(!wt_fs_entry_name_utf8(&entries[i], name, sizeof(name))) continue;
                if(!wt_fs_validate_relative(name)) continue;
                if(subdir_count < 8){
                    wt_summary_copy(subdirs[subdir_count], sizeof(subdirs[subdir_count]), name);
                    subdir_count++;
                }
            } else {
                long long es = wt_fs_entry_size(&entries[i]);
                if(es > 0){
                    total += (unsigned long long)es;
                } else {
                    if(!wt_fs_entry_name_utf8(&entries[i], name, sizeof(name))) continue;
                    n = wt_summary_len(name);
                    if(n > 7 && name[0] == 's' && name[1] == 'l' && name[2] == 't' &&
                       name[n-4] == '.' && name[n-3] == 'c' && name[n-2] == 'd' && name[n-1] == 'b'){
                        total += 1310740ULL;
                    }
                }
            }
        }
    }
    wt_fs_close_directory(&dir);

    for(s = 0; s < subdir_count; s++){
        if(wt_fs_join(child, sizeof(child), dir_path, subdirs[s])){
            total += wt_summary_scan_dir_fast(child, depth + 1u, entries, batch_cap);
        }
    }

    return total;
}

unsigned long long wt_summary_calculate_world_size_force_scan(const char *world_id){
    char full_path[WT_FS_MAX_PATH];
    WtFsDirectoryEntry entries[WT_SUMMARY_SCAN_BATCH];
    if(!world_id || !*world_id) return 0;
    if(!wt_fs_join(full_path, sizeof(full_path), "extdata:/minecraftWorlds", world_id)) return 0;
    return wt_summary_scan_dir_fast(full_path, 0, entries, WT_SUMMARY_SCAN_BATCH);
}

unsigned long long wt_summary_calculate_world_size(const char *world_id){
    WtWorldMeta meta;
    unsigned long long size;

    if(!world_id || !*world_id) return 0;

    /* 1. Fast path: check per-world sidecar metadata accelerator */
    if(wt_meta_read(world_id, &meta) && meta.total_size > 0){
        return meta.total_size;
    }

    /* 2. Fallback: 64-entry batch directory scan */
    size = wt_summary_calculate_world_size_force_scan(world_id);

    /* 3. Cache exact total into sidecar for subsequent sub-150ms cold boots */
    if(size > 0){
        wt_meta_write(world_id, size, (u32)wt_thumbnail_exists(world_id));
    }
    return size;
}

int wt_summary_path_is_resource_pack(void *path_object){return wt_summary_suffix(path_object,"rsrc_packs.txt");}
int wt_summary_text_is_resource_pack(const char *path_text){return wt_summary_text_suffix(path_text,"rsrc_packs.txt");}

int wt_summary_vector_valid(void){
    return s_level_summaries_valid;
}

int wt_summary_vector_populate(void *vector){
    WtStdVector *vec = (WtStdVector*)vector;
    unsigned i;
    if(!s_level_summaries_valid || !vec) return 0;
    for(i = 0; i < s_cached_level_summary_count; i++){
        const void *src = &s_cached_level_summaries[i][0];
        if(vec->finish != vec->end_of_storage){
            void *dest = vec->finish;
            vec->finish = (void*)((u8*)vec->finish + WT_LEVEL_SUMMARY_SIZE);
            ((LevelSummaryCopyFn)SEAM_LevelSummary_copy)(dest, src);
        } else {
            ((VectorInsertAuxFn)SEAM_vector_LevelSummary_insertAux)(vec, vec->finish, src);
        }
    }
    return 1;
}

void wt_summary_vector_save(const void *vector){
    const WtStdVector *vec = (const WtStdVector*)vector;
    unsigned i, count;
    if(!vec) return;
    if(s_level_summaries_valid){
        for(i = 0; i < s_cached_level_summary_count; i++){
            ((LevelSummaryDtorFn)SEAM_LevelSummary_dtor)(&s_cached_level_summaries[i][0]);
        }
        s_cached_level_summary_count = 0;
        s_level_summaries_valid = 0;
    }
    if(!vec->start || !vec->finish || (u8*)vec->finish < (u8*)vec->start){
        s_cached_level_summary_count = 0;
        s_level_summaries_valid = 1;
        return;
    }
    count = (unsigned)(((u8*)vec->finish - (u8*)vec->start) / WT_LEVEL_SUMMARY_SIZE);
    if(count > WT_SUMMARY_MAX_WORLDS) count = WT_SUMMARY_MAX_WORLDS;
    for(i = 0; i < count; i++){
        const void *item = (const void*)((const u8*)vec->start + i * WT_LEVEL_SUMMARY_SIZE);
        ((LevelSummaryCopyFn)SEAM_LevelSummary_copy)(&s_cached_level_summaries[i][0], item);
    }
    s_cached_level_summary_count = count;
    s_level_summaries_valid = 1;
}
