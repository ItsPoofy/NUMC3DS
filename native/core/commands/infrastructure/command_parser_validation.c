#include "command_parser_validation.h"
#include "../native_parser.h"
#include "../native_output.h"

int command_parser_origin_allowed(void *origin,const char *text){
    const NativeSchemaMapEntry *entry=native_command_parser_matched_entry(text);
    const NativeSchemaRoot *root;
    const NativeSchemaVersion *versions;
    void **vtable=origin?*(void***)origin:0;
    int permissions;
    if(!entry||!vtable||!vtable[9])return 0;
    root=&native_schema_roots[entry->root_index];
    if(!root->version_count)return 0;
    versions=native_schema_root_versions(root);
    permissions=((int(*)(void*))vtable[9])(origin);
    return permissions>=versions[root->version_count-1].permission;
}

u32 command_parser_validate(void *parser,void *origin,NativeGstdString *message){
    const char *text=message&&message->handle?(const char*)message->handle:"";
    const NativeSchemaMapEntry *entry=native_command_parser_matched_entry(text);
    McpeOverloadList candidates;McpeScopedOverload result;unsigned index=0;
    if(!parser||!entry)return native_command_result_not_found();
    if(!command_parser_origin_allowed(origin,text))return native_command_result_permission();
    while(text[index]==' '||text[index]=='\t')index++;
    if(text[index]=='/')index++;
    {
        const char *name=native_schema_text(entry->name);
        while(*name&&text[index]){name++;index++;}
    }
    while(text[index]==' '||text[index]=='\t')index++;
    if(!mcpe_parser_get_overloads(native_schema_text(native_schema_roots[entry->root_index].name),0,&candidates))return native_command_result_not_found();
    return mcpe_parser_scope_overloads(&candidates,text+index,&result)?native_command_result_success():native_command_result_error();
}
