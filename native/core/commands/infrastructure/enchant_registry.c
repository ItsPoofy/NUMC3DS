#include "../enchant_registry.h"
#include "../native_types.h"
#include "../../internal.h"

typedef int (*StringCompareFn)(const NativeGstdString *,const NativeGstdString *);

int native_enchant_id_from_name(const char *name){
    NativeGstdString value;
    void **table,*enchant;
    u32 scratch=0;
    int id;
    if(!name||!name[0])return -1;
    zero(&value,sizeof(value));
    ((StrCtor)SEAM_StrCtor)(&value,name,&scratch);
    table=*(void ***)SEAM_Enchant_registry;
    for(id=0;table&&id<27;id++){
        enchant=table[id];
        if(enchant&&((StringCompareFn)SEAM_gstd_string_compare)(&value,(NativeGstdString *)((u8 *)enchant+0x1c))==0)break;
    }
    ((StrDtor)SEAM_StrDtor)(&value);
    return id<27?id:-1;
}
