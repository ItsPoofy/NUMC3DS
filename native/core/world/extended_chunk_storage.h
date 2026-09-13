#ifndef NUMC3DS_EXTENDED_CHUNK_STORAGE_H
#define NUMC3DS_EXTENDED_CHUNK_STORAGE_H

#include "../rt.h"

int extended_chunk_storage_install_hooks(void);
u8 *extended_chunk_get_subchunk(const void *chunk, u32 subchunk_index);
u8 *extended_chunk_get_or_create_subchunk(void *chunk, u32 subchunk_index);
void extended_chunk_on_chunk_destroy(void *chunk);
void extended_chunk_storage_reset(void);

#endif
