#ifndef NUMC3DS_COMMAND_PARSER_INTELLISENSE_H
#define NUMC3DS_COMMAND_PARSER_INTELLISENSE_H

#include "../native_schema.h"

typedef struct {
    void *player;
    void *level;
    int permission;
} CommandParserIntellisenseOrigin;

typedef struct {
    char *text;
    u32 start;
    u32 length;
} CommandParserIntellisenseInformation;

typedef struct {
    char *text;
    char *replacement;
    char *description;
    u32 match_start;
    u32 match_length;
    u32 replacement_start;
} CommandParserIntellisenseAutoCompleteOption;

typedef struct {
    int valid;
    CommandParserIntellisenseInformation *begin;
    CommandParserIntellisenseInformation *end;
    CommandParserIntellisenseInformation *capacity;
} CommandParserIntellisenseInformationResult;

typedef struct {
    int valid;
    CommandParserIntellisenseAutoCompleteOption *begin;
    CommandParserIntellisenseAutoCompleteOption *end;
    CommandParserIntellisenseAutoCompleteOption *capacity;
    const NativeSchemaParameter *parameter;
} CommandParserIntellisenseAutoCompleteInformation;

_Static_assert(sizeof(CommandParserIntellisenseInformation)==0x0c,"CommandParser IntellisenseInformation ABI");
_Static_assert(sizeof(CommandParserIntellisenseAutoCompleteOption)==0x18,"CommandParser AutoCompleteOption ABI");
_Static_assert(sizeof(CommandParserIntellisenseInformationResult)==0x10,"CommandParser IntellisenseInformation result ABI");
_Static_assert(sizeof(CommandParserIntellisenseAutoCompleteInformation)==0x14,"CommandParser AutoCompleteInformation ABI");

void command_parser_get_intellisense_information(
    const CommandParserIntellisenseOrigin *origin,const char *input,u32 cursor,
    CommandParserIntellisenseInformationResult *result);
void command_parser_get_auto_complete_options(
    const CommandParserIntellisenseOrigin *origin,const char *input,u32 cursor,
    CommandParserIntellisenseAutoCompleteInformation *result);
void command_parser_intellisense_information_destroy(CommandParserIntellisenseInformationResult *result);
void command_parser_auto_complete_information_destroy(CommandParserIntellisenseAutoCompleteInformation *result);

#endif
