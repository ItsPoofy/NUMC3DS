#ifndef NUMC3DS_ENTITY_ROTATION_H
#define NUMC3DS_ENTITY_ROTATION_H

int command_entity_get_rotation(const void *entity,float rotation[2]);
int command_entity_resolve_rotation(void *entity,const float requested[2],const int relative[2],const int provided[2],float resolved[2]);

#endif
