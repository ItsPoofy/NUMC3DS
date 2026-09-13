#include "../native_parser.h"
#include "../../internal.h"

int mcpe_parser_get_overloads(const char *command_name,int version,McpeOverloadList *out){
    const NativeSchemaRoot *root;const NativeSchemaVersion *versions;
    unsigned selected_version_index,overload_index;
    if(!out)return 0;
    zero(out,sizeof(*out));
    {const NativeSchemaMapEntry *entry=native_command_parser_matched_entry(command_name);root=entry?&native_schema_roots[entry->root_index]:0;}
    if(!root||!root->version_count)return 0;
    versions=native_schema_root_versions(root);
    if(version==0)selected_version_index=root->version_count-1;
    else if(version>0&&(unsigned)version<=root->version_count)selected_version_index=(unsigned)version-1;
    else return 0;
    {
        const NativeSchemaVersion *ver=&versions[selected_version_index];
        const NativeSchemaOverload *overloads=native_schema_version_overloads(ver);
        for(overload_index=0;overload_index<ver->overload_count&&out->count<sizeof(out->candidates)/sizeof(out->candidates[0]);overload_index++){
            out->candidates[out->count].overload=&overloads[overload_index];
            out->candidates[out->count].version_index=selected_version_index;
            out->candidates[out->count].parameter_count=overloads[overload_index].parameter_count;
            out->count++;
        }
    }
    return out->count!=0;
}


int mcpe_parser_scope_overloads(const McpeOverloadList *candidates,const char *arguments,McpeScopedOverload *best_out){
    unsigned idx,max_success_params=0,max_error_params=0,success_count=0;
    McpeScopedOverload results[16];
    if(!candidates||!best_out)return 0;
    zero(best_out,sizeof(*best_out));
    best_out->status=NATIVE_COMMAND_PARSE_NOT_FOUND;
    for(idx=0;idx<candidates->count&&idx<16;idx++){
        mcpe_parser_test_overload(arguments,candidates->candidates[idx].overload,candidates->candidates[idx].version_index,&results[idx]);
        if(results[idx].status==NATIVE_COMMAND_PARSE_SUCCESS){
            success_count++;
            if(results[idx].matched_parameters>max_success_params)max_success_params=results[idx].matched_parameters;
        }else if(results[idx].status==NATIVE_COMMAND_PARSE_SYNTAX){
            if(results[idx].matched_parameters>max_error_params)max_error_params=results[idx].matched_parameters;
        }
    }
    if(success_count>0){
        for(idx=0;idx<candidates->count&&idx<16;idx++){
            if(results[idx].status==NATIVE_COMMAND_PARSE_SUCCESS&&results[idx].matched_parameters==max_success_params){
                *best_out=results[idx];return 1;
            }
        }
    }
    {
        int deepest=-1;
        for(idx=0;idx<candidates->count&&idx<16;idx++){
            if(results[idx].status==NATIVE_COMMAND_PARSE_SYNTAX&&results[idx].matched_parameters==max_error_params){
                deepest=(int)idx;
                break;
            }
        }
        if(deepest>=0)*best_out=results[deepest];
    }
    return 0;
}

