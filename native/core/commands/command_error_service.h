#ifndef NUMC3DS_COMMAND_ERROR_SERVICE_H
#define NUMC3DS_COMMAND_ERROR_SERVICE_H

#include "../../include/numc3ds_abi.h"

extern const char command_sent[];
extern const char command_unavailable[];
extern const char command_failed[];
extern const char command_permission[];
extern const char command_syntax[];
extern const char sent[];

void result_text(const char* t);
void result_number(const char* prefix, int value);
int fail_syntax(void);
int fail_command(const char* message);
int fail_command_localized(const char* key);
int fail_command_localized_args(const char* key, const char* const* arguments, unsigned argument_count);
int fail_number_too_small(int value, int minimum);
int fail_number_too_big(int value, int maximum);
void item_not_found(const char* name);
int send_command_chat(void* player, const char* source_text, const char* message);
void action_message(void* player, const char* message);

#endif /* NUMC3DS_COMMAND_ERROR_SERVICE_H */
