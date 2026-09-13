#ifndef NUMC3DS_ENTITY_TELEPORT_H
#define NUMC3DS_ENTITY_TELEPORT_H

int native_entity_teleport(void *entity,const float position[3]);
int native_entity_teleport_to_coordinates(void *entity,const int position[3]);
unsigned native_entity_teleport_multiple(void **entities,unsigned count,const float position[3],void **successful_out);
unsigned native_entity_teleport_multiple_to_coordinates(void **entities,unsigned count,const int position[3],void **successful_out);

#endif
