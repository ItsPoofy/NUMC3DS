#ifndef NUMC3DS_MOB_EQUIPMENT_SYNC_SERVICE_H
#define NUMC3DS_MOB_EQUIPMENT_SYNC_SERVICE_H

int command_mob_sync_equipment(void *mob,const void *item,int slot,int selected_slot,unsigned container_id);
int command_mob_sync_armor(void *mob);

#endif
