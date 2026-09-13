#ifndef NUMC3DS_TEXT_ENTRY_SESSION_H
#define NUMC3DS_TEXT_ENTRY_SESSION_H

typedef void (*UiTextEntryComplete)(void *context,const char *text,int accepted);
int ui_text_entry_open(void *owner,const char *initial,unsigned limit,int command,UiTextEntryComplete complete,void *context);
int ui_text_entry_active(void);
void ui_text_entry_finish(int accepted);

const char *ui_text_entry_title(void);
void ui_text_entry_set_title(const char *key);
#endif
