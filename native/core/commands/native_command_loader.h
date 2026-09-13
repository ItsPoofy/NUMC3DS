#ifndef NUMC3DS_NATIVE_COMMAND_LOADER_H
#define NUMC3DS_NATIVE_COMMAND_LOADER_H

#include "native_schema.h"

int native_command_loader_load(void *parser);
const NativeSharedCommand *native_command_loader_versions(unsigned root_index);

#endif
