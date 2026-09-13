#include "item_registry_service.h"
#include "../util/string_util.h"
#include "../util/math_util.h"
#include "../rt.h"


void* map_find_int(void* map, numc3ds_u32 key) {
    void *header, *node;
    if (!map) return 0;
    header = *(void**)((unsigned char*)map + 0x10);
    node = header ? *(void**)((unsigned char*)header + 4) : 0;
    while (node) {
        numc3ds_u32 k = *(numc3ds_u32*)((unsigned char*)node + 0x10);
        if (key < k) node = *(void**)((unsigned char*)node + 8);
        else if (key > k) node = *(void**)((unsigned char*)node + 0xc);
        else return *(void**)((unsigned char*)node + 0x14);
    }
    return 0;
}

void* item_by_id(int id) {
    if (id < 0 || id > 511) return 0;
    return ((void**)0x00B0CEF0u)[id];
}

void* block_by_name(const char* name) {
    numc3ds_u32 h = 0;
    void *block;
    make_str(&h, name);
    block = ((BlockLookupFn)0x005BD34Cu)(&h, 1);
    drop_str(&h);
    return block;
}

void* item_by_name(const char* name) {
    const char *s = strip_ns(name);
    void *item;
    if (streq(s, "bedrock")) {
        return item_by_id(7);
    }
    if (streq(s, "empty_map") || streq(s, "map_empty")) {
        return item_by_id(395);
    }
    item = map_find_int((void*)0x00B0D728u, ((HashCodeFn)0x0012A814u)(s));
    if (item) {
        return item;
    }
    {
        void *block = block_by_name(name);
        if (block) {
            unsigned char block_id = *(unsigned char*)((char*)block + 4);
            return item_by_id(block_id);
        }
    }
    return 0;
}

void* item_from_arg(const char* name) {
    void* item = item_by_name(name);
    if (!item) {
        const char* q = name;
        int id;
        if (integer(&q, &id) && !*q) {
            if (id == 358) return 0;
            item = item_by_id(id);
        }
    }
    return item;
}

void* supplies(void* player) {
    return ((GetSuppliesFn)0x00726090u)(player);
}

void* inventory_of(void* player) {
    void* proxy = supplies(player);
    return proxy ? ((GetInventoryFn)0x003170DCu)(proxy) : 0;
}
