#ifndef NUMC3DS_NATIVE_COMMAND_REGISTRY_H
#define NUMC3DS_NATIVE_COMMAND_REGISTRY_H

#include "native_types.h"

int native_registry_attach(void *minecraft_commands,void *game);
int native_registry_is_attached(void *minecraft_commands);
void *native_registry_commands_for_game(void *game);
void native_command_trace(char marker);
void native_command_trace_begin(void);
const char *native_command_trace_end(void);

#endif
