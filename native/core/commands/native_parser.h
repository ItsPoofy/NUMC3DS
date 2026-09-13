#ifndef NUMC3DS_NATIVE_COMMAND_PARSER_H
#define NUMC3DS_NATIVE_COMMAND_PARSER_H

#include "native_types.h"
#include "native_schema.h"

enum {
    NATIVE_COMMAND_PARSE_NOT_FOUND = 0,
    NATIVE_COMMAND_PARSE_SYNTAX = 1,
    NATIVE_COMMAND_PARSE_SUCCESS = 2
};

/* MCPE 1.1.5 CommandParser Parity Stage Structures */
typedef struct {
    const NativeSchemaOverload *overload;
    unsigned version_index;
    unsigned parameter_count;
} McpeOverloadCandidate;

typedef struct {
    McpeOverloadCandidate candidates[16];
    unsigned count;
} McpeOverloadList;

typedef struct {
    int status;
    const NativeSchemaOverload *overload;
    unsigned version_index;
    unsigned matched_parameters;
    unsigned specificity_score;
} McpeScopedOverload;

const NativeSchemaMapEntry *native_command_parser_matched_entry(const char *message);
const char *native_command_parser_root(const char *message);
void native_command_parser_get_command_json(void *parser,void *origin,NativeGstdString *message,void *json);

/* MCPE 1.1.5 Stage Functions */
int mcpe_parser_get_overloads(const char *command_name,int version,McpeOverloadList *out);
int mcpe_parser_test_overload(const char *arguments,const NativeSchemaOverload *schema,unsigned version_index,McpeScopedOverload *out);
int mcpe_parser_scope_overloads(const McpeOverloadList *candidates,const char *arguments,McpeScopedOverload *best_out);

#endif


