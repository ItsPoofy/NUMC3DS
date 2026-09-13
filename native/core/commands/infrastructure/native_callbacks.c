#include "../native_callbacks.h"
#include "../native_result_formatter.h"
#include "../native_command_context.h"
#include "../native_property_bag.h"
#include "../native_output.h"
#include "../native_registry.h"
#include "../virtual_command_origin.h"
#include "../command_origin_access.h"
#include "command_enum_registry.h"
#include "../../internal.h"

enum { EXECUTION_DEPTH_CAPACITY=8 };
static unsigned execution_depth;
static u8 execution_completed[EXECUTION_DEPTH_CAPACITY];
static u8 execution_succeeded[EXECUTION_DEPTH_CAPACITY];

static int callback_manager(void*destination,const void*source,int operation){
    if(operation==2&&destination&&source)cp(destination,source,sizeof(u32)*2);
    return 0;
}

int native_enum_callback_available(const NativeSchemaParameter *schema){
    return command_enum_available(schema);
}

static u32 enum_invoke(void*output,void*storage,void*origin){
    const NativeSchemaParameter*schema=storage?*(const NativeSchemaParameter**)storage:0;
    (void)origin;
    if(output)command_enum_build_vector(schema,(NativeVector*)output);
    return 0;
}

u32 native_callback_invoke_handler(void*storage,void*origin,void*input_bag,void*output_bag,NativeCommandHandlerFn handler){
    const NativeSchemaOverload*schema=storage?*(const NativeSchemaOverload**)storage:0;
    void*player,*level,*game;
    void*saved_output_bag;u32 saved_result,saved_native_callback;int result;NativeCommandContext context;const NativeCommandContext *previous_context;
    native_command_trace('V');
    if(!schema){native_command_trace('v');return native_command_result_error();}
    if(!origin){native_command_trace('o');return native_command_result_error();}
    level=native_command_origin_level(origin);player=native_command_origin_entity(origin);
    game=s->command_game?s->command_game:s->session_game;
    if(!level){native_command_trace('n');return native_command_result_error();}
    if(!game){native_command_trace('g');return native_command_result_error();}
    if(!handler){native_command_trace('h');return native_command_result_error();}
    native_command_trace('K');
    saved_output_bag=s->exec_output_bag;saved_result=s->exec_result;saved_native_callback=s->exec_native_callback;
    s->exec_output_bag=output_bag;s->exec_result=native_command_result_success();s->exec_native_callback=1;
    context.origin=origin;context.input_bag=input_bag;context.output_bag=output_bag;
    context.player=player;context.level=level;context.game=game;context.schema=schema;
    previous_context=native_command_context_push(&context);
    result=handler(player,level,game,"");
    native_command_context_restore(previous_context);
    native_command_trace(result?'L':'l');
    if(result!=0&&s->chat_command_active&&output_bag&&schema&&schema->format_count){
        native_command_format_schema_chat(schema,output_bag);
    }
    if(execution_depth&&execution_depth<=EXECUTION_DEPTH_CAPACITY){
        execution_completed[execution_depth-1]=1;
        execution_succeeded[execution_depth-1]=(u8)(result!=0);
    }
    if(!result)s->exec_result=native_command_result_error();
    result=s->exec_result;
    s->exec_output_bag=saved_output_bag;s->exec_result=saved_result;s->exec_native_callback=saved_native_callback;
    native_command_trace('M');
    return (u32)result;
}

void native_callback_init(NativeCommandCallback*callback,const NativeSchemaOverload*schema,NativeCommandInvokerFn invoker){
    zero(callback,sizeof(*callback));callback->storage[0]=(u32)schema;
    callback->manager=(u32)callback_manager;callback->invoker=(u32)invoker;
}

void native_enum_callback_init(NativeCommandCallback*callback,const NativeSchemaParameter*schema){
    zero(callback,sizeof(*callback));callback->storage[0]=(u32)schema;
    callback->manager=(u32)callback_manager;callback->invoker=(u32)enum_invoke;
}

static int native_command_execute_request(void **origin_ptr,void *player,void *level,void *game,const char *line){
    u32 message=0,error=0,message_scratch=0,error_scratch=0,result;void*commands,*saved_player,*saved_level,*saved_game;char command[MAX_TEXT+1];unsigned slot;
    if(!origin_ptr||!*origin_ptr||!game||!line||!line[0])return 0;
    commands=native_registry_commands_for_game(game);
    if(!commands||!native_registry_attach(commands,game))return 0;
    if(line[0]=='/')copy_text(command,line,sizeof(command));
    else{command[0]='/';copy_text(command+1,line,sizeof(command)-1);}
    ((StrCtor)SEAM_StrCtor)(&message,command,&message_scratch);
    ((StrCtor)SEAM_StrCtor)(&error,"",&error_scratch);
    saved_player=s->command_player;saved_level=s->command_level;saved_game=s->command_game;s->command_player=player;s->command_level=level;s->command_game=game;
    if(execution_depth>=EXECUTION_DEPTH_CAPACITY){s->command_player=saved_player;s->command_level=saved_level;s->command_game=saved_game;((StrDtor)SEAM_StrDtor)(&error);((StrDtor)SEAM_StrDtor)(&message);return 0;}
    slot=execution_depth++;execution_completed[slot]=0;execution_succeeded[slot]=0;
    native_command_trace('Q');
    result=((RequestCommandExecution)s->command_request.trampoline)(commands,origin_ptr,&message,&error);
    native_command_trace((u8)result==1?'R':'r');
    execution_depth--;
    s->command_player=saved_player;s->command_level=saved_level;s->command_game=saved_game;
    if((u8)result!=1){char*reason=0;cp(&reason,&error,sizeof(reason));result_text(reason&&reason[0]?reason:command_failed);}
    ((StrDtor)SEAM_StrDtor)(&error);
    ((StrDtor)SEAM_StrDtor)(&message);
    if(execution_completed[slot])return execution_succeeded[slot];
    return (u8)result==1;
}

int native_command_execute_line(void*player,void*level,void*game,const char*line){
    void *memory,*origin;int result;
    if(!player||!game||!line||!line[0])return 0;
    memory=((GameAllocWithSelector)SEAM_Heap_allocWithSelector)(0x38,(void*)SEAM_game_alloc_selector);
    if(!memory)return 0;
    origin=((PlayerCommandOriginCtor)SEAM_ClientCommandOrigin_ctor)(memory,player);
    if(!origin){((void(*)(void*))SEAM_operator_delete)(memory);return 0;}
    result=native_command_execute_request(&origin,player,level,game,line);
    if(origin)((void(*)(void*))(*(void***)origin)[1])(origin);
    return result;
}

int native_command_execute_line_with_origin(void **origin_ptr,void *player,void *level,void *game,const char *line){
    if(!origin_ptr||!*origin_ptr||!player||!level||!game||!line||!line[0])return 0;
    return native_command_execute_request(origin_ptr,player,level,game,line);
}

int native_command_execute_line_as_entity(void *base_origin,void *entity,const int position[3],void *level,void *game,const char *line){
    NativeVirtualCommandOriginScope scope;void *origin;int result;
    if(!base_origin||!entity||!position||!level||!game||!line||!line[0])return 0;
    zero(&scope,sizeof(scope));
    if(!native_virtual_command_origin_begin(&scope,base_origin,entity,level,position))return 0;
    origin=native_virtual_command_origin_get(&scope);
    result=native_command_execute_line_with_origin(&origin,entity,level,game,line);
    if(!origin)scope.origin=0;
    native_virtual_command_origin_end(&scope);
    return result;
}
