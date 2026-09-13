#ifndef NUMC3DS_ENTITY_DAMAGE_SERVICE_H
#define NUMC3DS_ENTITY_DAMAGE_SERVICE_H

int command_entity_kill(void *entity);
unsigned command_entity_kill_multiple(void **entities, unsigned count, void **killed_out);
int command_entity_is_alive(const void *entity);

#endif
