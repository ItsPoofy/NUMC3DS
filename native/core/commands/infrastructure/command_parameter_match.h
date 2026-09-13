#ifndef NUMC3DS_COMMAND_PARAMETER_MATCH_H
#define NUMC3DS_COMMAND_PARAMETER_MATCH_H

#include "../native_schema.h"
#include "command_tokenizer.h"

int command_parameter_matches(const NativeSchemaParameter *parameter,const McpeToken *token);

#endif
