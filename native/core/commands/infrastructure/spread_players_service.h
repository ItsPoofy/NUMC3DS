#ifndef NUMC3DS_SPREAD_PLAYERS_SERVICE_H
#define NUMC3DS_SPREAD_PLAYERS_SERVICE_H

typedef struct {
    float minimum_distance;
    unsigned iterations;
} SpreadPlayersResult;

int command_spread_players(void *level,void *source,void *const *targets,unsigned target_count,float center_x,float center_z,float spread_distance,float max_range,SpreadPlayersResult *result);

#endif
