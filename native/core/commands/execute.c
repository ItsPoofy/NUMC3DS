#include "../internal.h"
#include "native_callbacks.h"
#include "native_command_context.h"
#include "native_property_bag.h"
#include "virtual_command_origin.h"
#include "command_origin_access.h"
int cmd_execute(void*player,void*level,void*game,const char*p){
    const NativeCommandContext *context=native_command_context_current();
    const char *command,*detect_block;char buf[MAX_TEXT+1];
    unsigned n=0,count,i;int r;
    void*targets[SELECTOR_TARGET_CAPACITY],*failed_entities[SELECTOR_TARGET_CAPACITY];
    unsigned failed_count=0,success_count=0;
    int detect=0,position[3],detect_position[3],detect_data=0;unsigned char detect_id=0;
    (void)p;
    if(!context||!native_command_context_blockpos("position",position))return fail_syntax();
    count=native_command_context_targets("origin",targets,SELECTOR_TARGET_CAPACITY);
    if(!count)return fail_command_localized("commands.generic.noTargetMatch");
    command=native_bag_get_string(context->input_bag,"command");
    if(!command||!command[0])return fail_syntax();
    native_command_context_output_string("command",command);
    detect_block=native_bag_get_string(context->input_bag,"detectBlock");
    if(detect_block){
        if(!native_command_context_blockpos("detectPos",detect_position)||!resolve_block_arg((char*)detect_block,&detect_id))return fail_syntax();
        if(!native_bag_get_int(context->input_bag,"detectData",&detect_data)||detect_data<0||detect_data>15)return fail_syntax();
        detect=1;
    }
    for(i=0;i<count;i++){
        NativeVirtualCommandOriginScope scope;int run=1;
        void *ent=targets[i];
        if(!ent)continue;
        zero(&scope,sizeof(scope));
        if(!native_virtual_command_origin_begin(&scope,context->origin,ent,level,position)){
            failed_entities[failed_count++]=ent;
            continue;
        }
        if(detect){
            void *target_source=native_command_origin_block_source(native_virtual_command_origin_get(&scope));
            if(!target_source)run=0;
            else{
                unsigned char cur=command_block_read_id(target_source,detect_position);
                run=(cur==detect_id&&command_block_read_data(target_source,detect_position)==(unsigned char)detect_data);
            }
        }
        if(run){
            void *vorigin=native_virtual_command_origin_get(&scope);
            n=0;buf[0]='/';n=1;append(buf,&n,command);
            r=native_command_execute_line_with_origin(&vorigin,ent,level,game,buf);
            if(!vorigin)scope.origin=0;
        }else r=0;
        native_virtual_command_origin_end(&scope);
        if(!r)failed_entities[failed_count++]=ent;
        else success_count++;
    }
    if(failed_count)native_command_context_output_entities("failedEntities",failed_entities,failed_count);
    return success_count>0;
}


