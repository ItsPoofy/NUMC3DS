#ifndef NUMC3DS_COMMAND_ORIGIN_ACCESS_H
#define NUMC3DS_COMMAND_ORIGIN_ACCESS_H

void *native_command_origin_level(void *origin);
void *native_command_origin_entity(void *origin);
void *native_command_origin_block_source(void *origin);
int native_command_origin_block_position(void *origin,int position[3]);
int native_command_origin_world_position(void *origin,float position[3]);
int native_command_origin_name(void *origin,char *out,unsigned capacity);

#endif
