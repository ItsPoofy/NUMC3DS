#ifndef NUMC3DS_NATIVE_COMMAND_TYPES_H
#define NUMC3DS_NATIVE_COMMAND_TYPES_H

#include "../rt.h"

typedef struct {
    u32 handle;
} NativeGstdString;

typedef struct {
    u8 storage[0x14];
} NativeJsonValue;

typedef struct {
    u32 object;
    u32 control;
} NativeSharedCommand;

typedef struct {
    u32 begin;
    u32 end;
    u32 capacity;
} NativeVector;

typedef struct {
    NativeVector values;
    u32 type;
    NativeGstdString name;
} NativeCommandExtraction;

typedef struct {
    void *origin;
    u8 selector_mode;
    u8 reserved_05[3];
    NativeVector resolvers;
    NativeVector matches;
    u8 resolved;
    u8 reserved_21[3];
} NativeCommandTarget;

typedef struct {
    u32 storage[2];
    u32 manager;
    u32 invoker;
} NativeCommandCallback;

typedef struct {
    u32 type;
    NativeGstdString name;
    u8 optional;
    u8 target_main;
    u8 target_players_only;
    u8 target_reserved;
    NativeGstdString enum_name;
} NativeCommandParameter;

typedef struct {
    NativeGstdString name;
    NativeGstdString usage;
    NativeGstdString compact_usage;
    NativeVector input;
    NativeVector output;
    NativeVector callbacks;
    u32 formatter;
    u8 required_parameter_count;
    u8 reserved_35[3];
    NativeGstdString step_name;
    u8 output_to_speech;
    u8 reserved_3d[3];
} NativeCommandOverload;

typedef struct {
    NativeGstdString name;
    u8 reserved_04;
    u8 permission;
    u8 reserved_06[2];
    u8 player_origin_policy;
    u8 reserved_09[3];
    NativeGstdString description;
    NativeGstdString usage;
    u8 flags_14;
    u8 requires_edu;
    u8 requires_chat_permission;
    u8 requires_tell_permission;
    u8 is_hidden;
    u8 allows_indirect_exec;
    u8 reserved_1a[2];
    NativeVector overloads;
} NativeCommand;

typedef struct {
    u32 version;
    NativeGstdString command_name;
    NativeGstdString overload_name;
    NativeGstdString token;
    NativeVector input;
    NativeGstdString usage;
    NativeVector extractions_or_output;
    u32 reserved_2c;
} NativeIntellisenseCommandOverload;

typedef struct NativeCommandMapNode {
    u8 color;
    u8 reserved_01[3];
    struct NativeCommandMapNode *parent;
    struct NativeCommandMapNode *left;
    struct NativeCommandMapNode *right;
    NativeGstdString key;
    NativeVector commands;
} NativeCommandMapNode;

typedef struct NativeCommandNameMapNode {
    u8 color;
    u8 reserved_01[3];
    struct NativeCommandNameMapNode *parent;
    struct NativeCommandNameMapNode *left;
    struct NativeCommandNameMapNode *right;
    NativeGstdString key;
    NativeGstdString value;
} NativeCommandNameMapNode;

typedef struct {
    void *blocks;
    NativeCommandMapNode *free_list;
    u8 *next;
    u8 *end;
    NativeCommandMapNode *header;
    u32 count;
    u8 reserved_18;
    u8 reserved_19;
    u8 reserved_1a[2];
} NativeCommandMap;

_Static_assert(sizeof(NativeGstdString)==0x04,"NativeGstdString ABI");
_Static_assert(sizeof(NativeJsonValue)==0x14,"NativeJsonValue ABI");
_Static_assert(sizeof(NativeCommandExtraction)==0x14,"NativeCommandExtraction ABI");
_Static_assert(sizeof(NativeCommandTarget)==0x24,"NativeCommandTarget ABI");
_Static_assert(sizeof(NativeSharedCommand)==0x08,"NativeSharedCommand ABI");
_Static_assert(sizeof(NativeCommandCallback)==0x10,"NativeCommandCallback ABI");
_Static_assert(sizeof(NativeCommandParameter)==0x10,"NativeCommandParameter ABI");
_Static_assert(sizeof(NativeCommandOverload)==0x40,"NativeCommandOverload ABI");
_Static_assert(sizeof(NativeCommand)==0x28,"NativeCommand ABI");
_Static_assert(sizeof(NativeIntellisenseCommandOverload)==0x30,"NativeIntellisenseCommandOverload ABI");
_Static_assert(sizeof(NativeCommandMapNode)==0x20,"NativeCommandMapNode ABI");
_Static_assert(sizeof(NativeCommandNameMapNode)==0x18,"NativeCommandNameMapNode ABI");
_Static_assert(sizeof(NativeCommandMap)==0x1c,"NativeCommandMap ABI");

#endif
