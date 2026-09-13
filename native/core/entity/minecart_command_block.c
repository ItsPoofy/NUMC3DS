#include "minecart_command_block.h"
#include "../ui/command_block_screen.h"
#include "../world/command_block/command_block_model.h"
#include "../internal.h"
#include "../seams.h"
#include "../state.h"

typedef int  (*EntityGetInteractionFn)(void*, void*, void*);
typedef int  (*PlayerInteractFn)(void*, void*);
typedef void (*MinecartTickFn)(void*);

#define MINECART_CB_VTABLE_PRIMARY       0x009BECD8u
#define MINECART_CB_VTABLE_SECONDARY     0x009BEF80u
#define SEAM_Entity_isAlive              0x00724EC8u

int is_minecart_command_block(void *entity) {
    void *vtable;
    if (!entity) return 0;
    vtable = *(void**)entity;
    return vtable == (void*)MINECART_CB_VTABLE_PRIMARY ||
           vtable == (void*)MINECART_CB_VTABLE_SECONDARY;
}

void *minecart_command_block_get_or_create_component(void *entity) {
    void *comp;
    if (!is_minecart_command_block(entity)) return 0;

    comp = *(void**)((char*)entity + 0xdc4);
    if (!comp) {
        GameAllocWithSelector alloc = (GameAllocWithSelector)SEAM_Heap_allocWithSelector;
        comp = alloc(0x40, (void*)SEAM_game_alloc_selector);
        if (!comp) return 0;
        zero(comp, 0x40);

        /* CommandBlockComponent::CommandBlockComponent(comp, entity) */
        ((void(*)(void*, void*))0x00409DE8u)(comp, entity);

        /* Initialize runtime ticking defaults */
        *(int*)((char*)comp + 48) = 0; /* tick_delay */
        *(int*)((char*)comp + 52) = 1;
        *(u8*)((char*)comp + 56) = 1;  /* should_tick */

        *(void**)((char*)entity + 0xdc4) = comp;
    }
    return comp;
}

void minecart_command_block_cleanup(void *entity) {
    void *comp, *base;
    if (!is_minecart_command_block(entity)) return;

    comp = *(void**)((char*)entity + 0xdc4);
    if (comp) {
        base = (char*)comp + 8;
        ((StrDtor)SEAM_StrDtor)((char*)base + 8);  /* last_output */
        ((StrDtor)SEAM_StrDtor)((char*)base + 12); /* command */
        ((StrDtor)SEAM_StrDtor)((char*)base + 16); /* name */
        ((void(*)(void*))SEAM_operator_delete)(comp);
        *(void**)((char*)entity + 0xdc4) = 0;
    }
}

static int on_entity_get_interaction(void *entity, void *player, void *interaction) {
    if (is_minecart_command_block(entity)) {
        if (command_block_can_use(player)) {
            return 1;
        }
        return 0;
    }
    return ((EntityGetInteractionFn)s->entity_get_interaction.trampoline)(entity, player, interaction);
}

static int on_player_interact(void *player, void *target) {
    if (is_minecart_command_block(target)) {
        if (command_block_can_use(player)) {
            minecart_command_block_get_or_create_component(target);
            command_block_screen_open_entity(player, target);
            return 1;
        }
        return 0;
    }
    return ((PlayerInteractFn)s->player_interact.trampoline)(player, target);
}

static void on_minecart_normal_tick(void *entity) {
    void *comp, *source, *base;
    u8 should_tick;
    int *tick_delay;
    char origin[0x60];

    ((MinecartTickFn)s->minecart_cb_tick.trampoline)(entity);

    if (!is_minecart_command_block(entity)) return;

    /* Verify entity is alive and not marked removed before executing commands */
    if (!((int(*)(void*))SEAM_Entity_isAlive)(entity)) return;
    if (*(u8*)((char*)entity + 0xd4b)) return;

    comp = minecart_command_block_get_or_create_component(entity);
    if (!comp) return;

    should_tick = *(u8*)((char*)comp + 56);
    if (!should_tick) return;

    tick_delay = (int*)((char*)comp + 48);
    if (*tick_delay > 0) {
        (*tick_delay)--;
        return;
    }

    source = *(void**)((char*)entity + 0x210);
    if (source) {
        base = (char*)comp + 8;
        zero(origin, sizeof(origin));
        /* MinecartBlockCommandOrigin::MinecartBlockCommandOrigin(origin, source, &entity->uniqueId) */
        ((void(*)(void*, void*, const void*))0x00491C44u)(origin, source, (char*)entity + 0x48);
        /* BaseCommandBlock::_performCommand(base, source, origin) */
        ((void(*)(void*, void*, void*))0x002E9BE0u)(base, source, origin);
        /* MinecartBlockCommandOrigin::~MinecartBlockCommandOrigin(origin) */
        ((void(*)(void*))0x00491C98u)(origin);
    }
    *tick_delay = 4;
}

int minecart_command_block_install_hooks(void) {
    NuMC3DS_Hook *hook;
    int res;

    hook = &s->entity_get_interaction;
    zero(hook, sizeof(*hook));
    hook->target = 0x005EB198u; /* Entity::getInteraction */
    hook->expected[0] = 0xE92D43F0u;
    hook->expected[1] = 0xE1A04000u;
    hook->replacement = (numc3ds_u32)on_entity_get_interaction;
    res = s->host.install_hook(hook);
    if (res != 0) return res;

    hook = &s->player_interact;
    zero(hook, sizeof(*hook));
    hook->target = 0x00606CC0u; /* Player::interact */
    hook->expected[0] = 0xE92D4070u;
    hook->expected[1] = 0xE24DD050u;
    hook->replacement = (numc3ds_u32)on_player_interact;
    res = s->host.install_hook(hook);
    if (res != 0) return res;

    hook = &s->minecart_cb_tick;
    zero(hook, sizeof(*hook));
    hook->target = 0x00659750u; /* Minecart::normalTick */
    hook->expected[0] = 0xE92D4FF0u;
    hook->expected[1] = 0xE1A04000u;
    hook->replacement = (numc3ds_u32)on_minecart_normal_tick;
    res = s->host.install_hook(hook);
    if (res != 0) return res;

    return 0;
}
