#include "../mob_equipment_sync_service.h"
#include "../../internal.h"

typedef void (*PacketFactoryFn)(void **,unsigned);
typedef const u32 *(*EntityUniqueIdFn)(void *);
typedef void (*ItemInstanceCopyAssignFn)(void *,const void *);
typedef void (*PacketSendFn)(void *,void *);

enum {
    MOB_EQUIPMENT_PACKET_ID=0x1f,
    MOB_EQUIPMENT_ITEM_OFFSET=0x10,
    MOB_EQUIPMENT_SLOT_OFFSET=0x40,
    MOB_EQUIPMENT_SELECTED_SLOT_OFFSET=0x44,
    MOB_EQUIPMENT_FORCE_NULL_OFFSET=0x48,
    MOB_EQUIPMENT_CONTAINER_OFFSET=0x49,
    MOB_EQUIPMENT_WIRE_SLOT_OFFSET=0x4a,
    MOB_EQUIPMENT_WIRE_SELECTED_SLOT_OFFSET=0x4b,
    MOB_EQUIPMENT_WIRE_CONTAINER_OFFSET=0x4c
};

int command_mob_sync_equipment(void *mob,const void *item,int slot,int selected_slot,unsigned container_id){
    void *packet=0,*level,*sender;const u32 *unique_id;
    if(!mob||!item||slot<0||selected_slot<0||container_id>0xffu)return 0;
    level=((void *(*)(void *))SEAM_Entity_getLevel)(mob);
    sender=level?*(void **)((u8 *)level+SEAM_Level_packetSenderOffset):0;
    unique_id=((EntityUniqueIdFn)SEAM_Entity_getUniqueID)(mob);
    if(!sender||!unique_id)return 0;
    ((PacketFactoryFn)SEAM_MinecraftPackets_createPacket)(&packet,MOB_EQUIPMENT_PACKET_ID);
    if(!packet)return 0;
    *(u32 *)((u8 *)packet+0x08)=unique_id[0];
    *(u32 *)((u8 *)packet+0x0c)=unique_id[1];
    ((ItemInstanceCopyAssignFn)SEAM_ItemInstance_copyAssign)((u8 *)packet+MOB_EQUIPMENT_ITEM_OFFSET,item);
    *(int *)((u8 *)packet+MOB_EQUIPMENT_SLOT_OFFSET)=slot;
    *(int *)((u8 *)packet+MOB_EQUIPMENT_SELECTED_SLOT_OFFSET)=selected_slot;
    *((u8 *)packet+MOB_EQUIPMENT_FORCE_NULL_OFFSET)=0;
    *((u8 *)packet+MOB_EQUIPMENT_CONTAINER_OFFSET)=(u8)container_id;
    *((u8 *)packet+MOB_EQUIPMENT_WIRE_SLOT_OFFSET)=(u8)slot;
    *((u8 *)packet+MOB_EQUIPMENT_WIRE_SELECTED_SLOT_OFFSET)=(u8)selected_slot;
    *((u8 *)packet+MOB_EQUIPMENT_WIRE_CONTAINER_OFFSET)=(u8)container_id;
    ((PacketSendFn)SEAM_LoopbackSend)(sender,packet);
    ((PacketDtor)SEAM_MobEquipmentPacket_dtor)(packet);
    return 1;
}

int command_mob_sync_armor(void *mob){
    if(!mob)return 0;
    ((void (*)(void *))SEAM_Mob_sendInventory)(mob);
    return 1;
}
