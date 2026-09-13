#ifndef NUMC3DS_INVENTORY_ACTION_SERVICE_H
#define NUMC3DS_INVENTORY_ACTION_SERVICE_H

int command_inventory_action_give(void *level,void *player,const void *item_instance);
int command_inventory_action_enchant(void *level,void *player,const void *item_instance,int enchantment_id,int enchantment_level);

#endif
