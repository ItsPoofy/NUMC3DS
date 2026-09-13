#ifndef NUMC3DS_BLOCK_READ_FAST_H
#define NUMC3DS_BLOCK_READ_FAST_H

int block_read_fast_install_hooks(void);
void block_read_terrain_scope_enter(void);
void block_read_terrain_scope_leave(void);
void block_read_stock_id(unsigned char *result, const void *source, const int *position);
void block_read_stock_id_data(unsigned char *result, const void *source, const int *position);

#endif
