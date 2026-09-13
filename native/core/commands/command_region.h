#ifndef NUMC3DS_COMMAND_REGION_H
#define NUMC3DS_COMMAND_REGION_H

typedef struct {
    int minimum[3];
    int maximum[3];
    int volume;
} CommandRegion;

int command_region_from_bounds(const int first[3],const int second[3],CommandRegion *out);

#endif
