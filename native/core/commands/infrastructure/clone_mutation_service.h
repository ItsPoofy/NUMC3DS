#ifndef NUMC3DS_CLONE_MUTATION_SERVICE_H
#define NUMC3DS_CLONE_MUTATION_SERVICE_H

#include "command_region.h"

enum {
    CLONE_MUTATION_REPLACE = 0,
    CLONE_MUTATION_MASKED = 1,
    CLONE_MUTATION_FILTERED = 2
};

typedef struct {
    int mask;
    int move;
    unsigned char filter_id;
    unsigned char filter_data;
    int has_filter_data;
} CloneMutationOptions;

int command_clone_apply(void *source,const CommandRegion *region,const int destination[3],const CloneMutationOptions *options,int *copied_out);

#endif
