#include "../effect_registry.h"
#include "../native_types.h"
#include "../../internal.h"

typedef void (*MobEffectNameMapLowerBoundFn)(void *,void *,void *);
typedef int (*StringCompareFn)(const NativeGstdString *,const NativeGstdString *);

int native_effect_id_from_name(const char *name){
    NativeGstdString value;
    u32 scratch=0,node=0;
    void *header,*effect;
    int id=-1;
    if(!name||!name[0])return -1;
    zero(&value,sizeof(value));
    ((StrCtor)SEAM_StrCtor)(&value,name,&scratch);
    ((MobEffectNameMapLowerBoundFn)SEAM_MobEffectNameMap_lowerBound)(&node,(void *)SEAM_MobEffect_nameMap,&value);
    header=*(void **)((u8 *)SEAM_MobEffect_nameMap+0x10);
    if(node&&(void *)node!=header&&((StringCompareFn)SEAM_gstd_string_compare)(&value,(NativeGstdString *)(node+0x10))==0){
        effect=*(void **)(node+0x14);
        if(effect)id=*(int *)((u8 *)effect+4);
    }
    ((StrDtor)SEAM_StrDtor)(&value);
    return id;
}
