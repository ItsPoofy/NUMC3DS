#ifndef NUMC3DS_COMMAND_BLOCK_MODEL_H
#define NUMC3DS_COMMAND_BLOCK_MODEL_H

#include "../../rt.h"

typedef struct {
    void *player;
    void *level;
    void *entity;
    int position[3];
    u32 command, name, output;
    u8 mode, conditional, redstone, track_output;
    u8 is_minecart;
} CommandBlockModel;

int command_block_model_init(CommandBlockModel *model,void *player,const int position[3]);
int command_block_model_init_entity(CommandBlockModel *model,void *player,void *entity);
int command_block_model_valid(const CommandBlockModel *model);
int command_block_model_save(CommandBlockModel *model);
void command_block_model_destroy(CommandBlockModel *model);
void command_block_model_set_text(u32 *field,const char *text);
const char *command_block_model_text(u32 field);
int command_block_can_use(void *player);

#endif
