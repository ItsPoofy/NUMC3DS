#ifndef NUMC3DS_SKIN_SERVICE_H
#define NUMC3DS_SKIN_SERVICE_H

#include "skin_types.h"

int skin_service_init(void);
int skin_service_scan_skins(void);
int skin_service_get_count(void);
CustomSkinEntry *skin_service_get_entry(int index);
CustomSkinEntry *skin_service_get_hovered_entry(void);
int skin_service_get_hovered_index(void);
void skin_service_set_hovered_index(int index);
int skin_service_toggle_model(int index);
const char *skin_service_get_model_name(int model_type);

#endif /* NUMC3DS_SKIN_SERVICE_H */
