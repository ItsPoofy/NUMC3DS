#ifndef NUMC3DS_ENTITY_CLASSIFICATION_H
#define NUMC3DS_ENTITY_CLASSIFICATION_H

int command_entity_is_player(void *entity);
int command_entity_is_mob(void *entity);
int command_entity_is_type(void *entity,unsigned type);

#endif
