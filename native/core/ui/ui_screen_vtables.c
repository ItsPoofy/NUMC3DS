#include "ui_screen_vtables.h"
#include "ui_runtime.h"
#include "../seams.h"
#include "../internal.h"

enum {
    SCREEN_PRIMARY_VTABLE_HEADER_BYTES=8,
    SCREEN_PRIMARY_VTABLE_BYTES=0x1a0,
    SCREEN_OWNER_VTABLE_HEADER_BYTES=8,
    SCREEN_OWNER_VTABLE_BYTES=0x98,
    SCREEN_VTABLE_BLOCK_BYTES=SCREEN_PRIMARY_VTABLE_HEADER_BYTES+SCREEN_PRIMARY_VTABLE_BYTES+SCREEN_OWNER_VTABLE_HEADER_BYTES+SCREEN_OWNER_VTABLE_BYTES
};

static void default_owner_press(void *owner, void *button) {
    void *screen = (u8*)owner - 0x0C;
    void **vtable = *(void***)screen;
    if (vtable && vtable[89]) ((void (*)(void*, void*))vtable[89])(screen, button);
}

static void default_noop(void *screen) {
    (void)screen;
}

static int default_on_back(void *screen, int reason) {
    (void)reason;
    ui_screen_close(screen);
    return 1;
}

static void default_mapped(void *screen, int button) {
    ((void (*)(void*, int))SEAM_Screen_handleMappedButton)(screen, button);
}

static void default_move(void *screen, int source, int direction) {
    (void)source;
    ui_screen_navigate(screen, direction);
}

static void default_next(void *screen) {
    ui_screen_navigate(screen, UI_NAV_RIGHT);
}

static void default_previous(void *screen) {
    ui_screen_navigate(screen, UI_NAV_LEFT);
}

void *ui_screen_vtables_create(void *screen, void *owner_press) {
    void *primary, *owner, *block, *primary_clone, *owner_clone;
    if (!screen || !owner_press) return 0;
    primary = *(void**)screen;
    owner = *(void**)((u8*)screen + 0x0c);
    if (!primary || !owner) return 0;
    block = ((GameAllocWithSelector)SEAM_Heap_allocWithSelector)(SCREEN_VTABLE_BLOCK_BYTES, (void*)SEAM_game_alloc_selector);
    if (!block) return 0;
    cp(block, (u8*)primary - SCREEN_PRIMARY_VTABLE_HEADER_BYTES, SCREEN_PRIMARY_VTABLE_HEADER_BYTES + SCREEN_PRIMARY_VTABLE_BYTES);
    primary_clone = (u8*)block + SCREEN_PRIMARY_VTABLE_HEADER_BYTES;
    cp((u8*)primary_clone + SCREEN_PRIMARY_VTABLE_BYTES, (u8*)owner - SCREEN_OWNER_VTABLE_HEADER_BYTES, SCREEN_OWNER_VTABLE_HEADER_BYTES + SCREEN_OWNER_VTABLE_BYTES);
    owner_clone = (u8*)primary_clone + SCREEN_PRIMARY_VTABLE_BYTES + SCREEN_OWNER_VTABLE_HEADER_BYTES;
    ((void**)owner_clone)[2] = owner_press;
    return primary_clone;
}

void ui_screen_vtables_apply(void *screen, void *vtables) {
    if (!screen || !vtables) return;
    *(void**)screen = vtables;
    *(void**)((u8*)screen + 0x0c) = (u8*)vtables + SCREEN_PRIMARY_VTABLE_BYTES + SCREEN_OWNER_VTABLE_HEADER_BYTES;
}

void ui_screen_close(void *screen) {
    void *game = ui_screen_game(screen);
    if (game) ((MinecraftGameSchedulePopScreenFn)SEAM_MinecraftGame_schedulePopScreen)(game, 1);
}

int ui_custom_screen_push(void *game, void *client, void **vtable_cache,
                          const UiCustomScreenHooks *hooks, UiShared *held) {
    void *memory, *screen;
    u32 *vtable;
    if (!game) {
        game = s->session_game ? s->session_game : s->host.minecraft_game;
        if (!game) return 0;
    }
    if (!client) {
        void **clients = *(void***)((u8*)game + 0xC0);
        client = clients ? *clients : 0;
    }
    if (held && held->object && held->control) {
        ((MinecraftGamePushScreenFn)SEAM_MinecraftGame_pushScreen)(game, (Shared*)held, 0);
        return 1;
    }
    if (!client || !vtable_cache || !hooks) return 0;

    memory = ((GameAllocWithSelector)SEAM_Heap_allocWithSelector)(0x90, (void*)SEAM_game_alloc_selector);
    if (!memory) return 0;

    screen = ((ScreenCtorFn)SEAM_Screen_ctor)(memory, game, client);
    if (!screen) {
        s->host.heap_free(memory);
        return 0;
    }

    if (!*vtable_cache) {
        void *owner_press = hooks->owner_press ? hooks->owner_press : default_owner_press;
        void *primary = ui_screen_vtables_create(screen, owner_press);
        if (!primary) {
            release_obj(screen);
            return 0;
        }
        vtable = (u32*)primary;
        vtable[9]   = hooks->closed ? (u32)hooks->closed : (u32)default_noop;
        vtable[30]  = hooks->on_back ? (u32)hooks->on_back : (u32)default_on_back;
        vtable[68]  = (u32)hooks->render;
        vtable[69]  = (u32)hooks->setup;
        vtable[82]  = hooks->mapped ? (u32)hooks->mapped : (u32)default_mapped;
        vtable[89]  = (u32)hooks->press;
        vtable[90]  = (u32)default_noop;
        vtable[99]  = vtable[100] = hooks->move ? (u32)hooks->move : (u32)default_move;
        vtable[101] = hooks->next ? (u32)hooks->next : (u32)default_next;
        vtable[102] = hooks->previous ? (u32)hooks->previous : (u32)default_previous;
        vtable[103] = (u32)default_noop;
        *vtable_cache = primary;
    }
    ui_screen_vtables_apply(screen, *vtable_cache);

    if (held) {
        if (!ui_shared_from_object(screen, held)) return 0;
        ((MinecraftGamePushScreenFn)SEAM_MinecraftGame_pushScreen)(game, (Shared*)held, 0);
    } else {
        UiShared temp = {0, 0};
        if (!ui_shared_from_object(screen, &temp)) return 0;
        ((MinecraftGamePushScreenFn)SEAM_MinecraftGame_pushScreen)(game, (Shared*)&temp, 0);
        ui_shared_release(&temp);
    }
    return 1;
}
