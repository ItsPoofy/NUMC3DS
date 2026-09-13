#ifndef NUMC3DS_COMMAND_TOKENIZER_H
#define NUMC3DS_COMMAND_TOKENIZER_H

typedef struct {
    char *text;
    const char *raw;
    unsigned length;
    int is_quoted,is_selector;
} McpeToken;
typedef struct { McpeToken *tokens; char *storage; unsigned count; int valid; } CommandTokens;
int command_tokenize(const char *input,CommandTokens *result);
void command_tokens_destroy(CommandTokens *result);

#endif
