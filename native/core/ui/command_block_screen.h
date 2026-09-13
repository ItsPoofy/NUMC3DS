#ifndef NUMC3DS_COMMAND_BLOCK_SCREEN_H
#define NUMC3DS_COMMAND_BLOCK_SCREEN_H

int command_block_screen_install(void);
void command_block_screen_reset(void);
void command_block_screen_open(void *player,const int *position);
void command_block_screen_open_entity(void *player,void *entity);

#endif
