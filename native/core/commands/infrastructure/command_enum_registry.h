#ifndef NUMC3DS_COMMAND_ENUM_REGISTRY_H
#define NUMC3DS_COMMAND_ENUM_REGISTRY_H

#include "../native_schema.h"

typedef void (*CommandEnumVisitor)(const char *value,void *context);

int command_enum_available(const NativeSchemaParameter *schema);
int command_enum_build_vector(const NativeSchemaParameter *schema,NativeVector *output);
void command_enum_visit(const NativeSchemaParameter *schema,CommandEnumVisitor visitor,void *context);

#endif
