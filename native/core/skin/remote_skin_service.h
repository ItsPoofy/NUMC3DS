#ifndef NUMC3DS_REMOTE_SKIN_SERVICE_H
#define NUMC3DS_REMOTE_SKIN_SERVICE_H

#include "skin_types.h"
#include "../commands/native_types.h"

int remote_skin_service_install_hook(void);
void *remote_skin_service_find(const char *name, u32 name_len);
int remote_skin_service_register(const u8 *uuid_16, const char *skin_id,
                                 const u8 *rgba_bytes, u32 rgba_len,
                                 char *out_registered_name, u32 out_capacity);
void remote_skin_service_reset(void);

#endif
