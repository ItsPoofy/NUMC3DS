#ifndef NUMC3DS_ENTITY_SPAWN_SERVICE_H
#define NUMC3DS_ENTITY_SPAWN_SERVICE_H

int command_spawn_entity_by_id(void *level,void *source,int entity_id,const float position[3]);
int command_spawn_source_has_block(void *source,const int position[3]);

#endif
