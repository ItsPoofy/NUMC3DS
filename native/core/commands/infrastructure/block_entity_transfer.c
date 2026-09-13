#include "../block_entity_transfer.h"
#include "../../internal.h"

typedef void *(*BlockEntityFn)(void *,const int *);
typedef void (*BlockEntityLoadFn)(void *,void *);
typedef void (*BlockEntitySaveFn)(void *,void *);
typedef void (*BlockEntityChangedFn)(void *);

int block_entity_snapshot_save(void *source,const int position[3],void **tag_out){
    void *entity,*tag;void **vtable;if(!tag_out)return -1;*tag_out=0;if(!source||!position)return -1;
    entity=((BlockEntityFn)SEAM_BlockSource_getBlockEntity)(source,position);if(!entity)return 0;vtable=*(void ***)entity;if(!vtable||!vtable[1])return -1;
    tag=s->host.heap_alloc(0x1c);if(!tag)return -1;((void (*)(void *))SEAM_CompoundTag_ctor)(tag);((BlockEntitySaveFn)vtable[1])(entity,tag);*tag_out=tag;return 1;
}

int block_entity_snapshot_load(void *source,const int position[3],void *tag){
    void *entity;void **vtable;if(!source||!position||!tag)return 0;entity=((BlockEntityFn)SEAM_BlockSource_getBlockEntity)(source,position);if(!entity)return 0;
    vtable=*(void ***)entity;if(!vtable||!vtable[0])return 0;((BlockEntityLoadFn)vtable[0])(entity,tag);((BlockEntityChangedFn)SEAM_BlockEntity_setChanged)(entity);return 1;
}

void block_entity_snapshot_destroy(void *tag){if(!tag)return;((void (*)(void *))SEAM_CompoundTag_dtor)(tag);s->host.heap_free(tag);}
