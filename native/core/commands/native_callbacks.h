#ifndef NUMC3DS_NATIVE_COMMAND_CALLBACKS_H
#define NUMC3DS_NATIVE_COMMAND_CALLBACKS_H

#include "native_schema.h"

typedef int (*NativeCommandHandlerFn)(void*,void*,void*,const char*);
typedef u32 (*NativeCommandInvokerFn)(void*,void*,void*,void*);

void native_callback_init(NativeCommandCallback *callback,const NativeSchemaOverload *schema,NativeCommandInvokerFn invoker);
u32 native_callback_invoke_handler(void *storage,void *origin,void *input_bag,void *output_bag,NativeCommandHandlerFn handler);
void native_enum_callback_init(NativeCommandCallback *callback,const NativeSchemaParameter *schema);
int native_enum_callback_available(const NativeSchemaParameter *schema);
int native_command_execute_line(void *player,void *level,void *game,const char *line);
int native_command_execute_line_with_origin(void **origin_ptr,void *player,void *level,void *game,const char *line);
int native_command_execute_line_as_entity(void *origin,void *entity,const int position[3],void *level,void *game,const char *line);

#endif
