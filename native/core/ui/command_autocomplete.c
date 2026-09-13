#include "command_autocomplete.h"
#include "intellisense_handler.h"
#include "../internal.h"

static IntellisenseHandler *handler(void){
    IntellisenseHandler *value=(IntellisenseHandler*)s->intellisense_handler;
    if(!value){value=s->host.heap_alloc(sizeof(*value));if(!value)return 0;intellisense_handler_init(value);s->intellisense_handler=value;}
    return value;
}

void command_autocomplete_reset(void){
    IntellisenseHandler *value=handler();if(value){intellisense_handler_clear_messages(value);intellisense_handler_reset_tab_complete_progress(value);}s->autocomplete_scroll=0;
}

void command_autocomplete_destroy(void){IntellisenseHandler *value=(IntellisenseHandler*)s->intellisense_handler;if(!value)return;intellisense_handler_destroy(value);s->host.heap_free(value);s->intellisense_handler=0;s->autocomplete_scroll=0;}

void command_autocomplete_update(void *screen){
    IntellisenseHandler *value=handler();if(!value)return;intellisense_handler_reset_tab_complete_progress(value);intellisense_handler_update(value,screen,s->keyboard.text,s->keyboard.cursor);s->autocomplete_scroll=0;
}

int command_autocomplete_apply(void *screen){
    IntellisenseHandler *value=handler();return value?intellisense_handler_handle_tab_complete(value,screen,s->keyboard.text,&s->keyboard.length,&s->keyboard.cursor,s->keyboard.input_limit):0;
}

int command_autocomplete_active(void){IntellisenseHandler *value=handler();return value&&intellisense_handler_get_display_count(value)!=0;}
unsigned command_autocomplete_count(void){IntellisenseHandler *value=handler();return value?intellisense_handler_get_display_count(value):0;}
unsigned command_autocomplete_selected(void){return 0;}
unsigned command_autocomplete_scroll(void){return s->autocomplete_scroll;}
void command_autocomplete_set_scroll(unsigned scroll){s->autocomplete_scroll=scroll;}
const char *command_autocomplete_option(unsigned index){IntellisenseHandler *value=handler();return value?intellisense_handler_get_auto_complete_text(value,index):"";}
const void *command_autocomplete_item(unsigned index){IntellisenseHandler *value=handler();return value?intellisense_handler_get_auto_complete_item(value,index):0;}
unsigned command_autocomplete_match_start(unsigned index){IntellisenseHandler *value=handler();return value?intellisense_handler_get_auto_complete_match_start(value,index):0;}
unsigned command_autocomplete_match_length(unsigned index){IntellisenseHandler *value=handler();return value?intellisense_handler_get_auto_complete_match_length(value,index):0;}
