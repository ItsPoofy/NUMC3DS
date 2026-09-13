#ifndef NUMC3DS_COMMAND_AUTOCOMPLETE_H
#define NUMC3DS_COMMAND_AUTOCOMPLETE_H

void command_autocomplete_reset(void);
void command_autocomplete_destroy(void);
void command_autocomplete_update(void *screen);
int command_autocomplete_apply(void *screen);
int command_autocomplete_active(void);
unsigned command_autocomplete_count(void);
unsigned command_autocomplete_selected(void);
unsigned command_autocomplete_scroll(void);
void command_autocomplete_set_scroll(unsigned scroll);
const char *command_autocomplete_option(unsigned index);
const void *command_autocomplete_item(unsigned index);
unsigned command_autocomplete_match_start(unsigned index);
unsigned command_autocomplete_match_length(unsigned index);

#endif
