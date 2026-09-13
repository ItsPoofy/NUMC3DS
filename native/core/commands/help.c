#include "../internal.h"
#include "native_command_context.h"
#include "native_property_bag.h"
#include "native_schema.h"
#include "command_localization.h"

enum { HELP_PAGE_SIZE=5 };

static const NativeSchemaRoot *help_root(const char *name){
    unsigned index;
    if(!name)return 0;
    for(index=0;index<native_schema_root_count;index++)if(streq(name,native_schema_text(native_schema_roots[index].name)))return &native_schema_roots[index];
    for(index=0;index<native_schema_map_entry_count;index++)if(streq(name,native_schema_text(native_schema_map_entries[index].name)))return &native_schema_roots[native_schema_map_entries[index].root_index];
    return 0;
}

static const NativeSchemaVersion *visible_version(const NativeSchemaRoot *root){
    const NativeSchemaVersion *versions;unsigned index;
    if(!root)return 0;versions=native_schema_root_versions(root);
    for(index=0;index<root->version_count;index++)if(!native_schema_version_flag(&versions[index],NATIVE_SCHEMA_VERSION_HIDDEN))return &versions[index];
    return 0;
}

static void append_line(char *out,unsigned *length,const char *text){
    if(*length)append(out,length,"\n");append(out,length,text?text:"");
}

static void append_description(char *out,unsigned *length,const NativeSchemaRoot *root){
    const NativeSchemaVersion *version=visible_version(root);char description[MAX_TEXT+1];
    if(!version)return;
    if(!command_localize(description,sizeof(description),native_schema_text(version->description),0,0))copy_text(description,native_schema_text(version->description),sizeof(description)-1);
    append(out,length," - ");append(out,length,description);
}

static int help_command(const char *name){
    const NativeSchemaRoot *root=help_root(name);const NativeSchemaVersion *version;char body[MAX_TEXT+1];unsigned length=0,version_index;
    if(!root||!native_schema_root_visible(root))return fail_command("Unknown command");
    body[0]=0;append(body,&length,"/");append(body,&length,native_schema_text(root->name));append_description(body,&length,root);
    for(version_index=0;version_index<root->version_count;version_index++){
        unsigned overload_index;
        version=&native_schema_root_versions(root)[version_index];
        if(native_schema_version_flag(version,NATIVE_SCHEMA_VERSION_HIDDEN))continue;
        for(overload_index=0;overload_index<version->overload_count;overload_index++)append_line(body,&length,native_schema_text(native_schema_version_overloads(version)[overload_index].usage));
    }
    native_command_context_output_string("body",body);
    return 1;
}

static void append_page_entry(char *out,unsigned *length,const NativeSchemaRoot *root){
    char line[MAX_TEXT+1];unsigned line_length=0;
    line[0]=0;append(line,&line_length,"/");append(line,&line_length,native_schema_text(root->name));append_description(line,&line_length,root);append_line(out,length,line);
}

int cmd_help(void*player,void*level,void*game,const char*p){
    const NativeCommandContext *context=native_command_context_current();const char *name;int page=1;unsigned pages,start,end,index,visible_count=0,visible_index=0;char body[MAX_TEXT+1];unsigned length=0;
    (void)player;(void)level;(void)game;(void)p;
    if(!context)return fail_syntax();
    name=native_bag_get_string(context->input_bag,"command");
    if(name)return help_command(name);
    native_bag_get_int(context->input_bag,"page",&page);
    for(index=0;index<native_schema_root_count;index++)if(native_schema_root_visible(&native_schema_roots[index]))visible_count++;
    pages=(visible_count+HELP_PAGE_SIZE-1u)/HELP_PAGE_SIZE;
    if(page<1||(unsigned)page>pages)return fail_command("Invalid help page");
    start=((unsigned)page-1u)*HELP_PAGE_SIZE;end=start+HELP_PAGE_SIZE;if(end>visible_count)end=visible_count;
    body[0]=0;
    for(index=0;index<native_schema_root_count&&visible_index<end;index++){
        const NativeSchemaRoot *root=&native_schema_roots[index];
        if(!native_schema_root_visible(root))continue;
        if(visible_index++<start)continue;
        append_page_entry(body,&length,root);
    }
    native_command_context_output_int("page",page);
    native_command_context_output_int("pageCount",(int)pages);
    native_command_context_output_string("body",body);
    return 1;
}


