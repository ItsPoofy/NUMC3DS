#ifndef NUMC3DS_COMMAND_RANDOM_H
#define NUMC3DS_COMMAND_RANDOM_H

typedef struct {
    unsigned words[0x275];
} CommandRandom;

int command_random_construct(CommandRandom *random);
unsigned command_random_local_next_u32(CommandRandom *random);
float command_random_local_next_float(CommandRandom *random,float low,float high);
unsigned command_random_next_u32(void *level);
int command_random_next_int(void *level,int bound);
float command_random_next_float(void *level,float low,float high);

#endif
