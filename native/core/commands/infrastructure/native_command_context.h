#ifndef NUMC3DS_NATIVE_COMMAND_CONTEXT_H
#define NUMC3DS_NATIVE_COMMAND_CONTEXT_H

#include "native_schema.h"

typedef struct {
    void *origin;
    void *input_bag;
    void *output_bag;
    void *player;
    void *level;
    void *game;
    const NativeSchemaOverload *schema;
} NativeCommandContext;

const NativeCommandContext *native_command_context_current(void);
const NativeCommandContext *native_command_context_push(const NativeCommandContext *context);
void native_command_context_restore(const NativeCommandContext *previous);
int native_command_context_blockpos(const char *name,int position[3]);
int native_command_context_origin_blockpos(int position[3]);
int native_command_context_rotation(const char *name,int axis,float *value);
unsigned native_command_context_targets(const char *name,void **out,unsigned max);
int native_command_context_has_target(const char *name);
void *native_command_context_block_source(void);
void native_command_context_output_string(const char *name,const char *value);
void native_command_context_output_int(const char *name,int value);
void native_command_context_output_float(const char *name,float value);
void native_command_context_output_bool(const char *name,int value);
void native_command_context_output_blockpos(const char *name,const int value[3]);
void native_command_context_output_entities(const char *name,void *const *entities,unsigned count);
void native_command_context_output_strings(const char *name,const char *const *values,unsigned count);

#endif
