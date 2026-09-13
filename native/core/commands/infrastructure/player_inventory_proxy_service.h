#ifndef NUMC3DS_PLAYER_INVENTORY_PROXY_SERVICE_H
#define NUMC3DS_PLAYER_INVENTORY_PROXY_SERVICE_H

void *command_player_inventory_proxy_inventory(void *proxy);
int command_player_inventory_proxy_set_item(void *proxy,int slot,const void *item);

#endif
