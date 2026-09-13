#include "world_thumbnail_ui.h"
#include "ui_sprite.h"
#include "../world/world_thumbnail.h"
#include "../world_transfer/world_transfer_fs.h"
#include "../util/string_util.h"
#include "../state.h"
#include "../seams.h"

enum { WT_THUMBNAIL_SLOTS = 6 };

typedef struct {
    char world_id[32];
    u32 resource[5];
    void *texture;
    void *sprite;
    void *group;
    u32 last_used;
    int loaded;
} WtThumbnailSlot;

static WtThumbnailSlot s_slots[WT_THUMBNAIL_SLOTS];
static u32 s_tick;
static NuMC3DS_Hook s_render_hook;

static int thumbnail_world_id(void *card, char *out, unsigned cap){
    u8 *screen, *begin, *end;
    const char *id;
    unsigned index, length = 0;
    int button_id;
    void *owner = *(void**)((u8*)card + 0x84);
    if(!owner) return 0;
    screen = (u8*)owner - 0x0c;
    if(*(u32*)screen != 0x009B8680u) return 0;
    button_id = *(int*)((u8*)card + 0x78);
    if(button_id < 1) return 0;
    begin = *(u8**)(screen + 0x114);
    end = *(u8**)(screen + 0x118);
    index = (unsigned)(button_id - 1);
    if(!begin || (u32)end < (u32)begin || ((u32)end - (u32)begin) % 0xa0u ||
       index >= ((u32)end - (u32)begin) / 0xa0u) return 0;
    id = (const char*)begin + index * 0xa0u + 0x20;
    while(length < 0x1eu && id[length]) length++;
    if(!length || length >= 0x1eu || length + 1 > cap) return 0;
    cp(out, id, length);
    out[length] = 0;
    return wt_fs_validate_relative(out);
}

void wt_thumbnail_ui_invalidate(const char *world_id){
    int i;
    for(i = 0; i < WT_THUMBNAIL_SLOTS; i++){
        if(!world_id || streq(s_slots[i].world_id, world_id)){
            s_slots[i].world_id[0] = 0;
            s_slots[i].loaded = 0;
        }
    }
}

static void *thumbnail_sprite(void *game, const char *world_id){
    void *group;
    WtThumbnailSlot *slot = 0;
    int i;
    if(!game || !(group = *(void**)((u8*)game + 0x58))) return 0;
    s_tick++;
    for(i = 0; i < WT_THUMBNAIL_SLOTS; i++){
        WtThumbnailSlot *entry = &s_slots[i];
        if(entry->group && entry->group != group) zero(entry, sizeof(*entry));
        if(streq(entry->world_id, world_id)){
            entry->last_used = s_tick;
            return entry->loaded ? entry->sprite : 0;
        }
        if(!slot || entry->last_used < slot->last_used) slot = entry;
    }
    slot->last_used = s_tick;
    slot->group = group;
    slot->loaded = 0;
    wt_fs_copy_text(slot->world_id, sizeof(slot->world_id), world_id);
    if(!wt_thumbnail_is_cached_world(world_id) && !wt_thumbnail_exists(world_id)){
        return 0;
    }
    if(!slot->texture){
        char name[32];
        unsigned length = 0;
        append(name, &length, "numc3ds/wt_");
        append_int(name, &length, (int)(slot - s_slots));
        ((ResourceLocationCtor)SEAM_ResourceLocation_ctor)(slot->resource, name, 1);
        slot->texture = ((void*(*)(void*,void*))0x004F3F88u)(group, slot->resource);
        if(!slot->texture)
            slot->texture = ((void*(*)(void*,void*,int,int))0x004F4038u)(group, slot->resource, WT_THUMBNAIL_TEX_W, WT_THUMBNAIL_TEX_H);
    }
    if(!slot->texture || !wt_thumbnail_load_into_texture(slot->texture, world_id)){
        return 0;
    }
    if(!slot->sprite){
        void *memory = ((GameAllocWithSelector)SEAM_Heap_allocWithSelector)(0xa8, (void*)SEAM_game_alloc_selector);
        if(memory)
            slot->sprite = ((SpriteCtorFn)SEAM_Sprite_ctor)(memory, game, 0, 0, 45, 25, slot->resource, 0, 0, WT_THUMBNAIL_PIC_W, WT_THUMBNAIL_PIC_H);
        if(!slot->sprite){
            if(memory) ((void(*)(void*))SEAM_operator_delete)(memory);
            return 0;
        }
    }
    slot->loaded = 1;
    return slot->sprite;
}

static void on_world_card_render(void *card){
    char world_id[32];
    void *original = *(void**)((u8*)card + 0xa4);
    void *replacement = 0;
    if(original && thumbnail_world_id(card, world_id, sizeof(world_id)))
        replacement = thumbnail_sprite(*(void**)((u8*)card + 0x74), world_id);
    if(replacement) *(void**)((u8*)card + 0xa4) = replacement;
    ((void(*)(void*))s_render_hook.trampoline)(card);
    if(replacement) *(void**)((u8*)card + 0xa4) = original;
}

int wt_thumbnail_ui_install(void){
    s_render_hook.target = 0x005D7020u;
    s_render_hook.replacement = (u32)on_world_card_render;
    s_render_hook.expected[0] = 0xE92D41F0u;
    s_render_hook.expected[1] = 0xE1A04000u;
    return s->host.install_hook(&s_render_hook);
}

void wt_thumbnail_ui_remove(void){
    int i;
    if(s_render_hook.trampoline) s->host.remove_hook(&s_render_hook);
    zero(&s_render_hook, sizeof(s_render_hook));
    for(i = 0; i < WT_THUMBNAIL_SLOTS; i++){
        if(s_slots[i].sprite) ui_sprite_destroy(s_slots[i].sprite);
        if(s_slots[i].resource[0]) drop_str(s_slots[i].resource);
    }
    zero(s_slots, sizeof(s_slots));
    s_tick = 0;
}
