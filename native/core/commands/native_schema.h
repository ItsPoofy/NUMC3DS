#ifndef NUMC3DS_NATIVE_COMMAND_SCHEMA_H
#define NUMC3DS_NATIVE_COMMAND_SCHEMA_H

#include "native_types.h"

enum {
    NATIVE_PARAM_FLOAT = 0,
    NATIVE_PARAM_INT = 1,
    NATIVE_PARAM_STRING = 2,
    NATIVE_PARAM_STRING_ENUM = 3,
    NATIVE_PARAM_RAWTEXT = 4,
    NATIVE_PARAM_BLOCKPOS = 5,
    NATIVE_PARAM_TARGET = 6,
    NATIVE_PARAM_BOOL = 7,
    NATIVE_PARAM_ROTATION = 8,
    NATIVE_PARAM_RESULT_LIST = 9,
    NATIVE_PARAM_VEC3 = 10,
    NATIVE_PARAM_COMPONENTS = 11
};

typedef struct {
    u16 name;
    u16 enum_name;
    u16 enum_values;
    u8 type;
    u8 flags;
} NativeSchemaParameter;

typedef struct {
    u16 name;
    u16 usage;
    u16 dispatch;
    u16 prefix;
    u16 token;
    u16 parameter_start;
    u16 output_start;
    u16 format_start;
    u8 parameter_count;
    u8 output_count;
    u8 format_count;
    u8 flags;
} NativeSchemaOverload;

typedef struct {
    u16 format;
    u16 color;
    u16 parameter_name_start;
    u16 condition_start;
    u8 parameter_name_count;
    u8 condition_count;
} NativeSchemaFormatString;

typedef struct {
    u16 name;
    u8 kind;
    u8 parameter_type;
} NativeSchemaFormatCondition;

typedef struct NativeSchemaVersion {
    u16 description;
    u16 overload_start;
    u8 overload_count;
    u8 permission;
    u8 flags;
    u8 reserved;
} NativeSchemaVersion;

typedef struct {
    u16 name;
    u16 version_start;
    u8 version_count;
    u8 reserved[3];
} NativeSchemaRoot;

typedef struct NativeSchemaMapEntry {
    u16 name;
    u8 root_index;
    u8 reserved;
} NativeSchemaMapEntry;

enum {
    NATIVE_SCHEMA_PARAMETER_OPTIONAL = 1,
    NATIVE_SCHEMA_PARAMETER_TARGET_MAIN = 2,
    NATIVE_SCHEMA_PARAMETER_PLAYERS_ONLY = 4,
    NATIVE_SCHEMA_OVERLOAD_OUTPUT_SPEECH = 1,
    NATIVE_SCHEMA_FORMAT_NOT_EMPTY = 1,
    NATIVE_SCHEMA_FORMAT_IS_TRUE = 2,
    NATIVE_SCHEMA_VERSION_CHAT_PERMISSION = 1,
    NATIVE_SCHEMA_VERSION_TELL_PERMISSION = 2,
    NATIVE_SCHEMA_VERSION_HIDDEN = 4,
    NATIVE_SCHEMA_VERSION_ALLOWS_INDIRECT = 8,
    NATIVE_SCHEMA_VERSION_REQUIRES_EDU = 16
};

extern const char native_schema_strings[];
extern const NativeSchemaParameter native_schema_parameters[];
extern const NativeSchemaOverload native_schema_overloads[];
extern const NativeSchemaFormatString native_schema_format_strings[];
extern const NativeSchemaFormatCondition native_schema_format_conditions[];
extern const u16 native_schema_format_parameter_names[];
extern const NativeSchemaVersion native_schema_versions[];
extern const NativeSchemaRoot native_schema_roots[];
extern const unsigned native_schema_root_count;
extern const NativeSchemaMapEntry native_schema_map_entries[];
extern const unsigned native_schema_map_entry_count;

static inline const char *native_schema_text(u16 offset){return native_schema_strings+offset;}
static inline const NativeSchemaParameter *native_schema_overload_parameters(const NativeSchemaOverload*overload){return native_schema_parameters+overload->parameter_start;}
static inline const NativeSchemaParameter *native_schema_overload_outputs(const NativeSchemaOverload*overload){return native_schema_parameters+overload->output_start;}
static inline const NativeSchemaFormatString *native_schema_overload_formats(const NativeSchemaOverload*overload){return native_schema_format_strings+overload->format_start;}
static inline const NativeSchemaOverload *native_schema_version_overloads(const NativeSchemaVersion*version){return native_schema_overloads+version->overload_start;}
static inline const NativeSchemaVersion *native_schema_root_versions(const NativeSchemaRoot*root){return native_schema_versions+root->version_start;}
static inline int native_schema_parameter_optional(const NativeSchemaParameter*parameter){return (parameter->flags&NATIVE_SCHEMA_PARAMETER_OPTIONAL)!=0;}
static inline int native_schema_parameter_target_main(const NativeSchemaParameter*parameter){return (parameter->flags&NATIVE_SCHEMA_PARAMETER_TARGET_MAIN)!=0;}
static inline int native_schema_parameter_players_only(const NativeSchemaParameter*parameter){return (parameter->flags&NATIVE_SCHEMA_PARAMETER_PLAYERS_ONLY)!=0;}
static inline int native_schema_overload_flag(const NativeSchemaOverload*overload,u8 flag){return (overload->flags&flag)!=0;}
static inline int native_schema_version_flag(const NativeSchemaVersion*version,u8 flag){return (version->flags&flag)!=0;}
static inline int native_schema_root_visible(const NativeSchemaRoot*root){
    unsigned index;
    const NativeSchemaVersion*versions=native_schema_root_versions(root);
    for(index=0;index<root->version_count;index++)if(!native_schema_version_flag(&versions[index],NATIVE_SCHEMA_VERSION_HIDDEN))return 1;
    return 0;
}

_Static_assert(sizeof(NativeSchemaParameter)==0x08,"NativeSchemaParameter compact ABI");
_Static_assert(sizeof(NativeSchemaOverload)==0x14,"NativeSchemaOverload compact ABI");
_Static_assert(sizeof(NativeSchemaFormatString)==0x0a,"NativeSchemaFormatString compact ABI");
_Static_assert(sizeof(NativeSchemaFormatCondition)==0x04,"NativeSchemaFormatCondition compact ABI");
_Static_assert(sizeof(NativeSchemaVersion)==0x08,"NativeSchemaVersion compact ABI");
_Static_assert(sizeof(NativeSchemaRoot)==0x08,"NativeSchemaRoot compact ABI");
_Static_assert(sizeof(NativeSchemaMapEntry)==0x04,"NativeSchemaMapEntry compact ABI");

#endif
