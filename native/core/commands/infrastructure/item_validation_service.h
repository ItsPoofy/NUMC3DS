#ifndef NUMC3DS_ITEM_VALIDATION_SERVICE_H
#define NUMC3DS_ITEM_VALIDATION_SERVICE_H

typedef enum {
    COMMAND_ITEM_VALID = 0,
    COMMAND_ITEM_NOT_FOUND,
    COMMAND_ITEM_INVALID
} CommandItemValidation;

CommandItemValidation command_item_validate_instance(const void *item_instance);

#endif
