#include "text_entry_view.h"
#include "text_entry_session.h"
#include "keyboard_ui.h"
#include "command_autocomplete.h"
#include "../internal.h"
static UiTextEntryComplete completion;
static void *completion_context;
static const char *title_key="advMode.command";
const char *ui_text_entry_title(void){return title_key;}
void ui_text_entry_set_title(const char *key){title_key=key;}
int ui_text_entry_active(void){return completion!=0;}
void ui_text_entry_finish(int accepted){
    UiTextEntryComplete callback=completion;void*context=completion_context;
    if(!callback)return;completion=0;completion_context=0;
    ui_text_entry_view_reset();command_autocomplete_reset();callback(context,s->keyboard.text,accepted);
}
static void entry_action(void*screen,int action){
    if(action==UI_KEYBOARD_SUBMIT)ui_text_entry_finish(1);
    else if(action==UI_KEYBOARD_CANCEL)ui_text_entry_finish(0);
    else if(action==UI_KEYBOARD_DRAW)ui_text_entry_draw(screen);
    else if(action==UI_KEYBOARD_TAB)command_autocomplete_apply(screen);
    else if(action==UI_KEYBOARD_CHANGED)command_autocomplete_reset();
}
int ui_text_entry_open(void*owner,const char*initial,unsigned limit,int command,UiTextEntryComplete complete,void*context){
    if(completion||!complete)return 0;
    completion=complete;completion_context=context;command_autocomplete_reset();
    if(!ui_keyboard_open(owner,initial,limit,command?UI_KEYBOARD_ALLOW_TAB:0,entry_action)){completion=0;completion_context=0;return 0;}
    return 1;
}
