#include "../item_validation_service.h"
#include "../../internal.h"

typedef int (*ItemAuxValidFn)(void *,int);

CommandItemValidation command_item_validate_instance(const void *item_instance){
    void *item;
    int auxiliary_value;

    if(!item_instance||*(const unsigned char *)item_instance==0){
        return COMMAND_ITEM_NOT_FOUND;
    }

    /* In 3DS ItemInstance: +0x04 is valid (u8), +0x0c is item* */
    if(!*(const unsigned char *)((const unsigned char *)item_instance+4)){
        return COMMAND_ITEM_NOT_FOUND;
    }

    item=*(void *const *)((const unsigned char *)item_instance+0xc);
    if(!item){
        return COMMAND_ITEM_NOT_FOUND;
    }

    auxiliary_value=(int)*(const unsigned short *)((const unsigned char *)item_instance+2);
    if(auxiliary_value<0||auxiliary_value>32767){
        return COMMAND_ITEM_INVALID;
    }

    return COMMAND_ITEM_VALID;
}
