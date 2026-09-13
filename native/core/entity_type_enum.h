#ifndef NUMC3DS_ENTITY_TYPE_ENUM_H
#define NUMC3DS_ENTITY_TYPE_ENUM_H

#include "rt.h"

typedef void (*EntityTypeNameVisitor)(const char *,void *);

u32 entity_type_build_enum(void *output);
int entity_type_enum_contains(const char *name);
int entity_type_resolve_name(const char *name);
const char *entity_type_name_from_id(int entity_type);
void entity_type_visit_names(EntityTypeNameVisitor visitor,void *context);

#endif
