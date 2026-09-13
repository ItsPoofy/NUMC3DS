#include "command_parser_intellisense.h"
#include "../native_parser.h"
#include "../infrastructure/command_enum_registry.h"
#include "../infrastructure/command_parameter_match.h"
#include "../infrastructure/command_tokenizer.h"
#include "../../internal.h"

typedef struct {
    CommandParserIntellisenseAutoCompleteInformation *result;
    const char *prefix;
    u32 prefix_length;
    u32 replacement_start;
} EnumOptionContext;

static int is_space(char value){return value==' '||(value>='\t'&&value<='\r');}
static char fold(char value){return value>='A'&&value<='Z'?(char)(value+'a'-'A'):value;}

static int prefix_fold(const char *value,const char *prefix){
    while(*prefix){if(!*value||fold(*value++)!=fold(*prefix++))return 0;}
    return 1;
}

static int equal_fold(const char *left,const char *right){
    while(*left&&*right){if(fold(*left++)!=fold(*right++))return 0;}
    return !*left&&!*right;
}

static int text_compare(const char *left,const char *right){
    while(*left&&*right&&*left==*right){left++;right++;}
    return (unsigned char)*left-(unsigned char)*right;
}

static int find_fold(const char *value,const char *fragment,u32 *position){
    u32 at=0,scan;
    if(!fragment[0]){*position=0;return 1;}
    while(value[at]){
        scan=0;while(fragment[scan]&&value[at+scan]&&fold(value[at+scan])==fold(fragment[scan]))scan++;
        if(!fragment[scan]){*position=at;return 1;}at++;
    }
    return 0;
}

static char *duplicate(const char *text){
    unsigned length=text_len(text?text:"");char *copy=s->host.heap_alloc(length+1);
    if(!copy)return 0;
    cp(copy,text?text:"",length);copy[length]=0;return copy;
}

static void free_option(CommandParserIntellisenseAutoCompleteOption *option){
    if(option->text)s->host.heap_free(option->text);
    if(option->replacement)s->host.heap_free(option->replacement);
    if(option->description)s->host.heap_free(option->description);
    zero(option,sizeof(*option));
}

static int option_exists(CommandParserIntellisenseAutoCompleteInformation *result,const char *text){
    CommandParserIntellisenseAutoCompleteOption *at=result->begin;
    while(at&&at!=result->end){if(equal_fold(at->text,text))return 1;at++;}
    return 0;
}

static int grow_options(CommandParserIntellisenseAutoCompleteInformation *result){
    unsigned count=result->begin?(unsigned)(result->end-result->begin):0,capacity=result->begin?(unsigned)(result->capacity-result->begin):0,next=capacity?capacity*2:16;
    CommandParserIntellisenseAutoCompleteOption *items=s->host.heap_alloc(next*sizeof(*items));
    if(!items)return 0;
    zero(items,next*sizeof(*items));if(count)cp(items,result->begin,count*sizeof(*items));if(result->begin)s->host.heap_free(result->begin);
    result->begin=items;result->end=items+count;result->capacity=items+next;return 1;
}

static int add_option(CommandParserIntellisenseAutoCompleteInformation *result,const char *text,const char *description,
    const char *replacement,u32 match_start,u32 match_length,u32 replacement_start){
    CommandParserIntellisenseAutoCompleteOption *option;
    if(!text||!text[0]||option_exists(result,text))return 1;
    if(result->end==result->capacity&&!grow_options(result))return 0;
    option=result->end;zero(option,sizeof(*option));
    option->text=duplicate(text);option->description=duplicate(description?description:"");
    option->replacement=duplicate(replacement&&replacement[0]?replacement:text);
    if(!option->text||!option->description||!option->replacement){free_option(option);return 0;}
    option->match_start=match_start;option->match_length=match_length;option->replacement_start=replacement_start;
    result->end++;return 1;
}

static void sort_options(CommandParserIntellisenseAutoCompleteInformation *result){
    CommandParserIntellisenseAutoCompleteOption *at,*scan,value;
    if(!result->begin)return;
    for(at=result->begin+1;at<result->end;at++){value=*at;scan=at;while(scan>result->begin&&value.match_start<(scan-1)->match_start){*scan=*(scan-1);scan--;}*scan=value;}
}

static int grow_information(CommandParserIntellisenseInformationResult *result){
    unsigned count=result->begin?(unsigned)(result->end-result->begin):0,capacity=result->begin?(unsigned)(result->capacity-result->begin):0,next=capacity?capacity*2:8;
    CommandParserIntellisenseInformation *items=s->host.heap_alloc(next*sizeof(*items));
    if(!items)return 0;
    zero(items,next*sizeof(*items));if(count)cp(items,result->begin,count*sizeof(*items));if(result->begin)s->host.heap_free(result->begin);
    result->begin=items;result->end=items+count;result->capacity=items+next;return 1;
}

static int add_information(CommandParserIntellisenseInformationResult *result,const NativeSchemaOverload *overload,const NativeSchemaParameter *active){
    const char *usage=native_schema_text(overload->usage),*name=active?native_schema_text(active->name):"";CommandParserIntellisenseInformation *information,*at;u32 start=0,length=0,name_length=text_len(name);
    if(name[0]){const char *at=usage;while(*at){char close=*at=='<'?'>':*at=='['?']':0;if(close&&prefix_fold(at+1,name)&&at[name_length+1]==close){start=(u32)(at-usage);length=name_length+2;break;}at++;}}
    for(at=result->begin;at&&at!=result->end;at++)if(equal_fold(at->text,usage)){if(!at->length&&length){at->start=start;at->length=length;}return 1;}
    if(result->end==result->capacity&&!grow_information(result))return 0;
    information=result->end;information->text=duplicate(usage);if(!information->text)return 0;information->start=start;information->length=length;result->end++;return 1;
}

static int add_information_text(CommandParserIntellisenseInformationResult *result,const char *text,u32 start,u32 length){
    CommandParserIntellisenseInformation *information;if(result->end==result->capacity&&!grow_information(result))return 0;information=result->end;information->text=duplicate(text);if(!information->text)return 0;information->start=start;information->length=length;result->end++;return 1;
}

static void sort_information(CommandParserIntellisenseInformationResult *result){
    CommandParserIntellisenseInformation *at,*scan,value;
    if(!result->begin)return;
    for(at=result->begin+1;at<result->end;at++){value=*at;scan=at;while(scan>result->begin&&text_compare(value.text,(scan-1)->text)<0){*scan=*(scan-1);scan--;}*scan=value;}
}

static const NativeSchemaVersion *visible_version(const NativeSchemaRoot *root,int permission){
    const NativeSchemaVersion *versions=native_schema_root_versions(root);unsigned index=root->version_count;
    while(index){const NativeSchemaVersion *version=&versions[--index];if(version->permission<=permission&&!native_schema_version_flag(version,NATIVE_SCHEMA_VERSION_HIDDEN))return version;}
    return 0;
}

static u32 command_start(const char *input){u32 at=0;while(is_space(input[at]))at++;if(input[at]=='/')at++;while(is_space(input[at]))at++;return at;}

static u32 argument_start(const char *input,const char *name){
    u32 at=command_start(input);while(*name){if(*name==' '){if(!is_space(input[at]))return at;while(is_space(input[at]))at++;}else{if(fold(input[at])!=fold(*name))return at;at++;}name++;}return at;
}

static void token_range(const char *input,u32 base,u32 cursor,u32 *start_out,u32 *end_out){
    u32 at=base,start=base,end=cursor,square=0,curly=0;char quote=0;
    while(at<cursor&&input[at]){char value=input[at];if(quote){if(value=='\\'&&input[at+1])at++;else if(value==quote)quote=0;}else if(value=='"'||value=='\'')quote=value;else if(value=='[')square++;else if(value==']'&&square)square--;else if(value=='{')curly++;else if(value=='}'&&curly)curly--;else if(!square&&!curly&&is_space(value)){while(at+1<cursor&&is_space(input[at+1]))at++;start=at+1;}at++;}
    while(input[end]){char value=input[end];if(quote){if(value=='\\'&&input[end+1])end++;else if(value==quote)quote=0;}else if(value=='"'||value=='\'')quote=value;else if(value=='[')square++;else if(value==']'&&square)square--;else if(value=='{')curly++;else if(value=='}'&&curly)curly--;else if(!square&&!curly&&is_space(value))break;end++;}
    *start_out=start;*end_out=end;
}

static u32 active_parameter_mask_from(const NativeSchemaParameter *parameters,u32 parameter_count,
    const CommandTokens *tokens,u32 parameter_index,u32 token_index){
    const NativeSchemaParameter *parameter;u32 mask=0,width,index;
    if(parameter_index>=parameter_count)return 0;
    if(token_index>=tokens->count){
        for(index=parameter_index;index<parameter_count&&index<32;index++){
            mask|=1u<<index;
            if(!native_schema_parameter_optional(&parameters[index]))break;
        }
        return mask;
    }
    parameter=&parameters[parameter_index];
    if(native_schema_parameter_optional(parameter))mask|=active_parameter_mask_from(parameters,parameter_count,tokens,parameter_index+1,token_index);
    if(parameter->type==NATIVE_PARAM_RAWTEXT)return mask;
    width=(parameter->type==NATIVE_PARAM_BLOCKPOS||parameter->type==NATIVE_PARAM_VEC3)?3:1;
    if(token_index+width>tokens->count)return mask|(1u<<parameter_index);
    for(index=0;index<width;index++)if(!command_parameter_matches(parameter,&tokens->tokens[token_index+index]))return mask;
    return mask|active_parameter_mask_from(parameters,parameter_count,tokens,parameter_index+1,token_index+width);
}

static u32 active_parameter_mask(const NativeSchemaOverload *overload,const char *complete){
    CommandTokens tokens;u32 mask;
    if(!command_tokenize(complete,&tokens))return 0;
    mask=active_parameter_mask_from(native_schema_overload_parameters(overload),overload->parameter_count,&tokens,0,0);
    command_tokens_destroy(&tokens);return mask;
}

static void add_enum_value(const char *value,void *opaque){
    EnumOptionContext *context=(EnumOptionContext*)opaque;u32 match_start;
    if(find_fold(value,context->prefix,&match_start))add_option(context->result,value,"",value,match_start,context->prefix_length,context->replacement_start);
}

static void add_target_options(const CommandParserIntellisenseOrigin *origin,const NativeSchemaParameter *parameter,const char *prefix,u32 prefix_length,u32 replacement_start,CommandParserIntellisenseAutoCompleteInformation *result){
    void *targets[SELECTOR_TARGET_CAPACITY];unsigned count=0,index;char name[40],replacement[43];u32 match_start;
    if(find_fold("@a",prefix,&match_start))add_option(result,"@a","all players","@a",match_start,prefix_length,replacement_start);
    if(!native_schema_parameter_players_only(parameter)&&find_fold("@e",prefix,&match_start))add_option(result,"@e","all entities","@e",match_start,prefix_length,replacement_start);
    if(find_fold("@r",prefix,&match_start))add_option(result,"@r","random player(s)","@r",match_start,prefix_length,replacement_start);
    if(find_fold("@p",prefix,&match_start))add_option(result,"@p","nearest player","@p",match_start,prefix_length,replacement_start);
    if(origin&&origin->player&&origin->level)count=collect_targets(origin->player,origin->level,"@a",targets,SELECTOR_TARGET_CAPACITY);
    for(index=0;index<count;index++){unsigned at=0,length;entity_name(targets[index],name,sizeof(name));if(!find_fold(name,prefix,&match_start))continue;length=text_len(name);while(at<length&&!is_space(name[at]))at++;if(at<length&&length+3<=sizeof(replacement)){replacement[0]='"';cp(replacement+1,name,length);replacement[length+1]='"';replacement[length+2]=0;add_option(result,name,"",replacement,match_start,prefix_length,replacement_start);}else add_option(result,name,"",name,match_start,prefix_length,replacement_start);}
}

static void add_parameter_options(const CommandParserIntellisenseOrigin *origin,const NativeSchemaParameter *parameter,const char *prefix,u32 prefix_length,u32 replacement_start,CommandParserIntellisenseAutoCompleteInformation *result){
    CommandParserIntellisenseAutoCompleteOption *before=result->end;
    if(!parameter)return;
    if(parameter->type==NATIVE_PARAM_TARGET)add_target_options(origin,parameter,prefix,prefix_length,replacement_start,result);
    else if(parameter->type==NATIVE_PARAM_BOOL){u32 match_start;if(find_fold("true",prefix,&match_start))add_option(result,"true","","true",match_start,prefix_length,replacement_start);if(find_fold("false",prefix,&match_start))add_option(result,"false","","false",match_start,prefix_length,replacement_start);}
    else if(parameter->type==NATIVE_PARAM_STRING_ENUM){EnumOptionContext context={result,prefix,prefix_length,replacement_start};command_enum_visit(parameter,add_enum_value,&context);}
    if(!result->parameter&&result->end!=before)result->parameter=parameter;
}

static void canonical_command_prefix(const char *input,u32 cursor,char *buffer,u32 capacity){
    u32 at=command_start(input),length=0;int pending_space=0;while(at<cursor&&input[at]){if(is_space(input[at]))pending_space=length!=0;else{if(pending_space&&length+1<capacity)buffer[length++]=' ';pending_space=0;if(length+1<capacity)buffer[length++]=input[at];}at++;}if(pending_space&&length+1<capacity)buffer[length++]=' ';buffer[length]=0;
}

static void add_command_options(const CommandParserIntellisenseOrigin *origin,const char *input,u32 cursor,CommandParserIntellisenseAutoCompleteInformation *result){
    char prefix[MAX_TEXT+1],candidate[MAX_TEXT+2];u32 index,match_length;canonical_command_prefix(input,cursor,prefix,sizeof(prefix));match_length=1+text_len(prefix);
    for(index=0;index<native_schema_map_entry_count;index++){
        const NativeSchemaMapEntry *entry=&native_schema_map_entries[index];const NativeSchemaRoot *root=&native_schema_roots[entry->root_index];const NativeSchemaVersion *version=visible_version(root,origin?origin->permission:0);const char *name=native_schema_text(entry->name);u32 length;
        if(!version||!prefix_fold(name,prefix))continue;
        candidate[0]='/';length=text_len(name);if(length>MAX_TEXT)length=MAX_TEXT;cp(candidate+1,name,length);candidate[length+1]=0;if(!add_option(result,candidate,"",candidate,0,match_length,0))return;
    }
}

static void add_command_information(const CommandParserIntellisenseOrigin *origin,const char *input,u32 cursor,CommandParserIntellisenseInformationResult *result){
    char prefix[MAX_TEXT+1],candidate[MAX_TEXT+2];u32 index,match_length;canonical_command_prefix(input,cursor,prefix,sizeof(prefix));match_length=1+text_len(prefix);
    for(index=0;index<native_schema_map_entry_count;index++){
        const NativeSchemaMapEntry *entry=&native_schema_map_entries[index];const NativeSchemaRoot *root=&native_schema_roots[entry->root_index];const NativeSchemaVersion *version=visible_version(root,origin?origin->permission:0);const char *name=native_schema_text(entry->name);u32 length;if(!version||!prefix_fold(name,prefix))continue;candidate[0]='/';length=text_len(name);if(length>MAX_TEXT)length=MAX_TEXT;cp(candidate+1,name,length);candidate[length+1]=0;if(!add_information_text(result,candidate,0,match_length))return;
    }
}

static u32 clamp_cursor(const char *input,u32 cursor){u32 length=text_len(input);return cursor<length?cursor:length;}

static int line_through_cursor(const char *input,u32 cursor,char *line,u32 capacity){
    if(cursor>=capacity)return 0;
    cp(line,input,cursor);line[cursor]=0;return 1;
}

static int complete_prefix(const char *input,u32 argument_start_at,u32 token_start,char *buffer,u32 capacity){
    u32 length=token_start>argument_start_at?token_start-argument_start_at:0;while(length&&is_space(input[argument_start_at+length-1]))length--;if(length>=capacity)return 0;cp(buffer,input+argument_start_at,length);buffer[length]=0;return 1;
}

static void normalize_fragment(char *fragment,u32 *length){
    u32 start=0,end=*length;
    while(start<end&&is_space(fragment[start]))start++;
    while(end>start&&is_space(fragment[end-1]))end--;
    if(start<end&&fragment[start]=='"')start++;
    if(end>start&&fragment[end-1]=='"')end--;
    if(start&&end>start)cp(fragment,fragment+start,end-start);
    *length=end-start;fragment[*length]=0;
}

void command_parser_get_intellisense_information(const CommandParserIntellisenseOrigin *origin,const char *input,u32 cursor,CommandParserIntellisenseInformationResult *result){
    const NativeSchemaMapEntry *entry;const NativeSchemaRoot *root;const NativeSchemaVersion *version;const NativeSchemaOverload *overloads;u32 args,token_start,token_end,index;char complete[MAX_TEXT+1],line[MAX_TEXT+1];int partial;
    zero(result,sizeof(*result));if(!input||input[0]!='/')return;cursor=clamp_cursor(input,cursor);if(!line_through_cursor(input,cursor,line,sizeof(line)))return;entry=native_command_parser_matched_entry(line);if(!entry){add_command_information(origin,line,cursor,result);sort_information(result);result->valid=result->begin!=result->end;return;}root=&native_schema_roots[entry->root_index];version=visible_version(root,origin?origin->permission:0);if(!version)return;args=argument_start(line,native_schema_text(entry->name));if(cursor<args)cursor=args;token_range(line,args,cursor,&token_start,&token_end);partial=token_start<cursor&&!is_space(line[cursor-1]);if(!complete_prefix(line,args,partial?token_start:cursor,complete,sizeof(complete)))return;overloads=native_schema_version_overloads(version);
    for(index=0;index<version->overload_count;index++){u32 mask=active_parameter_mask(&overloads[index],complete);const NativeSchemaParameter *active=0;if(mask){u32 parameter_index=0;while(parameter_index<overloads[index].parameter_count&&parameter_index<32&&!(mask&(1u<<parameter_index)))parameter_index++;if(parameter_index<overloads[index].parameter_count&&parameter_index<32)active=&native_schema_overload_parameters(&overloads[index])[parameter_index];}if(mask||!complete[0]){if(!add_information(result,&overloads[index],active))break;}}
    sort_information(result);result->valid=result->begin!=result->end;
}

void command_parser_get_auto_complete_options(const CommandParserIntellisenseOrigin *origin,const char *input,u32 cursor,CommandParserIntellisenseAutoCompleteInformation *result){
    const NativeSchemaMapEntry *entry;const NativeSchemaRoot *root;const NativeSchemaVersion *version;const NativeSchemaOverload *overloads;u32 args,token_start,token_end,prefix_start,prefix_length,index;char complete[MAX_TEXT+1],prefix[MAX_TEXT+1],line[MAX_TEXT+1];int partial;
    zero(result,sizeof(*result));if(!input||input[0]!='/')return;cursor=clamp_cursor(input,cursor);if(!line_through_cursor(input,cursor,line,sizeof(line)))return;entry=native_command_parser_matched_entry(line);if(!entry){add_command_options(origin,line,cursor,result);sort_options(result);result->valid=result->begin!=result->end;return;}
    root=&native_schema_roots[entry->root_index];version=visible_version(root,origin?origin->permission:0);if(!version)return;args=argument_start(line,native_schema_text(entry->name));if(cursor<=args){result->valid=1;return;}token_range(line,args,cursor,&token_start,&token_end);partial=token_start<cursor&&!is_space(line[cursor-1]);prefix_start=partial?token_start:cursor;prefix_length=partial?cursor-token_start:0;if(prefix_length>MAX_TEXT)prefix_length=MAX_TEXT;cp(prefix,line+prefix_start,prefix_length);prefix[prefix_length]=0;normalize_fragment(prefix,&prefix_length);if(!complete_prefix(line,args,prefix_start,complete,sizeof(complete)))return;overloads=native_schema_version_overloads(version);
    for(index=0;index<version->overload_count;index++){const NativeSchemaParameter *parameters=native_schema_overload_parameters(&overloads[index]);u32 parameter_index,mask=active_parameter_mask(&overloads[index],complete);for(parameter_index=0;parameter_index<overloads[index].parameter_count&&parameter_index<32;parameter_index++)if(mask&(1u<<parameter_index))add_parameter_options(origin,&parameters[parameter_index],prefix,prefix_length,token_start,result);}
    sort_options(result);result->valid=1;
}

void command_parser_intellisense_information_destroy(CommandParserIntellisenseInformationResult *result){
    CommandParserIntellisenseInformation *at;if(!result)return;at=result->begin;while(at&&at!=result->end){if(at->text)s->host.heap_free(at->text);at++;}if(result->begin)s->host.heap_free(result->begin);zero(result,sizeof(*result));
}

void command_parser_auto_complete_information_destroy(CommandParserIntellisenseAutoCompleteInformation *result){
    CommandParserIntellisenseAutoCompleteOption *at;if(!result)return;at=result->begin;while(at&&at!=result->end)free_option(at++);if(result->begin)s->host.heap_free(result->begin);zero(result,sizeof(*result));
}
