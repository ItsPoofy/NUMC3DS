#ifndef NUMC3DS_WEATHER_COMMAND_SERVICE_H
#define NUMC3DS_WEATHER_COMMAND_SERVICE_H

void *command_weather_get(void *level);
void command_weather_apply(void *level,int raining,int thunder,int duration_ticks);
void command_weather_broadcast(void *level,int raining,int thunder);
int command_weather_toggle_downfall(void *level);

#endif
