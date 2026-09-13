#include "ui_localize.h"
#include "../internal.h"

int ui_localized_string(const char *key,const char *fallback,u32 *out_handle){u32 source=0,result=0,scratch=0;const char *text=0;if(!out_handle)return 0;*out_handle=0;if(key&&key[0]){((StrCtor)SEAM_StrCtor)(&source,key,&scratch);((LocalizationGetFn)SEAM_Localization_get)(&result,&source,0);if(result)cp(&text,&result,sizeof(text));if(text&&text[0]&&!streq(text,key))((StrCtor)SEAM_StrCtor)(out_handle,text,&scratch);if(result)((StrDtor)SEAM_StrDtor)(&result);((StrDtor)SEAM_StrDtor)(&source);}if(!*out_handle)((StrCtor)SEAM_StrCtor)(out_handle,fallback?fallback:(key?key:""),&scratch);return *out_handle!=0;}
void ui_localized_label(char *out,unsigned cap,const char *key,const char *fallback){u32 handle=0;const char *text=0;if(!out||!cap)return;out[0]=0;if(ui_localized_string(key,fallback,&handle)){cp(&text,&handle,sizeof(text));copy_text(out,text?text:"",cap-1);((StrDtor)SEAM_StrDtor)(&handle);}}
