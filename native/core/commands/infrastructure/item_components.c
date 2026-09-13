#include "../item_components.h"
#include "../../internal.h"

typedef void (*TagResourceReaderCtorFn)(void *);
typedef void (*TagResourceReaderDtorFn)(void *);
typedef void (*TagResourceReaderParseFn)(void *,const unsigned char *,const unsigned char *,void *,int);
typedef int (*JsonPredicateFn)(void *);
typedef int (*JsonSizeFn)(void *);
typedef void *(*JsonIndexFn)(void *,unsigned);
typedef void (*BlockPointerVectorAppendFn)(void *,void *);

static void *json_member(void *object,const char *name){
    u32 key=0,scratch=0;void *value;if(!object||!name)return 0;((StrCtor)SEAM_StrCtor)(&key,name,&scratch);value=((void *(*)(void *,void *))SEAM_Json_Value_lookupGstd)(object,&key);((StrDtor)SEAM_StrDtor)(&key);return value;
}

static int apply_component_blocks(void *item,void *component,unsigned offset){
    void *blocks;unsigned index,count;if(!item||!component||!((JsonPredicateFn)SEAM_Json_Value_isObject)(component)||((JsonPredicateFn)SEAM_Json_Value_isNull)(component))return 0;
    blocks=json_member(component,"blocks");if(!blocks||((JsonPredicateFn)SEAM_Json_Value_isNull)(blocks)||!((JsonPredicateFn)SEAM_Json_Value_isArrayOrNull)(blocks))return 0;
    count=(unsigned)((JsonSizeFn)SEAM_Json_Value_size)(blocks);if(!count)return 0;*(u32 *)((u8 *)item+offset+4)=*(u32 *)((u8 *)item+offset);
    for(index=0;index<count;index++){void *entry=((JsonIndexFn)SEAM_Json_Value_indexByUint)(blocks,index);const char *name=0;void *block;if(!entry||!((JsonPredicateFn)SEAM_Json_Value_isString)(entry))return 0;cp(&name,entry,sizeof(name));block=block_by_name(name);if(!block)return 0;((BlockPointerVectorAppendFn)SEAM_BlockPointerVector_append)((u8 *)item+offset,block);}
    return 1;
}

int command_item_components_apply(void *item,const char *components){
    unsigned char reader[0x80],value[20];void *destroy,*place;int present=0,expected;if(!components)return 1;zero(reader,sizeof(reader));zero(value,sizeof(value));
    ((TagResourceReaderCtorFn)SEAM_TagResourceReader_constructor)(reader);((TagResourceReaderParseFn)SEAM_TagResourceReader_parse)(reader,(const unsigned char *)components,(const unsigned char *)components+text_len(components),value,1);
    if(!((JsonPredicateFn)SEAM_Json_Value_isObject)(value)||((JsonPredicateFn)SEAM_Json_Value_isNull)(value))goto fail;expected=((JsonSizeFn)SEAM_Json_Value_size)(value);if(expected<1||expected>2)goto fail;
    destroy=json_member(value,"minecraft:can_destroy");place=json_member(value,"minecraft:can_place_on");if(destroy&&!((JsonPredicateFn)SEAM_Json_Value_isNull)(destroy)){present++;if(!apply_component_blocks(item,destroy,0x18))goto fail;}if(place&&!((JsonPredicateFn)SEAM_Json_Value_isNull)(place)){present++;if(!apply_component_blocks(item,place,0x24))goto fail;}
    ((void (*)(void *))SEAM_Tag_destructByType)(value);((TagResourceReaderDtorFn)SEAM_TagResourceReader_destructor)(reader);return present==expected;
fail:
    ((void (*)(void *))SEAM_Tag_destructByType)(value);((TagResourceReaderDtorFn)SEAM_TagResourceReader_destructor)(reader);return 0;
}
