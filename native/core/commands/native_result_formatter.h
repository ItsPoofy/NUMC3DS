#ifndef NUMC3DS_NATIVE_COMMAND_RESULT_FORMATTER_H
#define NUMC3DS_NATIVE_COMMAND_RESULT_FORMATTER_H

#include "native_schema.h"

int native_command_overload_set_format_strings(NativeCommandOverload *overload,const NativeSchemaOverload *schema);
void native_command_format_schema_chat(const NativeSchemaOverload *schema, void *output_bag);

#endif
