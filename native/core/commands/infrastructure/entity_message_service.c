#include "../entity_message_service.h"
#include "../native_types.h"
#include "../../internal.h"

int command_entity_display_message(void *entity,const char *sender,const char *message){
    NativeGstdString source,parameter;NativeVector parameters;void **vtable;u32 scratch=0;
    if(!entity||!sender||!message)return 0;
    vtable=*(void ***)entity;
    if(!vtable||!vtable[0x52c/4])return 0;
    zero(&source,sizeof(source));zero(&parameter,sizeof(parameter));zero(&parameters,sizeof(parameters));
    ((StrCtor)SEAM_StrCtor)(&source,sender,&scratch);
    ((StrCtor)SEAM_StrCtor)(&parameter,message,&scratch);
    if(!source.handle||!parameter.handle){if(parameter.handle)((StrDtor)SEAM_StrDtor)(&parameter);if(source.handle)((StrDtor)SEAM_StrDtor)(&source);return 0;}
    parameters.begin=(u32)&parameter;parameters.end=(u32)(&parameter+1);parameters.capacity=parameters.end;
    ((void(*)(void*,void*,void*,int))vtable[0x52c/4])(entity,&source,&parameters,0);
    ((StrDtor)SEAM_StrDtor)(&parameter);((StrDtor)SEAM_StrDtor)(&source);
    return 1;
}
