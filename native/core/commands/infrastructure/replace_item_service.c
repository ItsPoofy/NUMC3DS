#include "../replace_item_service.h"
#include "../item_components.h"
#include "../entity_classification.h"
#include "../mob_equipment_sync_service.h"
#include "../player_inventory_proxy_service.h"
#include "../../internal.h"

typedef void *(*BlockEntityFn)(void *,const int *);
typedef void (*ContainerSetItemFn)(void *,int,const void *);
typedef int (*ContainerSizeFn)(void *);
typedef int (*ItemInstanceIsNullFn)(const void *);
typedef void (*MobItemSlotFn)(void *,int,const void *);
typedef void (*MobOffhandItemFn)(void *,const void *);
typedef int (*FillingContainerLinkedSlotFn)(void *,int);
typedef int (*FillingContainerLinkSlotFn)(void *,int,int);
typedef int (*FillingContainerNextEmptySlotFn)(void *);
typedef int (*ItemInstanceHorseArmorFn)(const void *);
typedef int (*ItemInstanceArmorFn)(const void *);
typedef int (*ItemInstanceArmorSlotFn)(const void *);

enum { ENTITY_TYPE_MOB=0x0100,ENTITY_TYPE_HORSE_FAMILY=0x205300,ENTITY_TYPE_HORSE=0x205317,ENTITY_TYPE_DONKEY=0x205318,ENTITY_TYPE_MULE=0x205319,ENTITY_TYPE_BLOCK_CONTAINER_MAX=0x19,ENTITY_CONTAINER_TYPE_MASK=0x0220e106,ENTITY_BLOCK_COMPONENT_OFFSET=0x64 };

static int equipment_slot(const char *name){
    if(!name)return -1;if(streq(name,"slot.weapon.mainhand"))return 0;if(streq(name,"slot.weapon.offhand"))return 1;if(streq(name,"slot.armor.head"))return 2;if(streq(name,"slot.armor.chest"))return 3;if(streq(name,"slot.armor.legs"))return 4;if(streq(name,"slot.armor.feet"))return 5;if(streq(name,"slot.hotbar"))return 6;if(streq(name,"slot.inventory"))return 7;if(streq(name,"slot.enderchest"))return 8;if(streq(name,"slot.saddle"))return 9;if(streq(name,"slot.armor"))return 10;if(streq(name,"slot.chest"))return 11;return -1;
}

static int make_item(void *storage,const char *name,int amount,int data){
    void *item;if(!storage||!name)return 0;item=item_from_arg(name);if(!item||amount<1||amount>32767||data<0||data>32767)return 0;zero(storage,192);((ItemCountAuxCtorFn)SEAM_ItemInstance_itemCountAuxCtor)(storage,item,amount,data);return 1;
}

static void destroy_item(void *storage){if(storage)((ItemInstanceDtorFn)SEAM_ItemInstance_dtor)(storage);}
static void notify_player_inventory_changed(void *player){void **vtable;if(!player)return;vtable=*(void ***)player;if(vtable&&vtable[0x3fc/4])((void (*)(void *))vtable[0x3fc/4])(player);}

static int container_set(void *container,int slot,const void *item,int *maximum_slot){
    void **vtable;int size;if(maximum_slot)*maximum_slot=-1;if(!container||!item||slot<0)return 0;vtable=*(void ***)container;if(!vtable||!vtable[0x28/4]||!vtable[0x1c/4])return 0;size=((ContainerSizeFn)vtable[0x28/4])(container);if(maximum_slot)*maximum_slot=size-1;if(slot>=size)return 0;((ContainerSetItemFn)vtable[0x1c/4])(container,slot,item);return 1;
}

static void *entity_container_component(void *entity){return entity?*(void **)((u8 *)entity+SEAM_Entity_containerComponentOffset):0;}
static void *component_container(void *component){return component?*(void **)((u8 *)component+SEAM_ContainerComponent_containerOffset):0;}

static int component_set_item(void *component,int slot,const void *item,int *maximum_slot){if(!component||!item||((ItemInstanceIsNullFn)SEAM_ItemInstance_isNull)(item))return 0;return container_set(component_container(component),slot,item,maximum_slot);}

static int is_container_block_entity(void *entity){int type;if(!entity)return 0;type=*(int *)((u8 *)entity+0x4c);return type>=0&&type<=ENTITY_TYPE_BLOCK_CONTAINER_MAX&&(((unsigned)1u<<(unsigned)type)&ENTITY_CONTAINER_TYPE_MASK)!=0;}

ReplaceItemResult replace_item_block(void *source,const int position[3],const char *slot_type,int slot_id,const char *item_name,int amount,int data,const char *components,int *maximum_slot){
    void *block_entity;u32 item_storage[48];int ok;if(maximum_slot)*maximum_slot=-1;if(!source||!position||!slot_type||!item_name||!streq(slot_type,"slot.container"))return REPLACE_ITEM_NO_CONTAINER;
    if(slot_id<0||slot_id>255)return REPLACE_ITEM_BAD_SLOT;
    if(!make_item(item_storage,item_name,amount,data))return REPLACE_ITEM_FAILED;if(!command_item_components_apply(item_storage,components)){destroy_item(item_storage);return REPLACE_ITEM_FAILED;}
    block_entity=((BlockEntityFn)SEAM_BlockSource_getBlockEntity)(source,position);if(!is_container_block_entity(block_entity)){destroy_item(item_storage);return REPLACE_ITEM_NO_CONTAINER;}ok=container_set(*(void **)((u8 *)block_entity+ENTITY_BLOCK_COMPONENT_OFFSET),slot_id,item_storage,maximum_slot);destroy_item(item_storage);return ok?REPLACE_ITEM_OK:REPLACE_ITEM_BAD_SLOT;
}

static int replace_player_hotbar(void *entity,int slot_id,const void *item,int *maximum_slot){
    void *proxy,*container;int physical;if(maximum_slot)*maximum_slot=8;if(!command_entity_is_player(entity)||slot_id<0||slot_id>=9)return 0;proxy=supplies(entity);container=command_player_inventory_proxy_inventory(proxy);if(!container)return 0;physical=((FillingContainerLinkedSlotFn)SEAM_FillingContainer_getLinkedSlot)(container,slot_id);
    if(physical<0){physical=((FillingContainerNextEmptySlotFn)SEAM_FillingContainer_getNextEmptySlot)(container);if(physical<0||!((FillingContainerLinkSlotFn)SEAM_FillingContainer_linkSlot)(container,slot_id,physical))return 0;}
    if(!command_player_inventory_proxy_set_item(proxy,physical,item))return 0;notify_player_inventory_changed(entity);return 1;
}

static int replace_player_inventory(void *entity,int slot_id,const void *item,int *maximum_slot){void *proxy;if(maximum_slot)*maximum_slot=0x1a;if(!command_entity_is_player(entity)||slot_id<0||slot_id>=0x1b)return 0;proxy=supplies(entity);if(!command_player_inventory_proxy_set_item(proxy,slot_id+0x12,item))return 0;notify_player_inventory_changed(entity);return 1;}
static int item_is_named(const void *item_storage,const char *name){void *item;if(!item_storage||!name)return 0;item=item_from_arg(name);return item&&*(void **)((const u8 *)item_storage+0xc)==item;}
static int item_allows_offhand(const void *item_storage){void *item;if(!item_storage)return 0;item=*(void **)((const u8 *)item_storage+0xc);return item&&*(const u8 *)((const u8 *)item+SEAM_Item_allowOffhandOffset)!=0;}
static int item_fits_armor_slot(const void *item,int slot){return slot==2||(((ItemInstanceArmorFn)SEAM_ItemInstance_isArmor)(item)!=0&&((ItemInstanceArmorSlotFn)SEAM_ItemInstance_getSlotForItem)(item)==slot-2);}

static int replace_component_slot(void *entity,int requested_slot,int target_slot,const void *item,int *maximum_slot){
    void *component=entity_container_component(entity),*container=component_container(component);void **vtable;int size;if(maximum_slot)*maximum_slot=-1;if(!container||requested_slot<0||target_slot<0)return 0;vtable=*(void ***)container;if(!vtable||!vtable[0x28/4])return 0;size=((ContainerSizeFn)vtable[0x28/4])(container);if(maximum_slot)*maximum_slot=size-1;if(requested_slot>=size||target_slot>=size)return 0;return component_set_item(component,target_slot,item,maximum_slot);
}

ReplaceItemResult replace_item_entity(void *entity,const char *slot_type,int slot_id,const char *item_name,int amount,int data,const char *components,int *maximum_slot){
    int slot=equipment_slot(slot_type),ok=0;u32 item_storage[48];if(maximum_slot)*maximum_slot=-1;if(!entity||slot<0||!make_item(item_storage,item_name,amount,data))return REPLACE_ITEM_FAILED;if(slot_id<0||slot_id>32767)return REPLACE_ITEM_BAD_SLOT;if(!command_item_components_apply(item_storage,components)){destroy_item(item_storage);return REPLACE_ITEM_FAILED;}
    if(slot==0){if(command_entity_is_player(entity)){void *proxy=supplies(entity);int selected=proxy?*(int *)((u8 *)proxy+SEAM_PlayerInventoryProxy_selectedSlot_offset):-1;ok=replace_player_hotbar(entity,selected,item_storage,maximum_slot);}else if(command_entity_is_mob(entity)){((MobItemSlotFn)SEAM_Mob_setItemSlot)(entity,0,item_storage);ok=command_mob_sync_equipment(entity,item_storage,0,0,0);}}
    else if(slot==1){if(command_entity_is_type(entity,ENTITY_TYPE_MOB)&&item_allows_offhand(item_storage)){((MobOffhandItemFn)SEAM_Mob_setOffhandItem)(entity,item_storage);ok=command_mob_sync_equipment(entity,item_storage,1,0,0x77);}}
    else if(slot>=2&&slot<=5){if(command_entity_is_mob(entity)&&item_fits_armor_slot(item_storage,slot)){((MobItemSlotFn)SEAM_Mob_setItemSlot)(entity,slot,item_storage);ok=command_mob_sync_armor(entity);}}
    else if(slot==6)ok=replace_player_hotbar(entity,slot_id,item_storage,maximum_slot);else if(slot==7)ok=replace_player_inventory(entity,slot_id,item_storage,maximum_slot);
    else if(slot==8){if(command_entity_is_player(entity)){void *container=*(void **)((u8 *)entity+SEAM_Player_enderChestContainerOffset);ok=container_set(container,slot_id,item_storage,maximum_slot);}}
    else if(slot==9){if(command_entity_is_type(entity,ENTITY_TYPE_HORSE_FAMILY)&&item_is_named(item_storage,"saddle"))ok=replace_component_slot(entity,slot_id,0,item_storage,maximum_slot);}
    else if(slot==10){if(command_entity_is_type(entity,ENTITY_TYPE_HORSE)&&((ItemInstanceHorseArmorFn)SEAM_ItemInstance_isHorseArmorItem)(item_storage))ok=replace_component_slot(entity,slot_id,1,item_storage,maximum_slot);}
    else if(slot==11){if(command_entity_is_type(entity,ENTITY_TYPE_DONKEY)||command_entity_is_type(entity,ENTITY_TYPE_MULE))ok=replace_component_slot(entity,slot_id,slot_id+1,item_storage,maximum_slot);}
    destroy_item(item_storage);return ok?REPLACE_ITEM_OK:(maximum_slot&&*maximum_slot>=0?REPLACE_ITEM_BAD_SLOT:REPLACE_ITEM_FAILED);
}
