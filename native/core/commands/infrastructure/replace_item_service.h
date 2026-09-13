#ifndef NUMC3DS_REPLACE_ITEM_SERVICE_H
#define NUMC3DS_REPLACE_ITEM_SERVICE_H

typedef enum {
    REPLACE_ITEM_OK,
    REPLACE_ITEM_NO_CONTAINER,
    REPLACE_ITEM_BAD_SLOT,
    REPLACE_ITEM_FAILED
} ReplaceItemResult;

ReplaceItemResult replace_item_block(void *source,const int position[3],const char *slot_type,int slot_id,const char *item_name,int amount,int data,const char *components,int *maximum_slot);
ReplaceItemResult replace_item_entity(void *entity,const char *slot_type,int slot_id,const char *item_name,int amount,int data,const char *components,int *maximum_slot);

#endif
