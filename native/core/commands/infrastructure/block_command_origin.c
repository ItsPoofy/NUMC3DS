#include "block_command_origin.h"
#include "../../internal.h"

static NuMC3DS_Hook constructor_hook;
static u32 origin_vtable[24];

static void localized_name(u32 *out,const char *key){
    u32 source=0,scratch=0;
    ((StrCtor)SEAM_StrCtor)(&source,key,&scratch);
    ((LocalizationGetFn)SEAM_Localization_get)(out,&source,0);
    ((StrDtor)SEAM_StrDtor)(&source);
}

static void get_name(u32 *out,void *origin){
    void *base=((void*(*)(void*))(*(u32**)origin)[21])(origin);
    const u32 *name;
    if(!base){localized_name(out,"commandBlock.genericName");return;}
    name=((const u32*(*)(void*))0x006DB564u)(base);
    if(name&&*name&&*(const char*)*name){((void(*)(void*,const void*))0x002FF261u)(out,name);return;}
    localized_name(out,"commandBlock.shortName");
}

static void *construct(void *self,void *source,const int *position){
    void *result=((void*(*)(void*,void*,const int*))constructor_hook.trampoline)(self,source,position);
    if(result)*(u32**)result=origin_vtable+2;
    return result;
}

int block_command_origin_install(void){
    const u32 *stock=(const u32*)0x009B9380u;
    if(stock[3]!=0||stock[21]!=0x006F49C0u)return -90;
    cp(origin_vtable,stock-2,sizeof(origin_vtable));origin_vtable[5]=(u32)get_name;
    constructor_hook.target=0x0038AC04u;constructor_hook.replacement=(u32)construct;
    constructor_hook.expected[0]=0xE92D4070u;constructor_hook.expected[1]=0xE1A04002u;
    return s->host.install_hook(&constructor_hook);
}
