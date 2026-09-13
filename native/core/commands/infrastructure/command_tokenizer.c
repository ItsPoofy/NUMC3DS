#include "command_tokenizer.h"
#include "../../internal.h"

static int space(char value){return value==' '||(value>='\t'&&value<='\r');}

void command_tokens_destroy(CommandTokens *result){
    if(result->tokens)s->host.heap_free(result->tokens);
    if(result->storage)s->host.heap_free(result->storage);
    zero(result,sizeof(*result));
}

int command_tokenize(const char *input,CommandTokens *result){
    const char *at=input?input:"";unsigned length=0,capacity;char *out;
    zero(result,sizeof(*result));while(at[length])length++;
    capacity=length/2+1;
    result->tokens=s->host.heap_alloc(capacity*sizeof(McpeToken));
    result->storage=s->host.heap_alloc(length+2);
    if(!result->tokens||!result->storage){command_tokens_destroy(result);return 0;}
    out=result->storage;result->valid=1;
    while(*at){
        McpeToken *token;char quote=0;unsigned square=0,curly=0;
        while(space(*at))at++;
        if(!*at)break;
        token=&result->tokens[result->count++];zero(token,sizeof(*token));token->raw=at;token->text=out;
        token->is_selector=*at=='@'&&at[1]&&(at[1]=='p'||at[1]=='a'||at[1]=='r'||at[1]=='e'||at[1]=='s');
        token->is_quoted=*at=='"';
        if(token->is_quoted){
            at++;quote='"';
            while(*at&&*at!=quote){if(*at=='\\'&&at[1])at++;*out++=*at++;}
            if(*at==quote)at++;else result->valid=0;
            if(*at&&!space(*at))result->valid=0;
        }else{
            while(*at){
                char value=*at;
                if(!quote&&!square&&!curly&&space(value))break;
                if(quote){
                    if(value=='\\'&&at[1]){*out++=*at++;value=*at;}
                    else if(value==quote)quote=0;
                }else if(value=='"')quote=value;
                else if(value=='[')square++;
                else if(value==']'){if(square)square--;else result->valid=0;}
                else if(value=='{')curly++;
                else if(value=='}'){if(curly)curly--;else result->valid=0;}
                *out++=*at++;
            }
            if(quote||square||curly)result->valid=0;
        }
        token->length=(unsigned)(out-token->text);*out++=0;
    }
    return 1;
}
