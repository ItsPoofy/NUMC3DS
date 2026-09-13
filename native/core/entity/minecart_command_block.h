#ifndef NUMC3DS_MINECART_COMMAND_BLOCK_H
#define NUMC3DS_MINECART_COMMAND_BLOCK_H

#include "../rt.h"

int is_minecart_command_block(void *entity);
void *minecart_command_block_get_or_create_component(void *entity);
void minecart_command_block_cleanup(void *entity);
int minecart_command_block_install_hooks(void);

#endif /* NUMC3DS_MINECART_COMMAND_BLOCK_H */
