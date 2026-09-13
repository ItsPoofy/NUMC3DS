#include "../native_parser.h"
#include "command_tokenizer.h"
#include "command_parameter_match.h"
#include "command_parser_validation.h"
#include "../native_schema.h"
#include "../native_registry.h"

enum {
    NATIVE_PARSER_MAX_PARAMETERS = 16,
    NATIVE_PARSER_MAX_WORDS = 3,
    NATIVE_PARSER_VALUE_SLOTS = 10,
    NATIVE_PARSER_TOKEN_CAP = MAX_TEXT + 1,
    NATIVE_PARSER_MAX_TOKENS = 32
};

typedef struct {
    u32 status;
    NativeVector extractions;
    NativeIntellisenseCommandOverload *overload;
    NativeGstdString remaining;
} NativeCommandTestResults;

typedef void (*ExtractInputsFn)(void*,NativeCommandTestResults*,void*,NativeGstdString*);
typedef void *(*JsonMemberFn)(void*,NativeGstdString*);
typedef void *(*JsonStringFn)(void*,NativeGstdString*);
typedef void *(*JsonIntFn)(void*,int);
typedef void (*JsonAssignFn)(void*,void*);
typedef void (*TagDtorFn)(void*);

_Static_assert(sizeof(NativeCommandTestResults)==0x18,"NativeCommandTestResults ABI");



static int parser_space(char value){return value==' '||(value>='\t'&&value<='\r');}
static char parser_lower(char value){return value>='A'&&value<='Z'?(char)(value+'a'-'A'):value;}

static int parser_equal(const char *left,const char *right){if(!left)left="";if(!right)right="";while(*left&&*left==*right){left++;right++;}return *left==*right;}

static int parser_iequal(const char *left,const char *right){if(!left)left="";if(!right)right="";while(*left&&parser_lower(*left)==parser_lower(*right)){left++;right++;}return !*left&&!*right;}

static unsigned parser_entry_match(const char *message,const char *name){
    const char *text=message;const char *start;
    if(!text||!name||!*name)return 0;
    while(parser_space(*text))text++;
    if(*text=='/')text++;
    while(parser_space(*text))text++;
    start=text;
    while(*name){
        if(*name==' '){
            if(!parser_space(*text))return 0;
            while(parser_space(*text))text++;
        }else{
            if(parser_lower(*text)!=*name)return 0;
            text++;
        }
        name++;
    }
    return !*text||parser_space(*text)?(unsigned)(text-start):0;
}


const NativeSchemaMapEntry *native_command_parser_matched_entry(const char *message){
    const NativeSchemaMapEntry *best=0;unsigned best_length=0,index;
    if(!message)return 0;
    for(index=0;index<native_schema_map_entry_count;index++){
        const NativeSchemaMapEntry *entry=&native_schema_map_entries[index];
        unsigned length=parser_entry_match(message,native_schema_text(entry->name));
        if(length>best_length){best=entry;best_length=length;}
    }
    return best;
}

static const NativeSchemaRoot *parser_root_schema(const char *message,unsigned *matched){
    const NativeSchemaMapEntry *entry=native_command_parser_matched_entry(message);
    if(matched)*matched=entry?parser_entry_match(message,native_schema_text(entry->name)):0;
    return entry?&native_schema_roots[entry->root_index]:0;
}

const char *native_command_parser_root(const char *message){
    const NativeSchemaRoot *root=parser_root_schema(message,0);
    return root?native_schema_text(root->name):0;
}

static const char *parser_skip_space(const char *text){while(parser_space(*text))text++;return text;}

static unsigned parser_value_slot(u32 type,unsigned word_index){
    if(type==NATIVE_PARAM_BLOCKPOS)return word_index+2;
    if(type==NATIVE_PARAM_ROTATION||type==NATIVE_PARAM_TARGET)return word_index;
    return word_index+1;
}

static unsigned parser_value_slot_count(u32 type,unsigned word_count){
    unsigned slot_index,max_slot=0;
    for(slot_index=0;slot_index<word_count;slot_index++){
        unsigned slot=parser_value_slot(type,slot_index);
        if(slot>=max_slot)max_slot=slot+1;
    }
    if(type==NATIVE_PARAM_STRING&&max_slot<3)return 3;
    return max_slot;
}


static int parser_set_value(NativeGstdString *destination,const char *text){
    NativeGstdString scratch_str;u32 scratch=0;
    zero(&scratch_str,sizeof(scratch_str));
    ((StrCtor)SEAM_StrCtor)(&scratch_str,text?text:"",&scratch);
    if(!scratch_str.handle)return 0;
    ((StrAssign)SEAM_StrAssign)(destination,&scratch_str);
    ((StrDtor)SEAM_StrDtor)(&scratch_str);
    return destination->handle!=0;
}

static int parser_set_target_values(NativeGstdString values[NATIVE_PARSER_VALUE_SLOTS],const McpeToken *tok){
    char selector[2]={0,0},filters[NATIVE_PARSER_TOKEN_CAP];unsigned length=0;
    const char *token=tok->text;
    if(!token||!token[0])return 0;
    if(!tok->is_selector)return parser_set_value(&values[9],token);
    if(token[0]!='@'||!token[1])return 0;
    selector[0]=token[1];if(!parser_set_value(&values[3],selector))return 0;
    token+=2;
    if(!*token)return 1;
    if(*token++!='[')return 0;
    {
        char quote=0;
        while(*token&&length+1<sizeof(filters)){
            char c=*token;
            if(quote){
                if(c==quote)quote=0;
                else if(c=='\\'&&token[1])filters[length++]=*token++;
            }else{
                if(c=='\''||c=='\"')quote=c;
                else if(c==']')break;
            }
            filters[length++]=*token++;
        }
    }
    if(*token!=']'||token[1])return 0;
    filters[length]=0;return parser_set_value(&values[4],filters);
}


static void parser_drop_strings(NativeGstdString *strings,unsigned count){
    unsigned index;for(index=0;index<count;index++)if(strings[index].handle)((StrDtor)SEAM_StrDtor)(&strings[index]);
}

static int parser_make_parameter(NativeCommandParameter *destination,const NativeSchemaParameter *source){
    u32 scratch=0;
    zero(destination,sizeof(*destination));destination->type=source->type;destination->optional=native_schema_parameter_optional(source);
    destination->target_main=native_schema_parameter_target_main(source);destination->target_players_only=native_schema_parameter_players_only(source);
    ((StrCtor)SEAM_StrCtor)(&destination->name,native_schema_text(source->name),&scratch);scratch=0;
    ((StrCtor)SEAM_StrCtor)(&destination->enum_name,native_schema_text(source->enum_name),&scratch);
    return 1;
}

static void parser_drop_parameters(NativeCommandParameter *parameters,unsigned count){
    unsigned index;for(index=0;index<count;index++){
        if(parameters[index].name.handle)((StrDtor)SEAM_StrDtor)(&parameters[index].name);
        if(parameters[index].enum_name.handle)((StrDtor)SEAM_StrDtor)(&parameters[index].enum_name);
    }
}

static void parser_set_string(void *json,const char *key,NativeGstdString *text){
    NativeGstdString member_name;u32 scratch=0,value[4];void *member;
    zero(&member_name,sizeof(member_name));zero(value,sizeof(value));
    ((StrCtor)SEAM_StrCtor)(&member_name,key,&scratch);
    member=((JsonMemberFn)SEAM_Json_Value_getOrCreate)(json,&member_name);
    if(member){void *source=((JsonStringFn)SEAM_Json_Value_stdStringCtor)(value,text);((JsonAssignFn)SEAM_Json_Value_assign)(member,source);((TagDtorFn)SEAM_Tag_destructByType)(value);}
    ((StrDtor)SEAM_StrDtor)(&member_name);
}

static void parser_set_int(void *json,const char *key,int number){
    NativeGstdString member_name;u32 scratch=0,value[4];void *member;
    zero(&member_name,sizeof(member_name));zero(value,sizeof(value));
    ((StrCtor)SEAM_StrCtor)(&member_name,key,&scratch);
    member=((JsonMemberFn)SEAM_Json_Value_getOrCreate)(json,&member_name);
    if(member){void *source=((JsonIntFn)SEAM_Json_Value_intCtor)(value,number);((JsonAssignFn)SEAM_Json_Value_assign)(member,source);((TagDtorFn)SEAM_Tag_destructByType)(value);}
    ((StrDtor)SEAM_StrDtor)(&member_name);
}

/* Stage 1: MCPE 1.1.5 CommandParser::_getOverloads parity */
/* Stage 2: MCPE 1.1.5 CommandParser::_testOverload parity */
int mcpe_parser_test_overload(const char *arguments,const NativeSchemaOverload *schema,unsigned version_index,McpeScopedOverload *out){
    const NativeSchemaParameter *parameters=native_schema_overload_parameters(schema);
    CommandTokens tokenized;McpeToken *tokens;unsigned token_count;
    unsigned token_idx=0,param_idx=0;
    if(!out||!schema)return 0;
    zero(out,sizeof(*out));if(!command_tokenize(arguments,&tokenized))return 0;tokens=tokenized.tokens;token_count=tokenized.count;
    out->overload=schema;
    out->version_index=version_index;
    out->status=NATIVE_COMMAND_PARSE_SUCCESS;
    for(param_idx=0;param_idx<schema->parameter_count;param_idx++){
        const NativeSchemaParameter *param=&parameters[param_idx];
        int optional=native_schema_parameter_optional(param);
        if(token_idx>=token_count){
            if(optional)continue;
            out->status=NATIVE_COMMAND_PARSE_SYNTAX;
            break;
        }
        if(param->type==NATIVE_PARAM_RAWTEXT){
            token_idx=token_count;
            out->matched_parameters++;
            out->specificity_score+=1;
            continue;
        }
        if(param->type==NATIVE_PARAM_BLOCKPOS||param->type==NATIVE_PARAM_VEC3){
            if(token_idx+3>token_count){out->status=NATIVE_COMMAND_PARSE_SYNTAX;break;}
            if(!command_parameter_matches(param,&tokens[token_idx])||
               !command_parameter_matches(param,&tokens[token_idx+1])||
               !command_parameter_matches(param,&tokens[token_idx+2])){
                out->status=NATIVE_COMMAND_PARSE_SYNTAX;break;
            }
            token_idx+=3;
            out->matched_parameters++;
            out->specificity_score+=6;
            continue;
        }
        if(!command_parameter_matches(param,&tokens[token_idx])){
            out->status=NATIVE_COMMAND_PARSE_SYNTAX;
            break;
        }
        token_idx++;
        out->matched_parameters++;
        if(param->type==NATIVE_PARAM_STRING_ENUM)out->specificity_score+=10;
        else if(param->type==NATIVE_PARAM_TARGET)out->specificity_score+=8;
        else if(param->type==NATIVE_PARAM_INT)out->specificity_score+=5;
        else if(param->type==NATIVE_PARAM_FLOAT)out->specificity_score+=4;
        else if(param->type==NATIVE_PARAM_BOOL)out->specificity_score+=4;
        else out->specificity_score+=2;
    }
    if(out->status==NATIVE_COMMAND_PARSE_SUCCESS&&token_idx<token_count){
        out->status=NATIVE_COMMAND_PARSE_SYNTAX;
    }
    command_tokens_destroy(&tokenized);return out->status==NATIVE_COMMAND_PARSE_SUCCESS;
}

static int parser_emit_winning_overload(void *parser,const NativeSchemaRoot *root,const McpeScopedOverload *winning,const char *arguments,void *json){
    const NativeSchemaOverload *schema=winning->overload;
    const NativeSchemaParameter *schema_parameters=native_schema_overload_parameters(schema);
    NativeCommandParameter parameters[NATIVE_PARSER_MAX_PARAMETERS];
    NativeCommandExtraction extractions[NATIVE_PARSER_MAX_PARAMETERS];
    NativeGstdString values[NATIVE_PARSER_MAX_PARAMETERS][NATIVE_PARSER_VALUE_SLOTS];
    NativeIntellisenseCommandOverload candidate;NativeCommandTestResults result;
    NativeGstdString command_name,overload_name,token,input_name;
    CommandTokens tokenized;McpeToken *tokens;unsigned token_count;
    unsigned token_idx=0,parameter_index,extraction_count=0,slot_index;
    u32 scratch=0;
    if(!schema||schema->parameter_count>NATIVE_PARSER_MAX_PARAMETERS)return 0;if(!command_tokenize(arguments,&tokenized))return 0;tokens=tokenized.tokens;token_count=tokenized.count;
    zero(parameters,sizeof(parameters));zero(extractions,sizeof(extractions));zero(values,sizeof(values));
    zero(&candidate,sizeof(candidate));zero(&result,sizeof(result));zero(&command_name,sizeof(command_name));
    zero(&overload_name,sizeof(overload_name));zero(&token,sizeof(token));zero(&input_name,sizeof(input_name));
    for(parameter_index=0;parameter_index<schema->parameter_count;parameter_index++){
        const NativeSchemaParameter *param=&schema_parameters[parameter_index];
        unsigned slot_count,word_count=1;
        if(token_idx>=token_count){
            if(native_schema_parameter_optional(param))continue;
            goto cleanup;
        }
        if(param->type==NATIVE_PARAM_RAWTEXT){
            const char *raw_text=tokens[token_idx].raw;token_idx=token_count;
            slot_count=parser_value_slot_count(param->type,1);
            for(slot_index=0;slot_index<slot_count;slot_index++){
                scratch=0;((StrCtor)SEAM_StrCtor)(&values[extraction_count][slot_index],"",&scratch);
                if(!values[extraction_count][slot_index].handle)goto cleanup;
            }
            if(!parser_set_value(&values[extraction_count][parser_value_slot(param->type,0)],raw_text))goto cleanup;
        }else if(param->type==NATIVE_PARAM_BLOCKPOS||param->type==NATIVE_PARAM_VEC3){
            unsigned w;
            if(token_idx+3>token_count)goto cleanup;
            word_count=3;slot_count=parser_value_slot_count(param->type,3);
            for(slot_index=0;slot_index<slot_count;slot_index++){
                scratch=0;((StrCtor)SEAM_StrCtor)(&values[extraction_count][slot_index],"",&scratch);
                if(!values[extraction_count][slot_index].handle)goto cleanup;
            }
            for(w=0;w<3;w++){
                if(!parser_set_value(&values[extraction_count][parser_value_slot(param->type,w)],tokens[token_idx+w].text))goto cleanup;
            }
            token_idx+=3;
        }else if(param->type==NATIVE_PARAM_TARGET){
            slot_count=10;
            for(slot_index=0;slot_index<slot_count;slot_index++){
                scratch=0;((StrCtor)SEAM_StrCtor)(&values[extraction_count][slot_index],"",&scratch);
                if(!values[extraction_count][slot_index].handle)goto cleanup;
            }
            if(!parser_set_target_values(values[extraction_count],&tokens[token_idx]))goto cleanup;
            token_idx++;
        }else{
            slot_count=parser_value_slot_count(param->type,1);
            for(slot_index=0;slot_index<slot_count;slot_index++){
                scratch=0;((StrCtor)SEAM_StrCtor)(&values[extraction_count][slot_index],"",&scratch);
                if(!values[extraction_count][slot_index].handle)goto cleanup;
            }
            if(param->type==NATIVE_PARAM_BOOL){
                const char *val=tokens[token_idx].text;
                if(parser_equal(val,"1")||parser_iequal(val,"true"))val="true";
                else val="false";
                if(!parser_set_value(&values[extraction_count][parser_value_slot(param->type,0)],val))goto cleanup;
            }else{
                if(!parser_set_value(&values[extraction_count][parser_value_slot(param->type,0)],tokens[token_idx].text))goto cleanup;
            }
            if(param->type==NATIVE_PARAM_STRING){
                if(!parser_set_value(&values[extraction_count][2],tokens[token_idx].text))goto cleanup;
            }
            token_idx++;

        }
        extractions[extraction_count].values.begin=(u32)&values[extraction_count][0];
        extractions[extraction_count].values.end=(u32)&values[extraction_count][slot_count];
        extractions[extraction_count].values.capacity=extractions[extraction_count].values.end;
        extractions[extraction_count].type=param->type;
        scratch=0;((StrCtor)SEAM_StrCtor)(&extractions[extraction_count].name,native_schema_text(param->name),&scratch);
        if(!extractions[extraction_count].name.handle)goto cleanup;
        extraction_count++;
    }
    for(parameter_index=0;parameter_index<schema->parameter_count;parameter_index++){
        if(!parser_make_parameter(&parameters[parameter_index],&schema_parameters[parameter_index]))goto cleanup;
    }
    scratch=0;((StrCtor)SEAM_StrCtor)(&command_name,native_schema_text(root->name),&scratch);scratch=0;
    ((StrCtor)SEAM_StrCtor)(&overload_name,native_schema_text(schema->name),&scratch);scratch=0;
    ((StrCtor)SEAM_StrCtor)(&token,"",&scratch);scratch=0;
    ((StrCtor)SEAM_StrCtor)(&input_name,"input",&scratch);
    if(!command_name.handle||!overload_name.handle||!token.handle||!input_name.handle)goto cleanup;
    candidate.version=winning->version_index+1;candidate.command_name=command_name;candidate.overload_name=overload_name;candidate.token=token;
    candidate.input.begin=(u32)&parameters[0];candidate.input.end=(u32)&parameters[schema->parameter_count];candidate.input.capacity=candidate.input.end;
    result.status=2;result.extractions.begin=(u32)&extractions[0];result.extractions.end=(u32)&extractions[extraction_count];result.extractions.capacity=result.extractions.end;result.overload=&candidate;
    ((ExtractInputsFn)SEAM_CommandParser_extractInputs)(parser,&result,json,&input_name);
    parser_set_string(json,"name",&command_name);parser_set_string(json,"overload",&overload_name);parser_set_int(json,"version",(int)(winning->version_index+1));

    native_command_trace('Q');
    for(parameter_index=0;parameter_index<extraction_count;parameter_index++)if(extractions[parameter_index].name.handle)((StrDtor)SEAM_StrDtor)(&extractions[parameter_index].name);
    parser_drop_strings(&values[0][0],NATIVE_PARSER_MAX_PARAMETERS*NATIVE_PARSER_VALUE_SLOTS);parser_drop_parameters(parameters,schema->parameter_count);
    ((StrDtor)SEAM_StrDtor)(&input_name);((StrDtor)SEAM_StrDtor)(&token);((StrDtor)SEAM_StrDtor)(&overload_name);((StrDtor)SEAM_StrDtor)(&command_name);command_tokens_destroy(&tokenized);return 1;
cleanup:
    for(parameter_index=0;parameter_index<extraction_count;parameter_index++)if(extractions[parameter_index].name.handle)((StrDtor)SEAM_StrDtor)(&extractions[parameter_index].name);
    parser_drop_strings(&values[0][0],NATIVE_PARSER_MAX_PARAMETERS*NATIVE_PARSER_VALUE_SLOTS);parser_drop_parameters(parameters,schema->parameter_count);
    if(input_name.handle)((StrDtor)SEAM_StrDtor)(&input_name);if(token.handle)((StrDtor)SEAM_StrDtor)(&token);if(overload_name.handle)((StrDtor)SEAM_StrDtor)(&overload_name);if(command_name.handle)((StrDtor)SEAM_StrDtor)(&command_name);command_tokens_destroy(&tokenized);return 0;
}

void native_command_parser_get_command_json(void *parser,void *origin,NativeGstdString *message,void *json){
    const char *text=message&&message->handle?(const char*)message->handle:"";
    const NativeSchemaRoot *root;
    unsigned offset=0;
    McpeOverloadList candidates;
    McpeScopedOverload winning;
    root=parser_root_schema(text,&offset);
    if(!parser||!json||!root||!command_parser_origin_allowed(origin,text))return;
    text=parser_skip_space(text);if(*text=='/')text++;text=parser_skip_space(text);while(offset&&*text){text++;offset--;}
    text=parser_skip_space(text);
    if(!mcpe_parser_get_overloads(native_schema_text(root->name),0,&candidates)){
        native_command_trace('q');
        return;
    }
    if(mcpe_parser_scope_overloads(&candidates,text,&winning)){
        parser_emit_winning_overload(parser,root,&winning,text,json);
        return;
    }
    native_command_trace('q');
}
