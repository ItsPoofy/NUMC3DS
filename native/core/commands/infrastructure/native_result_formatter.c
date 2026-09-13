#include "../native_result_formatter.h"
#include "../native_property_bag.h"
#include "../command_conformance.h"
#include "../../internal.h"

typedef struct {
    NativeGstdString format;
    NativeGstdString color;
    NativeVector parameter_names;
    NativeVector conditions;
} NativeCommandFormatString;

typedef struct {
    u32 vtable;
    NativeCommandOverload *overload;
    NativeVector format_strings;
} NativeCommandStringResultFormatter;

typedef void *(*VectorAllocateFn)(u32,u32);
typedef void (*VectorDeallocateFn)(void*,u32,int);
typedef void (*OperatorDeleteFn)(void*);
typedef void (*FunctionWrapperManagerFn)(void*,void*,int);

static void *vector_allocate(u32 bytes){return ((VectorAllocateFn)SEAM_gstd_allocator_allocate)(bytes,0);}
static int string_init(NativeGstdString *value,const char *text){u32 scratch=0;((StrCtor)SEAM_StrCtor)(value,text?text:"",&scratch);return value->handle!=0;}
static unsigned local_text_length(const char *text){unsigned length=0;while(text&&text[length])length++;return length;}
static const char *local_find_text(const char *text,const char *needle){unsigned index,needle_length=local_text_length(needle);if(!text||!needle_length)return 0;for(;*text;text++){for(index=0;index<needle_length&&text[index]==needle[index];index++);if(index==needle_length)return text;}return 0;}
static void format_string_destroy(NativeCommandFormatString *value);

static int condition_manager(void *destination,const void *source,int operation){if(operation==2&&destination&&source)cp(destination,source,8);return 0;}

static u32 condition_invoke(void *storage,void *bag){
    const NativeSchemaFormatCondition *condition=storage?*(const NativeSchemaFormatCondition**)storage:0;const char *name;int value;
    if(!condition||!bag)return 0;name=native_schema_text(condition->name);
    if(condition->kind==NATIVE_SCHEMA_FORMAT_IS_TRUE)return native_bag_get_bool(bag,name,&value)&&value;
    if(condition->kind==NATIVE_SCHEMA_FORMAT_NOT_EMPTY)return native_bag_value_not_empty(bag,name,condition->parameter_type);
    return 0;
}

static int load_name_vector(NativeVector *vector,const NativeSchemaFormatString *schema){
    NativeGstdString *names;unsigned index;if(!schema->parameter_name_count)return 1;
    names=(NativeGstdString*)vector_allocate(schema->parameter_name_count*sizeof(*names));if(!names)return 0;zero(names,schema->parameter_name_count*sizeof(*names));
    vector->begin=(u32)names;vector->end=(u32)names;vector->capacity=(u32)(names+schema->parameter_name_count);
    for(index=0;index<schema->parameter_name_count;index++){vector->end=(u32)(names+index+1);if(!string_init(&names[index],native_schema_text(native_schema_format_parameter_names[schema->parameter_name_start+index])))return 0;}return 1;
}

static int load_condition_vector(NativeVector *vector,const NativeSchemaFormatString *schema){
    NativeCommandCallback *conditions;unsigned index;if(!schema->condition_count)return 1;
    conditions=(NativeCommandCallback*)vector_allocate(schema->condition_count*sizeof(*conditions));if(!conditions)return 0;zero(conditions,schema->condition_count*sizeof(*conditions));
    vector->begin=(u32)conditions;vector->end=(u32)conditions;vector->capacity=(u32)(conditions+schema->condition_count);
    for(index=0;index<schema->condition_count;index++){conditions[index].storage[0]=(u32)&native_schema_format_conditions[schema->condition_start+index];conditions[index].manager=(u32)condition_manager;conditions[index].invoker=(u32)condition_invoke;vector->end=(u32)(conditions+index+1);}return 1;
}

static int load_format_string(NativeCommandFormatString *destination,const NativeSchemaFormatString *schema){
    zero(destination,sizeof(*destination));
    if(!string_init(&destination->format,native_schema_text(schema->format)))return 0;
    if(!string_init(&destination->color,native_schema_text(schema->color)))goto failed;
    if(!load_name_vector(&destination->parameter_names,schema))goto failed;
    if(load_condition_vector(&destination->conditions,schema))return 1;
failed:
    format_string_destroy(destination);
    zero(destination,sizeof(*destination));
    return 0;
}

static void format_string_destroy(NativeCommandFormatString *value){
    NativeGstdString *name=(NativeGstdString*)value->parameter_names.begin,*name_end=(NativeGstdString*)value->parameter_names.end;NativeCommandCallback *condition=(NativeCommandCallback*)value->conditions.begin,*condition_end=(NativeCommandCallback*)value->conditions.end;
    while(name!=name_end){if(name->handle)((StrDtor)SEAM_StrDtor)(name);name++;}if(value->parameter_names.begin)((VectorDeallocateFn)SEAM_gstd_allocator_deallocate)((void*)value->parameter_names.begin,(value->parameter_names.capacity-value->parameter_names.begin)/sizeof(NativeGstdString),0);
    while(condition!=condition_end){if(condition->manager)((FunctionWrapperManagerFn)condition->manager)(condition,condition,3);condition++;}if(value->conditions.begin)((VectorDeallocateFn)SEAM_gstd_allocator_deallocate)((void*)value->conditions.begin,(value->conditions.capacity-value->conditions.begin)/sizeof(NativeCommandCallback),0);
    if(value->color.handle)((StrDtor)SEAM_StrDtor)(&value->color);if(value->format.handle)((StrDtor)SEAM_StrDtor)(&value->format);
}

static void formatter_destroy(NativeCommandStringResultFormatter *formatter){
    NativeCommandFormatString *value=(NativeCommandFormatString*)formatter->format_strings.begin,*end=(NativeCommandFormatString*)formatter->format_strings.end;while(value!=end)format_string_destroy(value++);if(formatter->format_strings.begin)((VectorDeallocateFn)SEAM_gstd_allocator_deallocate)((void*)formatter->format_strings.begin,(formatter->format_strings.capacity-formatter->format_strings.begin)/sizeof(NativeCommandFormatString),0);
}

static void formatter_destructor(NativeCommandStringResultFormatter *formatter){formatter_destroy(formatter);}
static void formatter_deleting_destructor(NativeCommandStringResultFormatter *formatter){formatter_destroy(formatter);((OperatorDeleteFn)SEAM_operator_delete)(formatter);}

static void replace_token(char *text,unsigned capacity,const char *token,const char *replacement){
    char result[MAX_TEXT+1];const char *cursor=text,*match;unsigned length=0,token_length=local_text_length(token);result[0]=0;if(!token_length)return;
    while((match=local_find_text(cursor,token))!=0){const char *replacement_at=replacement;while(cursor<match&&length+1<capacity)result[length++]=*cursor++;cursor+=token_length;while(replacement_at&&*replacement_at&&length+1<capacity)result[length++]=*replacement_at++;}while(*cursor&&length+1<capacity)result[length++]=*cursor++;result[length]=0;copy_text(text,result,capacity-1);
}

static void localize_key(char *out,unsigned capacity,const char *key,NativeGstdString *parameters,unsigned parameter_count){
    NativeGstdString source,result;NativeVector vector;u32 scratch=0;const char *text=0;zero(&source,sizeof(source));zero(&result,sizeof(result));zero(&vector,sizeof(vector));
    NativeGstdString *eff_params = parameters;
    unsigned eff_count = parameter_count;
    if (key && streq(key, "commands.gamemode.success.self") && parameter_count >= 3) {
        eff_params = &parameters[2];
        eff_count = 1;
    } else if (key && streq(key, "commands.gamemode.success.other") && parameter_count >= 3) {
        eff_params = &parameters[1];
        eff_count = 2;
    }
    ((StrCtor)SEAM_StrCtor)(&source,key?key:"",&scratch);vector.begin=(u32)eff_params;vector.end=(u32)(eff_params+eff_count);vector.capacity=vector.end;((LocalizationGetFn)SEAM_Localization_get)(&result,&source,eff_count?&vector:0);if(result.handle)text=(const char*)result.handle;copy_text(out,text&&text[0]?text:(key?key:""),capacity-1);if(result.handle)((StrDtor)SEAM_StrDtor)(&result);if(source.handle)((StrDtor)SEAM_StrDtor)(&source);
}

static int format_should_show(const NativeCommandFormatString *format,void *bag){
    NativeCommandCallback *condition=(NativeCommandCallback*)format->conditions.begin,*end=(NativeCommandCallback*)format->conditions.end;while(condition!=end){if(!condition->invoker||!((u32(*)(void*,void*))condition->invoker)(condition->storage,bag))return 0;condition++;}return 1;
}

static void format_add_output_parameter(NativeGstdString *parameters,unsigned *count,void *bag,const NativeCommandParameter *parameter){
    char value[MAX_TEXT+1];int position[3],relative[3];unsigned axis;
    if(!parameters||!count||!parameter||*count>=32)return;
    if(parameter->type==NATIVE_PARAM_BLOCKPOS&&native_bag_get_blockpos(bag,(const char*)parameter->name.handle,position,relative)){
        for(axis=0;axis<3&&*count<32;axis++){unsigned length=0;value[0]=0;append_int(value,&length,position[axis]);string_init(&parameters[(*count)++],value);}return;
    }
    native_bag_value_text(bag,(const char*)parameter->name.handle,value,sizeof(value));string_init(&parameters[(*count)++],value);
}

static void format_parameters(char *line, unsigned capacity, NativeGstdString *parameters, unsigned count) {
    char token[16];
    unsigned index;
    for (index = 0; index < count; index++) {
        unsigned tlen = 0;
        const char *val = parameters[index].handle ? (const char*)parameters[index].handle : "";
        token[0] = '%'; tlen = 1;
        append_int(token, &tlen, (int)(index + 1));
        token[tlen++] = '$';
        token[tlen] = 's'; token[tlen + 1] = 0;
        replace_token(line, capacity, token, val);
        token[tlen] = 'd';
        replace_token(line, capacity, token, val);
        token[tlen] = 'i';
        replace_token(line, capacity, token, val);
        token[tlen] = 'u';
        replace_token(line, capacity, token, val);
    }
    for (index = 0; index < count; index++) {
        unsigned tlen = 0;
        const char *val = parameters[index].handle ? (const char*)parameters[index].handle : "";
        token[0] = '{'; tlen = 1;
        append_int(token, &tlen, (int)index);
        token[tlen++] = '}'; token[tlen] = 0;
        replace_token(line, capacity, token, val);
    }
}

static void formatter_format(NativeGstdString *out,NativeCommandStringResultFormatter *formatter,void *bag,int localize){
    NativeCommandFormatString *format=(NativeCommandFormatString*)formatter->format_strings.begin,*end=(NativeCommandFormatString*)formatter->format_strings.end;NativeCommandParameter *output=(NativeCommandParameter*)formatter->overload->output.begin,*output_end=(NativeCommandParameter*)formatter->overload->output.end;char result[MAX_TEXT+1],line[MAX_TEXT+1],value[MAX_TEXT+1];unsigned result_length=0,index,count;u32 scratch=0;result[0]=0;
    while(format!=end){if(format_should_show(format,bag)){NativeGstdString parameters[32];NativeGstdString *selected=(NativeGstdString*)format->parameter_names.begin,*selected_end=(NativeGstdString*)format->parameter_names.end;const char *pattern=(const char*)format->format.handle;zero(parameters,sizeof(parameters));count=0;
            if(selected!=selected_end){while(selected!=selected_end&&count<32){native_bag_value_text(bag,(const char*)selected->handle,value,sizeof(value));string_init(&parameters[count++],value);selected++;}}
            else{NativeCommandParameter *parameter=output;while(parameter!=output_end&&count<32){format_add_output_parameter(parameters,&count,bag,parameter);parameter++;}}
            if(pattern&&pattern[0]=='%'){const char *separator=local_find_text(pattern,": ");char key[128];unsigned key_length=separator?(unsigned)(separator-(pattern+1)):local_text_length(pattern+1),line_length;if(key_length>=sizeof(key))key_length=sizeof(key)-1;cp(key,pattern+1,key_length);key[key_length]=0;localize_key(line,sizeof(line),key,parameters,count);if(separator){line_length=local_text_length(line);append(line,&line_length,separator+2);}}
            else if(pattern&&pattern[0]=='{')copy_text(line,pattern,sizeof(line)-1);else if(localize)localize_key(line,sizeof(line),pattern,parameters,count);else copy_text(line,pattern?pattern:"",sizeof(line)-1);
            format_parameters(line,sizeof(line),parameters,count);
            if(line[0]=='%'||local_find_text(line,"commands.")==line){char key[128];const char*source=line+(line[0]=='%');unsigned key_length=local_text_length(source);if(key_length>=sizeof(key))key_length=sizeof(key)-1;cp(key,source,key_length);key[key_length]=0;localize_key(line,sizeof(line),key,parameters,count);format_parameters(line,sizeof(line),parameters,count);}
            if(result_length)append(result,&result_length,"\n");if(format->color.handle&&((const char*)format->color.handle)[0]&&streq((const char*)format->color.handle,"red"))append(result,&result_length,"\xC2\xA7" "c");append(result,&result_length,line);for(index=0;index<count;index++)if(parameters[index].handle)((StrDtor)SEAM_StrDtor)(&parameters[index]);}
        format++;}
    command_conformance_capture_result(result);
    ((StrCtor)SEAM_StrCtor)(out,result,&scratch);
}
static const u32 formatter_vtable[]={ (u32)formatter_destructor,(u32)formatter_deleting_destructor,(u32)formatter_format };

int native_command_overload_set_format_strings(NativeCommandOverload *overload,const NativeSchemaOverload *schema){
    NativeCommandStringResultFormatter *formatter;NativeCommandFormatString *formats;const NativeSchemaFormatString *source;unsigned index;if(!overload||!schema)return 0;
    formatter=(NativeCommandStringResultFormatter*)((void*(*)(u32))SEAM_operator_new)(sizeof(*formatter));if(!formatter)return 0;zero(formatter,sizeof(*formatter));formatter->vtable=(u32)formatter_vtable;formatter->overload=overload;overload->formatter=(u32)formatter;
    if(!schema->format_count)return 1;
    formats=(NativeCommandFormatString*)vector_allocate(schema->format_count*sizeof(*formats));if(!formats)goto failed;zero(formats,schema->format_count*sizeof(*formats));formatter->format_strings.begin=(u32)formats;formatter->format_strings.end=(u32)formats;formatter->format_strings.capacity=(u32)(formats+schema->format_count);source=native_schema_overload_formats(schema);
    for(index=0;index<schema->format_count;index++){formatter->format_strings.end=(u32)(formats+index+1);if(!load_format_string(&formats[index],&source[index]))goto failed;}
    return 1;
failed:
    overload->formatter=0;
    formatter_deleting_destructor(formatter);
    return 0;
}

_Static_assert(sizeof(NativeCommandFormatString)==0x20,"NativeCommandFormatString ABI");
_Static_assert(sizeof(NativeCommandStringResultFormatter)==0x14,"NativeCommandStringResultFormatter ABI");

void native_command_format_schema_chat(const NativeSchemaOverload *schema, void *bag) {
    unsigned f_idx;
    if (!schema || !bag || !schema->format_count) return;
    for (f_idx = 0; f_idx < schema->format_count; f_idx++) {
        const NativeSchemaFormatString *fmt = &native_schema_format_strings[schema->format_start + f_idx];
        unsigned c_idx, condition_met = 1;
        NativeGstdString parameters[32];
        unsigned count = 0, index;
        char line[MAX_TEXT+1], value[MAX_TEXT+1], token[16];
        const char *pattern;
        for (c_idx = 0; c_idx < fmt->condition_count; c_idx++) {
            const NativeSchemaFormatCondition *cond = &native_schema_format_conditions[fmt->condition_start + c_idx];
            const char *cname = native_schema_text(cond->name);
            int bval = 0;
            if (cond->kind == NATIVE_SCHEMA_FORMAT_IS_TRUE) {
                if (!native_bag_get_bool(bag, cname, &bval) || !bval) { condition_met = 0; break; }
            } else if (cond->kind == NATIVE_SCHEMA_FORMAT_NOT_EMPTY) {
                if (!native_bag_value_not_empty(bag, cname, cond->parameter_type)) { condition_met = 0; break; }
            }
        }
        if (!condition_met) continue;

        zero(parameters, sizeof(parameters));
        if (fmt->parameter_name_count > 0) {
            unsigned p_idx;
            for (p_idx = 0; p_idx < fmt->parameter_name_count && count < 32; p_idx++) {
                const char *pname = native_schema_text(native_schema_format_parameter_names[fmt->parameter_name_start + p_idx]);
                native_bag_value_text(bag, pname, value, sizeof(value));
                string_init(&parameters[count++], value);
            }
        } else {
            unsigned o_idx;
            for (o_idx = 0; o_idx < schema->output_count && count < 32; o_idx++) {
                const NativeSchemaParameter *oparam = &native_schema_parameters[schema->output_start + o_idx];
                const char *oname = native_schema_text(oparam->name);
                if (oparam->type == NATIVE_PARAM_BLOCKPOS) {
                    int pos[3], rel[3];
                    if (native_bag_get_blockpos(bag, oname, pos, rel)) {
                        unsigned axis;
                        for (axis = 0; axis < 3 && count < 32; axis++) {
                            unsigned vlen = 0; value[0] = 0;
                            append_int(value, &vlen, pos[axis]);
                            string_init(&parameters[count++], value);
                        }
                    }
                } else {
                    native_bag_value_text(bag, oname, value, sizeof(value));
                    string_init(&parameters[count++], value);
                }
            }
        }

        pattern = native_schema_text(fmt->format);
        if (pattern && pattern[0] == '%') {
            const char *sep = local_find_text(pattern, ": ");
            char key[128];
            unsigned key_len = sep ? (unsigned)(sep - (pattern + 1)) : local_text_length(pattern + 1);
            if (key_len >= sizeof(key)) key_len = sizeof(key) - 1;
            cp(key, pattern + 1, key_len); key[key_len] = 0;
            localize_key(line, sizeof(line), key, parameters, count);
            if (sep) {
                unsigned llen = local_text_length(line);
                append(line, &llen, sep + 2);
            }
        } else if (pattern && pattern[0] == '{') {
            copy_text(line, pattern, sizeof(line) - 1);
        } else {
            localize_key(line, sizeof(line), pattern ? pattern : "", parameters, count);
        }

        format_parameters(line, sizeof(line), parameters, count);

        if (line[0] == '%' || local_find_text(line, "commands.") == line) {
            char key[128];
            const char *src = line + (line[0] == '%');
            unsigned klen = local_text_length(src);
            if (klen >= sizeof(key)) klen = sizeof(key) - 1;
            cp(key, src, klen); key[klen] = 0;
            localize_key(line, sizeof(line), key, parameters, count);
            format_parameters(line, sizeof(line), parameters, count);
        }

        for (index = 0; index < count; index++) {
            if (parameters[index].handle) ((StrDtor)SEAM_StrDtor)(&parameters[index]);
        }

        if (line[0]) {
            command_conformance_capture_result(line);
        }
    }
}
