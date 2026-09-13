#include "../native_property_bag.h"
#include "../native_registry.h"
#include "../../internal.h"

typedef void *(*JsonLookupFn)(void*,void*);
typedef int (*JsonIntFn)(void*,int);
typedef int (*JsonBoolFn)(void*,int);
typedef int (*JsonPredicateFn)(void*);
typedef double (__attribute__((pcs("aapcs-vfp"))) *JsonDoubleFn)(void*);
typedef void *(__attribute__((pcs("aapcs-vfp"))) *JsonDoubleCtorFn)(void*,double);
typedef void *(*OriginPointerFn)(void *);
typedef void *(*LevelFetchEntityFn)(void*,u32,u32,u32,u32);
typedef void (*CommandTargetFromPropertyBagFn)(void*,void*,void*,void*);
typedef int (*CommandTargetMatchCountFn)(void*);
typedef void (*CommandTargetIteratorFn)(void*,void*);
typedef void *(*CommandTargetDereferenceFn)(void*);
typedef void *(*CommandTargetIncrementFn)(void*);
typedef unsigned (*CommandTargetIteratorNotEqualFn)(void*,void*);
typedef void (*FunctionWrapperManagerFn)(void*,void*,int);
typedef void (*AllocatorDeallocateFn)(void*,unsigned,int);
typedef unsigned long long (*JsonInt64Fn)(void*);
typedef const u32 *(*EntityUniqueIdFn)(void *);
typedef void *(*JsonMemberFn)(void*,void*);
typedef void *(*JsonAssignFn)(void*,void*);
typedef void *(*JsonValueCtorFn)(void*,int);
typedef void *(*JsonStringCtorFn)(void*,void*);
typedef void *(*JsonIntCtorFn)(void*,int);
typedef void *(*JsonBoolCtorFn)(void*,int);
typedef void *(*JsonAppendFn)(void*,void*);
typedef void (*JsonDtorFn)(void*);
typedef int (*GstdSnprintfFn)(char*,u32,const char*,...);

static void command_target_destroy(NativeCommandTarget *target){
    u8*cursor=(u8*)target->resolvers.begin;
    u8*end=(u8*)target->resolvers.end;
    if(target->matches.begin)((AllocatorDeallocateFn)SEAM_gstd_allocator_deallocate)((void*)target->matches.begin,(target->matches.capacity-target->matches.begin)/sizeof(void*),0);
    while(cursor!=end){
        FunctionWrapperManagerFn manager=*(FunctionWrapperManagerFn*)(cursor+8);
        if(manager)manager(cursor,cursor,3);
        cursor+=16;
    }
    if(target->resolvers.begin)((AllocatorDeallocateFn)SEAM_gstd_allocator_deallocate)((void*)target->resolvers.begin,(target->resolvers.capacity-target->resolvers.begin)/16,0);
}

static void *bag_member(void*object,const char*name){
    u32 key=0,scratch=0;void*value;
    if(!object||!name)return 0;
    ((StrCtor)SEAM_StrCtor)(&key,name,&scratch);
    value=((JsonLookupFn)SEAM_Json_Value_lookupGstd)(object,&key);
    ((StrDtor)SEAM_StrDtor)(&key);
    return value;
}

static void *parameter_value(void*bag,const char*name){
    void*value;if(!bag)return 0;
    value=bag_member((u8*)bag+SEAM_CommandPropertyBag_jsonOffset,name);
    if(!value||((JsonPredicateFn)SEAM_Json_Value_isNull)(value))return 0;
    return value;
}

static void set_parameter_value(void*bag,const char*name,void*value){
    NativeGstdString key;void*slot;u32 scratch=0;if(!bag||!name||!value)return;zero(&key,sizeof(key));((StrCtor)SEAM_StrCtor)(&key,name,&scratch);slot=((JsonMemberFn)SEAM_Json_Value_getOrCreate)((u8*)bag+SEAM_CommandPropertyBag_jsonOffset,&key);if(slot)((JsonAssignFn)SEAM_Json_Value_assign)(slot,value);((StrDtor)SEAM_StrDtor)(&key);
}

static void *origin_executing_entity(void *origin);

int native_bag_has(void*bag,const char*name){return parameter_value(bag,name)!=0;}

static const char *value_string(void*value){
    const char*text=0;
    if(!value||!((JsonPredicateFn)SEAM_Json_Value_isString)(value))return 0;
    cp(&text,value,sizeof(text));return text;
}

static void *origin_executing_entity(void *origin){
    void **vtable=origin?*(void ***)origin:0;
    void *entity=vtable&&vtable[7]?((OriginPointerFn)vtable[7])(origin):0;
    return entity?entity:(s?s->command_player:0);
}

static void *origin_level(void *origin){
    void **vtable=origin?*(void ***)origin:0;
    return vtable&&vtable[6]?((OriginPointerFn)vtable[6])(origin):0;
}

static int resolved_target_entity(void *origin,void *value,void **entity){
    unsigned long long unique_id,origin_id;const u32 *origin_words;void *level,*executing_entity;
    if(entity)*entity=0;
    if(!value||
       (!((JsonPredicateFn)SEAM_Json_Value_isInt)(value)&&!((JsonPredicateFn)SEAM_Json_Value_isUInt)(value)))return 0;
    if(!origin)return 1;
    unique_id=((JsonInt64Fn)SEAM_Json_Value_asInt64)(value);
    executing_entity=origin_executing_entity(origin);
    if(executing_entity){
        origin_words=((EntityUniqueIdFn)SEAM_Entity_getUniqueID)(executing_entity);
        if(origin_words){origin_id=(unsigned long long)origin_words[0]|((unsigned long long)origin_words[1]<<32);if(origin_id==unique_id){if(entity)*entity=executing_entity;return 1;}}
    }
    level=origin_level(origin);
    if(!level)return 1;
    if(entity)*entity=((LevelFetchEntityFn)SEAM_Level_fetchEntity)(level,0,(u32)unique_id,(u32)(unique_id>>32),0);
    return 1;
}

const char *native_bag_get_string(void*bag,const char*name){
    void*value;
    if(!bag||!name)return 0;
    value=parameter_value(bag,name);
    return value_string(value);
}

int native_bag_get_int(void*bag,const char*name,int*out){
    void*value;
    if(!out)return 0;
    value=parameter_value(bag,name);
    if(!value||(!((JsonPredicateFn)SEAM_Json_Value_isInt)(value)&&!((JsonPredicateFn)SEAM_Json_Value_isUInt)(value)))return 0;
    *out=((JsonIntFn)SEAM_Json_Value_asInt)(value,0);
    return 1;
}

int native_bag_get_float(void*bag,const char*name,float*out){
    void*value;
    if(!out)return 0;value=parameter_value(bag,name);if(!value)return 0;
    *out=(float)((JsonDoubleFn)SEAM_Json_Value_asDouble)(value);
    return 1;
}

int native_bag_get_bool(void*bag,const char*name,int*out){
    void*value;
    if(!out)return 0;
    value=parameter_value(bag,name);
    if(!value)return 0;
    *out=((JsonBoolFn)SEAM_Json_Value_asBool)(value,0);
    return 1;
}

int native_bag_get_rotation(void*bag,const char*name,float*out,int*relative){
    void*object,*value,*is_relative;double number;
    if(!out||!relative)return 0;object=parameter_value(bag,name);if(!object)return 0;
    value=bag_member(object,"rotation");if(!value)return 0;
    number=((JsonDoubleFn)SEAM_Json_Value_asDouble)(value);*out=(float)number;
    is_relative=bag_member(object,"relative");*relative=is_relative?((JsonBoolFn)SEAM_Json_Value_asBool)(is_relative,0):0;
    return 1;
}

int native_bag_get_blockpos(void*bag,const char*name,int value[3],int relative[3]){
    static const char*const axes[3]={"x","y","z"};
    static const char*const relative_axes[3]={"xrelative","yrelative","zrelative"};
    void*object;unsigned index;
    if(!value||!relative)return 0;
    object=parameter_value(bag,name);
    if(!object)return 0;
    for(index=0;index<3;index++){
        void*axis=bag_member(object,axes[index]);
        void*is_relative=bag_member(object,relative_axes[index]);
        if(!axis||(!((JsonPredicateFn)SEAM_Json_Value_isInt)(axis)&&!((JsonPredicateFn)SEAM_Json_Value_isUInt)(axis)))return 0;
        value[index]=((JsonIntFn)SEAM_Json_Value_asInt)(axis,0);
        relative[index]=is_relative?((JsonBoolFn)SEAM_Json_Value_asBool)(is_relative,0):0;
    }
    return 1;
}

int native_bag_value_not_empty(void*bag,const char*name,u8 parameter_type){
    void*value=parameter_value(bag,name);const char*text;(void)parameter_type;if(!value)return 0;
    if(((JsonPredicateFn)SEAM_Json_Value_isArrayOrNull)(value))return ((u32(*)(void*))SEAM_Json_Value_size)(value)!=0;
    text=value_string(value);if(text)return text[0]!=0;
    if(((JsonPredicateFn)SEAM_Json_Value_isInt)(value)||((JsonPredicateFn)SEAM_Json_Value_isUInt)(value))return 1;
    return ((JsonBoolFn)SEAM_Json_Value_asBool)(value,0)!=0;
}

int native_bag_value_text(void*bag,const char*name,char*out,unsigned capacity){
    void*value;const char*text;unsigned count,index,length=0;if(!out||!capacity)return 0;out[0]=0;value=parameter_value(bag,name);if(!value)return 0;
    text=value_string(value);if(text){copy_text(out,text,capacity-1);return 1;}
    if(((JsonPredicateFn)SEAM_Json_Value_isInt)(value)||((JsonPredicateFn)SEAM_Json_Value_isUInt)(value)){append_int(out,&length,((JsonIntFn)SEAM_Json_Value_asInt)(value,0));return 1;}
    if(((JsonPredicateFn)SEAM_Json_Value_isDouble)(value)){((GstdSnprintfFn)SEAM_gstd_snprintfBuffered)(out,capacity,"%.5f",((JsonDoubleFn)SEAM_Json_Value_asDouble)(value));return 1;}
    if(((JsonPredicateFn)SEAM_Json_Value_isArrayOrNull)(value)){
        count=((u32(*)(void*))SEAM_Json_Value_size)(value);for(index=0;index<count;index++){void*entry=((void*(*)(void*,u32))SEAM_Json_Value_indexByUint)(value,index);const char*entry_text=value_string(entry);if(index)append(out,&length,", ");if(entry_text)append(out,&length,entry_text);else if(entry&&(((JsonPredicateFn)SEAM_Json_Value_isInt)(entry)||((JsonPredicateFn)SEAM_Json_Value_isUInt)(entry)))append_int(out,&length,((JsonIntFn)SEAM_Json_Value_asInt)(entry,0));}return 1;
    }
    copy_text(out,((JsonBoolFn)SEAM_Json_Value_asBool)(value,0)?"true":"false",capacity-1);return 1;
}

void native_bag_set_string(void*bag,const char*name,const char*text){NativeJsonValue value;NativeGstdString string;u32 scratch=0;zero(&value,sizeof(value));zero(&string,sizeof(string));((StrCtor)SEAM_StrCtor)(&string,text?text:"",&scratch);((JsonStringCtorFn)SEAM_Json_Value_stdStringCtor)(&value,&string);set_parameter_value(bag,name,&value);((JsonDtorFn)SEAM_Tag_destructByType)(&value);((StrDtor)SEAM_StrDtor)(&string);}
void native_bag_set_int(void*bag,const char*name,int number){NativeJsonValue value;zero(&value,sizeof(value));((JsonIntCtorFn)SEAM_Json_Value_intCtor)(&value,number);set_parameter_value(bag,name,&value);((JsonDtorFn)SEAM_Tag_destructByType)(&value);}
void native_bag_set_float(void*bag,const char*name,float number){NativeJsonValue value;double converted=(double)number;zero(&value,sizeof(value));((JsonDoubleCtorFn)SEAM_Json_Value_doubleCtor)(&value,converted);set_parameter_value(bag,name,&value);((JsonDtorFn)SEAM_Tag_destructByType)(&value);}
void native_bag_set_bool(void*bag,const char*name,int boolean){NativeJsonValue value;zero(&value,sizeof(value));((JsonBoolCtorFn)SEAM_Json_Value_boolCtor)(&value,boolean!=0);set_parameter_value(bag,name,&value);((JsonDtorFn)SEAM_Tag_destructByType)(&value);}
void native_bag_set_blockpos(void*bag,const char*name,const int position[3]){NativeJsonValue object,value;static const char*const axes[3]={"x","y","z"};unsigned index;zero(&object,sizeof(object));((JsonValueCtorFn)SEAM_Tag_constructByType)(&object,7);for(index=0;index<3;index++){NativeGstdString key;void*slot;u32 scratch=0;zero(&key,sizeof(key));zero(&value,sizeof(value));((StrCtor)SEAM_StrCtor)(&key,axes[index],&scratch);slot=((JsonMemberFn)SEAM_Json_Value_getOrCreate)(&object,&key);((JsonIntCtorFn)SEAM_Json_Value_intCtor)(&value,position[index]);if(slot)((JsonAssignFn)SEAM_Json_Value_assign)(slot,&value);((JsonDtorFn)SEAM_Tag_destructByType)(&value);((StrDtor)SEAM_StrDtor)(&key);}set_parameter_value(bag,name,&object);((JsonDtorFn)SEAM_Tag_destructByType)(&object);}
void native_bag_set_result_list(void*bag,const char*name,const char*const*values,unsigned count){NativeJsonValue array,value;unsigned index;zero(&array,sizeof(array));((JsonValueCtorFn)SEAM_Tag_constructByType)(&array,6);for(index=0;index<count;index++){NativeGstdString string;u32 scratch=0;zero(&string,sizeof(string));zero(&value,sizeof(value));((StrCtor)SEAM_StrCtor)(&string,values[index]?values[index]:"",&scratch);((JsonStringCtorFn)SEAM_Json_Value_stdStringCtor)(&value,&string);((JsonAppendFn)SEAM_Json_Value_append)(&array,&value);((JsonDtorFn)SEAM_Tag_destructByType)(&value);((StrDtor)SEAM_StrDtor)(&string);}set_parameter_value(bag,name,&array);((JsonDtorFn)SEAM_Tag_destructByType)(&array);}

unsigned native_bag_collect_targets(void*origin,void*bag,const char*name,void**out,unsigned max){
    NativeCommandTarget target;u32 begin[2],end[2];NativeGstdString key;void *value,*entity;u32 scratch=0;unsigned count=0;
    if(!origin||!bag||!name||!out||!max)return 0;
    value=parameter_value(bag,name);
    if(resolved_target_entity(origin,value,&entity)){
        if(entity){out[0]=entity;return 1;}
        return 0;
    }
    zero(&target,sizeof(target));zero(begin,sizeof(begin));zero(end,sizeof(end));zero(&key,sizeof(key));
    ((StrCtor)SEAM_StrCtor)(&key,name,&scratch);
    ((CommandTargetFromPropertyBagFn)SEAM_CommandTarget_fromPropertyBag)(&target,origin,bag,&key);
    ((StrDtor)SEAM_StrDtor)(&key);
    if(((CommandTargetMatchCountFn)SEAM_CommandTarget_getMatchCount)(&target)>0){
        ((CommandTargetIteratorFn)SEAM_CommandTarget_begin)(begin,&target);
        ((CommandTargetIteratorFn)SEAM_CommandTarget_end)(end,&target);
        while(((CommandTargetIteratorNotEqualFn)SEAM_CommandTarget_Iterator_notEqual)(begin,end)&&count<max){
            void*entity=((CommandTargetDereferenceFn)SEAM_CommandTarget_Iterator_dereference)(begin);
            if(entity)out[count++]=entity;
            ((CommandTargetIncrementFn)SEAM_CommandTarget_Iterator_increment)(begin);
        }
    }
    command_target_destroy(&target);
    return count;
}

unsigned native_bag_visit_targets(void*origin,void*bag,const char*name,NativeCommandTargetVisitor visitor,void*user){
    NativeCommandTarget target;u32 begin[2],end[2];NativeGstdString key;unsigned count=0;u32 scratch=0;
    if(!origin||!bag||!name||!visitor)return 0;
    zero(&target,sizeof(target));zero(begin,sizeof(begin));zero(end,sizeof(end));zero(&key,sizeof(key));
    ((StrCtor)SEAM_StrCtor)(&key,name,&scratch);
    ((CommandTargetFromPropertyBagFn)SEAM_CommandTarget_fromPropertyBag)(&target,origin,bag,&key);
    ((StrDtor)SEAM_StrDtor)(&key);
    if(((CommandTargetMatchCountFn)SEAM_CommandTarget_getMatchCount)(&target)>0){
        ((CommandTargetIteratorFn)SEAM_CommandTarget_begin)(begin,&target);
        ((CommandTargetIteratorFn)SEAM_CommandTarget_end)(end,&target);
        while(((CommandTargetIteratorNotEqualFn)SEAM_CommandTarget_Iterator_notEqual)(begin,end)){
            void*entity=((CommandTargetDereferenceFn)SEAM_CommandTarget_Iterator_dereference)(begin);
            if(entity){count++;if(!visitor(user,entity))break;}
            ((CommandTargetIncrementFn)SEAM_CommandTarget_Iterator_increment)(begin);
        }
    }
    command_target_destroy(&target);
    return count;
}
