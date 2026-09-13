#ifndef NUMC3DS_BLOCK_ENTITY_TRANSFER_H
#define NUMC3DS_BLOCK_ENTITY_TRANSFER_H

int block_entity_snapshot_save(void *source,const int position[3],void **tag_out);
int block_entity_snapshot_load(void *source,const int position[3],void *tag);
void block_entity_snapshot_destroy(void *tag);

#endif
