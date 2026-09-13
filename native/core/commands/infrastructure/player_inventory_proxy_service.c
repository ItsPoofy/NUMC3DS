#include "../player_inventory_proxy_service.h"
#include "../../internal.h"

typedef void *(*GetInventoryFn)(void *);
typedef int (*ContainerSizeFn)(void *);
typedef void (*ContainerSetItemFn)(void *,int,const void *);

void *command_player_inventory_proxy_inventory(void *proxy){
    return proxy?((GetInventoryFn)SEAM_PlayerInventoryProxy_getInventory)(proxy):0;
}

int command_player_inventory_proxy_set_item(void *proxy,int slot,const void *item){
    void *inventory;void **vtable;int size;
    if(!proxy||!item||slot<0)return 0;
    inventory=command_player_inventory_proxy_inventory(proxy);
    vtable=inventory?*(void ***)inventory:0;
    if(!vtable||!vtable[0x28/4]||!vtable[0x1c/4])return 0;
    size=((ContainerSizeFn)vtable[0x28/4])(inventory);
    if(slot>=size)return 0;
    ((ContainerSetItemFn)vtable[0x1c/4])(inventory,slot,item);
    return 1;
}
