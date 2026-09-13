#ifndef NUMC3DS_INTELLISENSE_HANDLER_H
#define NUMC3DS_INTELLISENSE_HANDLER_H

#include "../commands/intellisense/command_parser_intellisense.h"

typedef struct {
    char *text;
    u32 item_instance[12];
    u32 has_item;
    u32 match_start;
    u32 match_length;
} IntellisenseAutoCompleteMessage;

typedef struct {
    char **intellisense_begin;
    char **intellisense_end;
    char **intellisense_capacity;
    char *tab_complete_input;
    u32 last_tab_complete_index;
    u8 reset_tab_complete;
    u8 needs_layout_update;
    u16 reserved;
    u32 auto_complete_grid_size;
    IntellisenseAutoCompleteMessage *messages_begin;
    IntellisenseAutoCompleteMessage *messages_end;
    IntellisenseAutoCompleteMessage *messages_capacity;
} IntellisenseHandler;

_Static_assert(__builtin_offsetof(IntellisenseHandler,tab_complete_input)==0x0c,"IntellisenseHandler tab input ABI");
_Static_assert(__builtin_offsetof(IntellisenseHandler,last_tab_complete_index)==0x10,"IntellisenseHandler tab index ABI");
_Static_assert(__builtin_offsetof(IntellisenseHandler,reset_tab_complete)==0x14,"IntellisenseHandler reset ABI");
_Static_assert(__builtin_offsetof(IntellisenseHandler,auto_complete_grid_size)==0x18,"IntellisenseHandler grid ABI");
_Static_assert(__builtin_offsetof(IntellisenseHandler,messages_begin)==0x1c,"IntellisenseHandler messages ABI");

void intellisense_handler_init(IntellisenseHandler *handler);
void intellisense_handler_destroy(IntellisenseHandler *handler);
void intellisense_handler_clear_messages(IntellisenseHandler *handler);
void intellisense_handler_reset_tab_complete_progress(IntellisenseHandler *handler);
void intellisense_handler_update(IntellisenseHandler *handler,void *screen,const char *input,u32 cursor);
int intellisense_handler_handle_tab_complete(IntellisenseHandler *handler,void *screen,char *input,u32 *length,u32 *cursor,u32 limit);
u32 intellisense_handler_get_auto_complete_grid_size(const IntellisenseHandler *handler);
void intellisense_handler_set_auto_complete_grid_size(IntellisenseHandler *handler,u32 size);
u32 intellisense_handler_get_last_tab_complete_index(const IntellisenseHandler *handler);
int intellisense_handler_get_needs_layout_update(const IntellisenseHandler *handler);
void intellisense_handler_set_needs_layout_update(IntellisenseHandler *handler,int value);
u32 intellisense_handler_get_display_count(const IntellisenseHandler *handler);
const char *intellisense_handler_get_auto_complete_text(const IntellisenseHandler *handler,u32 index);
const void *intellisense_handler_get_auto_complete_item(const IntellisenseHandler *handler,u32 index);
u32 intellisense_handler_get_auto_complete_match_start(const IntellisenseHandler *handler,u32 index);
u32 intellisense_handler_get_auto_complete_match_length(const IntellisenseHandler *handler,u32 index);

#endif
