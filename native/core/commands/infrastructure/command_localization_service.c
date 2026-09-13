#include "../command_localization.h"
#include "../native_types.h"
#include "../../internal.h"

int command_localize(char *out,unsigned capacity,const char *key,const char *const *arguments,unsigned argument_count){
    NativeGstdString source,result,values[16];NativeVector vector;u32 scratch=0;const char *text=0;unsigned index;
    if(!out||!capacity||!key||argument_count>16)return 0;
    out[0]=0;zero(&source,sizeof(source));zero(&result,sizeof(result));zero(values,sizeof(values));zero(&vector,sizeof(vector));
    for(index=0;index<argument_count;index++)((StrCtor)SEAM_StrCtor)(&values[index],arguments&&arguments[index]?arguments[index]:"",&scratch);
    ((StrCtor)SEAM_StrCtor)(&source,key,&scratch);
    vector.begin=(u32)values;vector.end=(u32)(values+argument_count);vector.capacity=vector.end;
    ((LocalizationGetFn)SEAM_Localization_get)(&result,&source,argument_count?&vector:0);
    if(result.handle)text=(const char*)result.handle;
    if(text&&text[0]&&!streq(text,key))copy_text(out,text,capacity-1);
    if(result.handle)((StrDtor)SEAM_StrDtor)(&result);
    if(source.handle)((StrDtor)SEAM_StrDtor)(&source);
    for(index=0;index<argument_count;index++)if(values[index].handle)((StrDtor)SEAM_StrDtor)(&values[index]);
    return out[0]!=0;
}
