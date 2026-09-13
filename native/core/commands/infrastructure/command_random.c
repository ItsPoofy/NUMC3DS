#include "../command_random.h"
#include "../../internal.h"

extern numc3ds_u32 rng_next(void);

static numc3ds_u32 prng_next_u32(void) {
    if(!rng_state) rng_state = 0x12345678u;
    return rng_next();
}

int command_random_construct(CommandRandom *random){
    if(!random)return 0;
    zero(random,sizeof(*random));
    return 1;
}

unsigned command_random_local_next_u32(CommandRandom *random){
    (void)random;
    return prng_next_u32();
}

float command_random_local_next_float(CommandRandom *random,float low,float high){
    (void)random;
    return low+((float)prng_next_u32()/4294967296.0f)*(high-low);
}

unsigned command_random_next_u32(void *level){
    (void)level;
    return prng_next_u32();
}

int command_random_next_int(void *level,int bound){
    (void)level;
    if(bound<=0)return 0;
    return (int)(prng_next_u32()%(unsigned)bound);
}

float command_random_next_float(void *level,float low,float high){
    (void)level;
    return low+((float)prng_next_u32()/4294967296.0f)*(high-low);
}
