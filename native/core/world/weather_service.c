#include "weather_service.h"
#include "../rt.h"

void* weather_of(void* level) {
    void* dim = ((GetDimensionFn)0x00720854u)(level, 0);
    return dim ? *(void**)((unsigned char*)dim + 0xD0) : 0;
}

static void weather_sync_mirror(void* w, float rain, float lit) {
    if (!w) return;
    *(float*)((unsigned char*)w + 0x18) = rain;
    *(float*)((unsigned char*)w + 0x1C) = rain;
    *(float*)((unsigned char*)w + 0x20) = rain;
    *(float*)((unsigned char*)w + 0x2C) = lit;
}

void weather_apply(void* level, int raining, int thunder, int durationTicks) {
    unsigned char* L = (unsigned char*)level;
    float rain = raining ? 1.0f : 0.0f;
    float lit = thunder ? 1.0f : 0.0f;
    if (durationTicks < 20) durationTicks = 20;
    *(float*)(L + 0x130) = rain;
    *(int*)(L + 0x134) = durationTicks;
    *(float*)(L + 0x138) = lit;
    *(int*)(L + 0x13C) = durationTicks;
    weather_sync_mirror(weather_of(level), rain, lit);
}
