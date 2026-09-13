#include "../block_mutation_service.h"
#include "../../internal.h"

typedef int (*BlockEntityIsTypeFn)(void *,int);
typedef void (*BlockEntitySetChangedFn)(void *);
typedef void (*SkullBlockEntitySetTypeFn)(void *,int);
typedef void *(*BlockSourceGetLevelFn)(void *);
typedef void *(*BlockSourceGetBlockEntityFn)(void *,const int *);

static int set_raw(void *source,const int position[3],unsigned char id,unsigned char data,int flags){
    return ((SetBlockIdDataFn)SEAM_BlockSource_setBlockAndDataIdData)(source,(void *)position,&id,data,flags,0);
}

static int is_skull(unsigned char id){
    void *block=block_by_name("skull");
    return block&&*((unsigned char *)block+4)==id;
}

unsigned char command_block_read_id(void *source,const int position[3]){
    unsigned char id=0;
    ((GetBlockIdFn)SEAM_BlockSource_getBlockId)(&id,source,(void *)position);
    return id;
}

unsigned char command_block_read_data(void *source,const int position[3]){
    unsigned char block[2]={0,0};
    ((GetBlockIdAndDataFn)SEAM_BlockSource_getBlockIdAndData)(block,source,(void *)position);
    return block[1];
}

int command_block_is_empty(void *source,const int position[3]){
    return ((int (*)(void *,const int *))SEAM_BlockSource_isEmptyBlock)(source,position);
}

void command_block_description_name(unsigned char id,unsigned char data,char *out,unsigned capacity){
    void *block=((void **)SEAM_BlockRegistry_byNumericId)[id];u32 name=0;const char *text=0;
    if(!out||!capacity)return;
    out[0]=0;
    if(block){
        void **vtable=*(void ***)block;
        if(vtable&&vtable[0x158/4])((void (*)(void *,void *,unsigned char))vtable[0x158/4])(&name,block,data);
    }
    if(name)cp(&text,&name,sizeof(text));
    if(text&&text[0])copy_text(out,text,capacity-1);
    else{unsigned length=0;append(out,&length,"#");append_int(out,&length,id);}
    if(name)drop_str(&name);
}

int command_block_set(void *source,const int position[3],unsigned char id,unsigned char data){
    void *entity;
    if(!source||!position)return 0;
    if(!is_skull(id))return set_raw(source,position,id,data,3);
    set_raw(source,position,id,data,4);
    entity=((BlockSourceGetBlockEntityFn)SEAM_BlockSource_getBlockEntity)(source,position);
    if(!entity||!((BlockEntityIsTypeFn)SEAM_BlockEntity_isType)(entity,6))return 0;
    ((BlockEntitySetChangedFn)SEAM_BlockEntity_setChanged)(entity);
    ((SkullBlockEntitySetTypeFn)SEAM_SkullBlockEntity_setSkullType)(entity,data);
    *(int *)((unsigned char *)entity+SEAM_SkullBlockEntity_rotationOffset)=0;
    set_raw(source,position,id,1,3);
    return 1;
}

void command_block_update_neighbors(void *source,const int position[3]){
    if(source&&position)((void (*)(void *,const int *))SEAM_BlockSource_updateNeighborsAt)(source,position);
}

void command_block_destroy(void *source,const int position[3],int drops){
    void *level;
    if(!source||!position)return;
    level=((BlockSourceGetLevelFn)SEAM_BlockSource_getLevel)(source);
    if(level)((LevelDestroyBlockFn)SEAM_Level_destroyBlock)(level,source,position,drops);
}
