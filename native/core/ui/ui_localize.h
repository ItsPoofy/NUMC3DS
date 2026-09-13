#ifndef NUMC3DS_UI_LOCALIZE_H
#define NUMC3DS_UI_LOCALIZE_H

#include "../rt.h"

void ui_localized_label(char *out,unsigned cap,const char *key,const char *fallback);
int ui_localized_string(const char *key,const char *fallback,u32 *out_handle);

#endif
