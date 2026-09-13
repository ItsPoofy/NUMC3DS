#ifndef NUMC3DS_INVENTORY_MUTATION_SERVICE_H
#define NUMC3DS_INVENTORY_MUTATION_SERVICE_H

int command_inventory_clear_all(void *player);
int command_inventory_count_resource(void *player,void *item);
int command_inventory_remove_resource(void *player,void *item,int data,int maximum);
int command_inventory_clear_player(void *player,void *item,int data,int max_count);
int command_inventory_clear_players(void **players,unsigned count,void *item,int data,int max_count,int *removed_total);

#endif
