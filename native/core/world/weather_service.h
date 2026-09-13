#ifndef NUMC3DS_WEATHER_SERVICE_H
#define NUMC3DS_WEATHER_SERVICE_H

#include "../../include/numc3ds_abi.h"

void* weather_of(void* level);
void weather_apply(void* level, int raining, int thunder, int durationTicks);

#endif /* NUMC3DS_WEATHER_SERVICE_H */
