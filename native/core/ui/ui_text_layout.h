#ifndef NUMC3DS_UI_TEXT_LAYOUT_H
#define NUMC3DS_UI_TEXT_LAYOUT_H

#include "ui_runtime.h"

unsigned ui_text_wrap_line(void *font,const char *source,unsigned start,int max_width,char *line,unsigned capacity,unsigned *next);

unsigned ui_text_wrap_input_line(void *font,const char *source,unsigned start,int max_width,char *line,unsigned capacity,unsigned *next);

#endif
