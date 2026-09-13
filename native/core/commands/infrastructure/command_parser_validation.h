#ifndef NUMC3DS_COMMAND_PARSER_VALIDATION_H
#define NUMC3DS_COMMAND_PARSER_VALIDATION_H
#include "../native_types.h"
u32 command_parser_validate(void *parser,void *origin,NativeGstdString *message);
int command_parser_origin_allowed(void *origin,const char *text);
#endif
