#ifndef NUMC3DS_BLOCK_MUTATION_SERVICE_H
#define NUMC3DS_BLOCK_MUTATION_SERVICE_H

unsigned char command_block_read_id(void *source,const int position[3]);
unsigned char command_block_read_data(void *source,const int position[3]);
int command_block_is_empty(void *source,const int position[3]);
int command_block_set(void *source,const int position[3],unsigned char id,unsigned char data);
void command_block_update_neighbors(void *source,const int position[3]);
void command_block_destroy(void *source,const int position[3],int drops);
void command_block_description_name(unsigned char id,unsigned char data,char *out,unsigned capacity);

#endif
