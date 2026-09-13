#ifndef NUMC3DS_CHAT_UI_H
#define NUMC3DS_CHAT_UI_H

#include "../rt.h"
#include "text_entry_session.h"

int chat_ui_install_hooks(void);
void chat_ui_set_input(const char *text);
void chat_ui_flush_session(void);

#endif
