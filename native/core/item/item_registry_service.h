#ifndef NUMC3DS_ITEM_REGISTRY_SERVICE_H
#define NUMC3DS_ITEM_REGISTRY_SERVICE_H

#include "../../include/numc3ds_abi.h"

void* map_find_int(void* map, numc3ds_u32 key);
void* item_by_name(const char* name);
void* item_by_id(int id);
void* item_from_arg(const char* name);
void* block_by_name(const char* name);
void* supplies(void* player);
void* inventory_of(void* player);

#endif /* NUMC3DS_ITEM_REGISTRY_SERVICE_H */
