#include "../inventory_action_service.h"
#include "../../internal.h"

typedef void (*PacketFactoryFn)(void **,unsigned);
typedef void (*ItemInstanceCopyAssignFn)(void *,const void *);
typedef void (*TargetedPacketSendFn)(void *,const void *,const void *);
typedef void (*NetworkIdentifierCopyFn)(void *,void *);

enum {
    INVENTORY_ACTION_PACKET_ID=0x2f,
    INVENTORY_ACTION_PACKET_ACTION_OFFSET=0x38,
    INVENTORY_ACTION_PACKET_ITEM_OFFSET=0x08,
    NETWORK_IDENTIFIER_SIZE=0x20
};

int command_inventory_action_give(void *level,void *player,const void *item_instance){
    void *packet=0,*sender;
    u8 network_identifier[NETWORK_IDENTIFIER_SIZE];
    if(!level||!player||!item_instance)return 0;
    sender=*(void **)((u8 *)level+SEAM_Level_packetSenderOffset);
    if(!sender)return 0;
    ((PacketFactoryFn)SEAM_MinecraftPackets_createPacket)(&packet,INVENTORY_ACTION_PACKET_ID);
    if(!packet)return 0;
    ((ItemInstanceCopyAssignFn)SEAM_ItemInstance_copyAssign)((u8 *)packet+INVENTORY_ACTION_PACKET_ITEM_OFFSET,item_instance);
    *(u8 *)((u8 *)packet+INVENTORY_ACTION_PACKET_ACTION_OFFSET)=0;
    ((NetworkIdentifierCopyFn)SEAM_NetworkIdentifier_copyFromEntity)(network_identifier,player);
    ((TargetedPacketSendFn)SEAM_LoopbackPacketSender_sendTo)(sender,network_identifier,packet);
    ((PacketDtor)SEAM_InventoryActionPacket_dtor)(packet);
    return 1;
}

int command_inventory_action_enchant(void *level,void *player,const void *item_instance,int enchantment_id,int enchantment_level){
    void *packet=0,*sender;
    u8 network_identifier[NETWORK_IDENTIFIER_SIZE];
    int pair[2];
    if(!level||!player||!item_instance)return 0;
    sender=*(void **)((u8 *)level+SEAM_Level_packetSenderOffset);
    if(!sender)return 0;
    ((PacketFactoryFn)SEAM_MinecraftPackets_createPacket)(&packet,INVENTORY_ACTION_PACKET_ID);
    if(!packet)return 0;
    ((ItemInstanceCopyAssignFn)SEAM_ItemInstance_copyAssign)((u8 *)packet+INVENTORY_ACTION_PACKET_ITEM_OFFSET,item_instance);
    pair[0]=enchantment_id;
    pair[1]=enchantment_level;
    if(!((int (*)(void *,const int *))SEAM_EnchantUtils_applyEnchant)((u8 *)packet+INVENTORY_ACTION_PACKET_ITEM_OFFSET,pair)){
        ((PacketDtor)SEAM_InventoryActionPacket_dtor)(packet);
        return 0;
    }
    ((ItemInstanceCopyAssignFn)SEAM_ItemInstance_copyAssign)((u8 *)packet+INVENTORY_ACTION_PACKET_ITEM_OFFSET,item_instance);
    *(u8 *)((u8 *)packet+INVENTORY_ACTION_PACKET_ACTION_OFFSET)=2;
    *(int *)((u8 *)packet+0x3c)=enchantment_id;
    *(int *)((u8 *)packet+0x40)=enchantment_level;
    ((NetworkIdentifierCopyFn)SEAM_NetworkIdentifier_copyFromEntity)(network_identifier,player);
    ((TargetedPacketSendFn)SEAM_LoopbackPacketSender_sendTo)(sender,network_identifier,packet);
    ((PacketDtor)SEAM_InventoryActionPacket_dtor)(packet);
    return 1;
}
