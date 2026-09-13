#include "world_transfer_fs.h"

enum { WT_FS_READ_MODE=1u, WT_FS_WRITE_MODE=2u, WT_FS_MAX_DELETE_DEPTH=16u };

static int wt_sdmc_state;

typedef int (*WtOpenSpecialArchiveFn)(void **,u32);
typedef int (*WtRegisterArchiveFn)(const char *,void *,int,int);
typedef void *(*WtFindArchiveWideFn)(const wt_u16 *);
typedef void *(*WtFindArchiveCharFn)(const char *);
typedef void (*WtInitializeFileFn)(void **,wt_u16 *,u32);
typedef void (*WtInitializeDirectoryFn)(void **,wt_u16 *);
typedef int (*WtReadFileFn)(int *,void *,long long,void *,u32);
typedef int (*WtWriteFileFn)(int *,void *,long long,const void *,u32,int);
typedef int (*WtReadDirectoryFn)(int *,void *,WtFsDirectoryEntry *,int);
typedef void (*WtCloseFn)(void *);
typedef int (*WtArchiveOpenFileFn)(void *,void **,const WtFsLowPath *,u32);
typedef int (*WtArchiveOpenDirectoryFn)(void *,void **,const WtFsLowPath *);
typedef int (*WtArchivePathFn)(void *,const WtFsLowPath *);
typedef int (*WtArchiveCreateFileFn)(void *,const WtFsLowPath *,long long);
typedef int (*WtArchiveFreeBytesFn)(void *,long long *);

static unsigned wt_len(const char *text){unsigned n=0;if(text)while(text[n])n++;return n;}
int wt_fs_copy_text(char *out,unsigned cap,const char *text){unsigned i=0;if(!out||!cap)return 0;if(!text)text="";while(text[i]&&i+1<cap){out[i]=text[i];i++;}out[i]=0;return !text[i];}

static int wt_utf8_next(const char **at,u32 *out){
    const u8 *p=(const u8*)*at;u32 cp,n,min;unsigned i;
    if(!p||!*p)return 0;
    if(p[0]<0x80){cp=p[0];n=1;min=0;}
    else if(p[0]>=0xC2&&p[0]<=0xDF){cp=p[0]&0x1F;n=2;min=0x80;}
    else if(p[0]>=0xE0&&p[0]<=0xEF){cp=p[0]&0x0F;n=3;min=0x800;}
    else if(p[0]>=0xF0&&p[0]<=0xF4){cp=p[0]&7;n=4;min=0x10000;}
    else return 0;
    for(i=1;i<n;i++){if((p[i]&0xC0)!=0x80)return 0;cp=(cp<<6)|(p[i]&0x3F);}
    if(cp<min||cp>0x10FFFFu||(cp>=0xD800u&&cp<=0xDFFFu))return 0;
    *at=(const char*)(p+n);*out=cp;return 1;
}

static int wt_utf8_valid(const char *text){const char *p=text;u32 cp;while(p&&*p)if(!wt_utf8_next(&p,&cp))return 0;return 1;}

static int wt_make_wide(const char *path,wt_u16 *out,unsigned cap,unsigned *units_out){
    const char *p=path;u32 cp;unsigned n=0;unsigned i;
    if(!path||!out||cap<2||!wt_utf8_valid(path))return 0;
    while(*p){
        if(!wt_utf8_next(&p,&cp))return 0;
        if(cp<=0xFFFFu){if(n+1>=cap)return 0;out[n++]=(wt_u16)cp;}
        else {if(n+2>=cap)return 0;cp-=0x10000u;out[n++]=(wt_u16)(0xD800u+(cp>>10));out[n++]=(wt_u16)(0xDC00u+(cp&0x3FFu));}
    }
    out[n]=0;
    for(i=0;i<n;i++)if(out[i]==0)return 0;
    if(units_out)*units_out=n;
    return n>0;
}

static int wt_prefix_colon(const wt_u16 *wide,unsigned units,unsigned *colon){unsigned i;for(i=0;i<units;i++)if(wide[i]==':'){if(colon)*colon=i;return 1;}return 0;}

static int wt_make_archive_path(const char *path,wt_u16 *wide,unsigned cap,void **archive,WtFsLowPath *low){
    unsigned units,colon,sub_units;
    void *a;
    if(!wt_make_wide(path,wide,cap,&units)||!wt_prefix_colon(wide,units,&colon)||colon==0||colon+1>=units)return 0;
    a=((WtFindArchiveWideFn)SEAM_nn_fs_FindArchiveWide)(wide);
    if(!a)return 0;
    sub_units=units-colon-1;
    low->type=4;low->data=wide+colon+1;low->length=(sub_units+1u)*2u;
    if(archive)*archive=a;
    return 1;
}

static int wt_result(int value){return value>=0;}

int wt_fs_mount_sdmc(void){
    void *archive=0;int result;
    if(wt_sdmc_state==1)return 1;
    if(((WtFindArchiveCharFn)SEAM_nn_fs_FindArchiveChar)("sdmc:")){wt_sdmc_state=1;return 1;}
    result=((WtOpenSpecialArchiveFn)SEAM_nn_fs_OpenSpecialArchiveRaw)(&archive,9u);
    if(!wt_result(result)||!archive){wt_sdmc_state=-1;return 0;}
    result=((WtRegisterArchiveFn)SEAM_nn_fs_RegisterArchive)("sdmc:",archive,0,0);
    if(!wt_result(result)){
        void **vtable=*(void***)archive;
        if(vtable&&vtable[0x30/4])((void(*)(void*))vtable[0x30/4])(archive);
        wt_sdmc_state=-1;return 0;
    }
    wt_sdmc_state=1;return 1;
}

int wt_fs_sdmc_ready(void){return wt_sdmc_state==1;}

int wt_fs_open_file(const char *path,int write,WtFsFile *out){wt_u16 wide[WT_FS_MAX_WIDE];void *handle=0;if(!out)return 0;out->handle=0;if(!wt_make_wide(path,wide,WT_FS_MAX_WIDE,0))return 0;/* Writers create the exact-size file through the archive first.  Opening that existing file with OPEN_MODE_CREATE is rejected by extdata and SDMC. */((WtInitializeFileFn)SEAM_nn_fs_FileManagerTryInitializeFile)(&handle,wide,write?WT_FS_WRITE_MODE:WT_FS_READ_MODE);if(!handle)return 0;out->handle=handle;return 1;}
void wt_fs_close_file(WtFsFile *file){if(file&&file->handle){((WtCloseFn)SEAM_nn_fs_FileClose)(file->handle);file->handle=0;}}
int wt_fs_read(WtFsFile *file,long long offset,void *buffer,u32 size,u32 *out_bytes){int count=0,result;if(out_bytes)*out_bytes=0;if(!file||!file->handle||!buffer)return 0;result=((WtReadFileFn)SEAM_nn_fs_TryReadFile)(&count,file->handle,offset,buffer,size);if(!wt_result(result)||count<0)return 0;if(out_bytes)*out_bytes=(u32)count;return 1;}
int wt_fs_write(WtFsFile *file,long long offset,const void *buffer,u32 size,u32 *out_bytes){int count=0,result;if(out_bytes)*out_bytes=0;if(!file||!file->handle||(!buffer&&size))return 0;result=((WtWriteFileFn)SEAM_nn_fs_TryWriteFile)(&count,file->handle,offset,buffer,size,1);if(!wt_result(result)||count<0)return 0;if(out_bytes)*out_bytes=(u32)count;return 1;}

int wt_fs_create_file(const char *path,long long size){wt_u16 wide[WT_FS_MAX_WIDE];WtFsLowPath low;void *archive;void **vtable;if(!wt_make_archive_path(path,wide,WT_FS_MAX_WIDE,&archive,&low))return 0;vtable=*(void***)archive;if(!vtable||!vtable[0x18/4])return 0;return wt_result(((WtArchiveCreateFileFn)vtable[0x18/4])(archive,&low,size));}
int wt_fs_delete_file(const char *path){wt_u16 wide[WT_FS_MAX_WIDE];WtFsLowPath low;void *archive;void **vtable;if(!wt_make_archive_path(path,wide,WT_FS_MAX_WIDE,&archive,&low))return 0;vtable=*(void***)archive;if(!vtable||!vtable[0x8/4])return 0;return wt_result(((WtArchivePathFn)vtable[0x8/4])(archive,&low));}
int wt_fs_file_size(const char *path,long long *out_size){WtFsFile file;int result;void *h;if(out_size)*out_size=0;if(!wt_fs_open_file(path,0,&file))return 0;h=(void*)((u32)file.handle&~1u);result=((int(*)(long long*,void*))SEAM_nn_fs_TryGetSize)(out_size,h);wt_fs_close_file(&file);return wt_result(result);}

int wt_fs_open_directory(const char *path,WtFsDirectory *out){wt_u16 wide[WT_FS_MAX_WIDE];void *handle=0;if(!out)return 0;out->handle=0;if(!wt_make_wide(path,wide,WT_FS_MAX_WIDE,0))return 0;((WtInitializeDirectoryFn)SEAM_nn_fs_FileManagerTryInitializeDirectory)(&handle,wide);if(!handle)return 0;out->handle=handle;return 1;}
void wt_fs_close_directory(WtFsDirectory *directory){if(directory&&directory->handle){((WtCloseFn)SEAM_nn_fs_DirectoryClose)(directory->handle);directory->handle=0;}}
int wt_fs_read_directory(WtFsDirectory *directory,WtFsDirectoryEntry *entries,int max_entries,int *out_count){int count=0,result;if(out_count)*out_count=0;if(!directory||!directory->handle||!entries||max_entries<=0)return 0;result=((WtReadDirectoryFn)SEAM_nn_fs_TryReadDirectory)(&count,directory->handle,entries,max_entries);if(!wt_result(result)||count<0)return 0;if(out_count)*out_count=count;return 1;}

int wt_fs_create_directory(const char *path){wt_u16 wide[WT_FS_MAX_WIDE];WtFsLowPath low;void *archive;void **vtable;if(!wt_make_archive_path(path,wide,WT_FS_MAX_WIDE,&archive,&low))return 0;vtable=*(void***)archive;if(!vtable||!vtable[0x1c/4])return 0;return wt_result(((WtArchivePathFn)vtable[0x1c/4])(archive,&low));}
int wt_fs_delete_directory(const char *path){wt_u16 wide[WT_FS_MAX_WIDE];WtFsLowPath low;void *archive;void **vtable;if(!wt_make_archive_path(path,wide,WT_FS_MAX_WIDE,&archive,&low))return 0;vtable=*(void***)archive;if(!vtable||!vtable[0x10/4])return 0;return wt_result(((WtArchivePathFn)vtable[0x10/4])(archive,&low));}

static int wt_fs_delete_tree_inner(const char *path,unsigned depth){
    WtFsDirectory directory;
    WtFsDirectoryEntry entries[WT_FS_DIRECTORY_BATCH];
    int count,i;
    if(depth>=WT_FS_MAX_DELETE_DEPTH||!wt_fs_open_directory(path,&directory))return 0;
    for(;;){
        if(!wt_fs_read_directory(&directory,entries,WT_FS_DIRECTORY_BATCH,&count)){wt_fs_close_directory(&directory);return 0;}
        if(count==0)break;
        for(i=0;i<count;i++){
            char name[WT_FS_MAX_PATH],child[WT_FS_MAX_PATH];
            if(!wt_fs_entry_name_utf8(&entries[i],name,sizeof(name))||!wt_fs_validate_relative(name)||!wt_fs_join(child,sizeof(child),path,name)){wt_fs_close_directory(&directory);return 0;}
            if(wt_fs_entry_is_directory(&entries[i])){
                if(!wt_fs_delete_tree_inner(child,depth+1u)){wt_fs_close_directory(&directory);return 0;}
            }else if(!wt_fs_delete_file(child)){
                wt_fs_close_directory(&directory);return 0;
            }
        }
    }
    wt_fs_close_directory(&directory);
    return wt_fs_delete_directory(path);
}

int wt_fs_delete_tree(const char *path){int is_directory=0;if(!path||!*path)return 0;if(!wt_fs_exists(path,&is_directory))return 1;if(!is_directory)return wt_fs_delete_file(path);return wt_fs_delete_tree_inner(path,0);}

int wt_fs_exists(const char *path,int *is_directory){WtFsDirectory dir;WtFsFile file;if(is_directory)*is_directory=0;if(wt_fs_open_directory(path,&dir)){wt_fs_close_directory(&dir);if(is_directory)*is_directory=1;return 1;}if(wt_fs_open_file(path,0,&file)){wt_fs_close_file(&file);return 1;}return 0;}

int wt_fs_rename(const char *current_path,const char *new_path){wt_u16 current[WT_FS_MAX_WIDE],target[WT_FS_MAX_WIDE];WtFsLowPath a,b;void *archive_a,*archive_b;void **vtable;if(!wt_make_archive_path(current_path,current,WT_FS_MAX_WIDE,&archive_a,&a)||!wt_make_archive_path(new_path,target,WT_FS_MAX_WIDE,&archive_b,&b)||archive_a!=archive_b)return 0;vtable=*(void***)archive_a;if(!vtable||!vtable[0xc/4])return 0;return wt_result(((int(*)(void*,const WtFsLowPath*,const WtFsLowPath*))vtable[0xc/4])(archive_a,&a,&b));}
int wt_fs_get_free_bytes(const char *archive_path,long long *out_bytes){wt_u16 wide[WT_FS_MAX_WIDE];WtFsLowPath low;void *archive;void **vtable;(void)low;if(out_bytes)*out_bytes=0;if(!wt_make_archive_path(archive_path,wide,WT_FS_MAX_WIDE,&archive,&low))return 0;vtable=*(void***)archive;if(!vtable||!vtable[0x2c/4])return 0;return wt_result(((WtArchiveFreeBytesFn)vtable[0x2c/4])(archive,out_bytes));}

int wt_fs_mkdirs(const char *path){
    char part[WT_FS_MAX_PATH];
    unsigned n=wt_len(path),i,start=0;
    int exists,created;
    if(!path||n==0||n>=sizeof(part)||!wt_fs_copy_text(part,sizeof(part),path))return 0;
    /* Do not try to create the archive root (for example, "sdmc:").
     * The first component after the archive colon is the first real
     * directory that this helper owns. */
    for(i=0;i<n;i++)if(part[i]==':'){start=i+1;break;}
    for(i=start;i<=n;i++){
        if(i==n||part[i]=='/'){
            if(i==start||(i==n&&i>start&&part[i-1]=='/'))continue;
            {char saved=part[i];part[i]=0;exists=wt_fs_exists(part,&created);
             if(!exists){if(!wt_fs_create_directory(part)){part[i]=saved;return 0;}}
             else if(!created){part[i]=saved;return 0;}
             part[i]=saved;}
        }
    }
    return 1;
}

int wt_fs_entry_is_directory(const WtFsDirectoryEntry *entry){return entry&&entry->attributes[0]!=0;}
long long wt_fs_entry_size(const WtFsDirectoryEntry *entry){return entry?entry->entry_size:0;}
int wt_fs_entry_name_utf8(const WtFsDirectoryEntry *entry,char *out,unsigned cap){unsigned i=0,n=0;u32 cp;wt_u16 high;if(!entry||!out||cap<2)return 0;while(i<WT_FS_MAX_ENTRY_NAME&&entry->entry_name[i]){cp=entry->entry_name[i++];if(cp>=0xD800u&&cp<=0xDBFFu){high=(wt_u16)cp;if(i>=WT_FS_MAX_ENTRY_NAME||entry->entry_name[i]<0xDC00u||entry->entry_name[i]>0xDFFFu)return 0;cp=0x10000u+(((u32)high-0xD800u)<<10)+(entry->entry_name[i++]-0xDC00u);}else if(cp>=0xDC00u&&cp<=0xDFFFu)return 0;if(cp<0x80u){if(n+1>=cap)return 0;out[n++]=(char)cp;}else if(cp<0x800u){if(n+2>=cap)return 0;out[n++]=(char)(0xC0u|(cp>>6));out[n++]=(char)(0x80u|(cp&0x3Fu));}else if(cp<0x10000u){if(n+3>=cap)return 0;out[n++]=(char)(0xE0u|(cp>>12));out[n++]=(char)(0x80u|((cp>>6)&0x3Fu));out[n++]=(char)(0x80u|(cp&0x3Fu));}else{if(n+4>=cap)return 0;out[n++]=(char)(0xF0u|(cp>>18));out[n++]=(char)(0x80u|((cp>>12)&0x3Fu));out[n++]=(char)(0x80u|((cp>>6)&0x3Fu));out[n++]=(char)(0x80u|(cp&0x3Fu));}}out[n]=0;return n>0;}

static int wt_component_equal(const char *start,unsigned length,const char *value){unsigned i;if(wt_len(value)!=length)return 0;for(i=0;i<length;i++)if(start[i]!=value[i])return 0;return 1;}
int wt_fs_validate_relative(const char *path){const char *p=path,*component;unsigned length;u32 cp;if(!path||!*path||path[0]=='/'||path[0]=='\\')return 0;if(!wt_utf8_valid(path))return 0;component=path;length=0;while(*p){if(*p==':'||*p=='\\'||(u8)*p<0x20u)return 0;if(*p=='/'){{if(length==0||wt_component_equal(component,length,".")||wt_component_equal(component,length,".."))return 0;component=p+1;length=0;} }else length++;p++;}if(length==0||wt_component_equal(component,length,".")||wt_component_equal(component,length,".."))return 0;return 1;}
int wt_fs_join(char *out,unsigned cap,const char *parent,const char *child){unsigned a,b;int slash;if(!out||!cap||!parent||!child)return 0;a=wt_len(parent);b=wt_len(child);slash=(a>0&&parent[a-1]!='/');if(a+b+(slash?1:0)+1>cap)return 0;cp(out,parent,a);if(slash)out[a++]='/';cp(out+a,child,b);out[a+b]=0;return 1;}

static int s_public_roots_ready = 0;
static int s_private_roots_ready = 0;

int wt_fs_ensure_public_roots(void){
    if(s_public_roots_ready && wt_fs_sdmc_ready()) return 1;
    if(!wt_fs_mount_sdmc())return 0;
    if(!wt_fs_mkdirs("sdmc:/Minecraft 3DS/ImportedWorlds"))return 0;
    if(!wt_fs_mkdirs("sdmc:/Minecraft 3DS/ExportedWorlds"))return 0;
    if(!wt_fs_mkdirs("sdmc:/Minecraft 3DS/.numc3ds-transfer"))return 0;
    s_public_roots_ready = 1;
    return 1;
}
int wt_fs_ensure_private_roots(void){
    if(s_private_roots_ready) return 1;
    if(!wt_fs_mkdirs("extdata:/numc3ds-transfer")) return 0;
    s_private_roots_ready = 1;
    return 1;
}
