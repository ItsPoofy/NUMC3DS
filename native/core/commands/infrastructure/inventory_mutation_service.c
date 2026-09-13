#include "../inventory_mutation_service.h"
#include "../entity_classification.h"
#include "../../internal.h"

typedef int (*ContainerSizeFn)(void *);
typedef void *(*ContainerGetItemFn)(void *,int);
typedef int (*ItemInstanceIsNullFn)(const void *);
typedef int (*TargetItemInstanceIdFn)(void *);
typedef void *(*ProxyGetInventoryFn)(void *);
typedef int (*ContainerClearFn)(void *,int);
typedef void (*ContainerSubItemFn)(void *,int,unsigned);

static void *player_container(void *player){
    void *proxy;
    if(!command_entity_is_player(player))return 0;
    proxy=supplies(player);
    return proxy?((ProxyGetInventoryFn)SEAM_PlayerInventoryProxy_getInventory)(proxy):0;
}

static int container_size(void *container){
    void **vtable=container?*(void ***)container:0;
    return vtable&&vtable[0x28/4]?((ContainerSizeFn)vtable[0x28/4])(container):0;
}

static void *container_item(void *container,int slot){
    void **vtable=container?*(void ***)container:0;
    return vtable&&vtable[0x10/4]?((ContainerGetItemFn)vtable[0x10/4])(container,slot):0;
}

static int item_matches(const void *item,const void *requested){
    if(!item||((ItemInstanceIsNullFn)SEAM_ItemInstance_isNull)(item))return 0;
    if(((TargetItemInstanceIdFn)SEAM_ItemInstance_getId)((void *)item)!=((TargetItemInstanceIdFn)SEAM_ItemInstance_getId)((void *)requested))return 0;
    return *(const unsigned short *)((const u8 *)requested+2)==0x7fffu||
        *(const unsigned short *)((const u8 *)item+2)==*(const unsigned short *)((const u8 *)requested+2);
}

int command_inventory_clear_all(void *player){
    void *container=player_container(player);void **vtable;int removed;
    if(!container)return -1;
    removed=((ContainerClearFn)SEAM_FillingContainer_clearInventory)(container,-1);
    vtable=*(void ***)player;
    if(vtable&&vtable[0x3fc/4])((void (*)(void *))vtable[0x3fc/4])(player);
    return removed;
}

int command_inventory_count_resource(void *player,void *item){
    void *container=player_container(player);u32 requested[48];int slot,size,total=0;
    if(!item)return -1;
    if(!container)return -1;
    zero(requested,sizeof(requested));
    ((ItemCountAuxCtorFn)SEAM_ItemInstance_itemCountAuxCtor)(requested,item,1,0);
    size=container_size(container);
    for(slot=0;slot<size;slot++){
        void *slot_item=container_item(container,slot);
        if(item_matches(slot_item,requested))total+=*(const unsigned char *)slot_item;
    }
    ((ItemInstanceDtorFn)SEAM_ItemInstance_dtor)(requested);
    return total;
}

int command_inventory_remove_resource(void *player,void *item,int data,int maximum){
    void *container=player_container(player);u32 requested[48];int slot,size,remaining;
    if(!item)return -1;
    if(!container)return -1;
    zero(requested,sizeof(requested));
    ((ItemCountAuxCtorFn)SEAM_ItemInstance_itemCountAuxCtor)(requested,item,maximum>0?maximum:1,data<0?0x7fff:data);
    remaining=maximum>0?maximum:*(const unsigned char *)requested;
    size=container_size(container);
    while(remaining>0){
        int found=-1;
        for(slot=0;slot<size;slot++){
            void *slot_item=container_item(container,slot);
            if(item_matches(slot_item,requested)){found=slot;break;}
        }
        if(found<0)break;
        {
            void *slot_item=container_item(container,found);
            unsigned count=*(const unsigned char *)slot_item;
            unsigned take=(unsigned)remaining<count?(unsigned)remaining:count;
            ((ContainerSubItemFn)SEAM_FillingContainer_subItem)(container,found,take);
            remaining-=(int)take;
        }
    }
    ((ItemInstanceDtorFn)SEAM_ItemInstance_dtor)(requested);
    return remaining;
}

int command_inventory_clear_player(void *player,void *item,int data,int max_count){
    if(!item)return command_inventory_clear_all(player);
    if(max_count==0)return command_inventory_count_resource(player,item);
    {
        int requested_count=max_count>0?max_count:1;
        int remaining=command_inventory_remove_resource(player,item,data,max_count);
        return remaining<0?remaining:requested_count-remaining;
    }
}

int command_inventory_clear_players(void **players,unsigned count,void *item,int data,int max_count,int *removed_total){
    unsigned index;int total=0,any_player=0;
    if(removed_total)*removed_total=0;
    if(!players||!count)return -1;
    for(index=0;index<count;index++){
        int removed;
        if(!players[index])continue;
        any_player=1;
        removed=command_inventory_clear_player(players[index],item,data,max_count);
        if(removed>0)total+=removed;
    }
    if(!any_player)return -1;
    if(removed_total)*removed_total=total;
    return total;
}
