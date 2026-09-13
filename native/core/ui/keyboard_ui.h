#ifndef NUMC3DS_KEYBOARD_UI_H
#define NUMC3DS_KEYBOARD_UI_H
enum { UI_KEYBOARD_ALLOW_TAB=1, UI_KEYBOARD_ALLOW_COMMAND=2 };
enum { UI_KEYBOARD_CHANGED, UI_KEYBOARD_TAB, UI_KEYBOARD_COMMAND, UI_KEYBOARD_UP, UI_KEYBOARD_DOWN, UI_KEYBOARD_PAGE_PREV, UI_KEYBOARD_PAGE_NEXT, UI_KEYBOARD_SUBMIT, UI_KEYBOARD_CANCEL, UI_KEYBOARD_DRAW };
typedef void (*UiKeyboardAction)(void *screen,int action);
int ui_keyboard_open(void *owner,const char *initial,unsigned limit,unsigned flags,UiKeyboardAction callback);
void ui_keyboard_set_text(const char *text);
void ui_keyboard_reset(void);
#endif
