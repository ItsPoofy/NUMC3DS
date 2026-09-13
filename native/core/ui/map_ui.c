#include "map_ui.h"
#include "map_preview_texture.h"
#include "../util/string_util.h"
#include "../item/map_item.h"
#include "../rt.h"
#include "../state.h"

/* AAPCS-VFP signature for InGamePlayScreen::renderBottomScreen */
typedef void (__attribute__((pcs("aapcs-vfp"))) *RenderBottomScreenFn)(void*, int, int, float);

typedef void* (*PlayerGetPlayerFn)(void*);
typedef void* (*PlayerGetSelectedItemFn)(void*);
typedef int   (*ItemInstanceGetIdFn)(void*);
typedef void* (*PlayerGetSectorArrayFn)(void*);

#define SEAM_Player_getPlayer            0x0025118Cu
#define SEAM_Player_getSelectedItem      0x00726D50u
#define SEAM_ItemInstance_getId          0x006B2924u
#define SEAM_Player_getSectorArray       0x00606924u

static int held_map_active;

static void *map_player_level(void *player) {
    return map_player_server_level(player);
}

static void __attribute__((pcs("aapcs-vfp"))) on_render_bottom_screen(void *screen, int x, int y, float tick) {
    void *mc;
    void *player;
    void *held;
    int held_id;

    if (!screen) return;

    mc = *(void**)((char*)screen + 4);
    player = mc ? ((PlayerGetPlayerFn)SEAM_Player_getPlayer)(mc) : 0;
    held = player ? ((PlayerGetSelectedItemFn)SEAM_Player_getSelectedItem)(player) : 0;
    if ((u32)held < 0x00100000u) held = 0;
    held_id = held ? ((ItemInstanceGetIdFn)SEAM_ItemInstance_getId)(held) : 0;
    if (held_id < 0) held_id = 0;

    if(held_map_active&&held_id!=395&&held_id!=358){
        map_preview_texture_reset();
        ((void(*)(void*))0x00677574u)(screen);
        held_map_active=0;
    }
    if(held_id==395||held_id==358)held_map_active=1;

    if (held_id == 395 && player) {
        /* Holding Empty Map / Empty Locator Map: blank parchment */
        void *sectorBase = ((PlayerGetSectorArrayFn)SEAM_Player_getSectorArray)(player);
        if (sectorBase) {
            int sector = *(int *)((char *)screen + 0x1ec);
            if (sector < 0 || sector >= 32) sector = 0;
            void *sectorItem = (char*)sectorBase + sector * 48;
            void **pSprite = (void**)((char*)screen + 0xd8 + sector * 4);
            void *origSprite = pSprite ? *pSprite : 0;
            if (origSprite) {
                MapPreviewTexture binding;
                char saved[48];
                char blank[48];
                zero(blank, sizeof(blank));
                cp(saved, sectorItem, 48);
                cp(sectorItem, blank, 48);
                map_preview_texture_begin(&binding,mc,origSprite,0);
                ((RenderBottomScreenFn)s->render_bottom_screen.trampoline)(screen, x, y, tick);
                map_preview_texture_end(&binding);
                cp(sectorItem, saved, 48);
                return;
            }
        }
    }

    if (held_id == 358 && player) {
        /* Holding Filled Map: display held map's pixels and tracking markers on bottom screen */
        void *level = map_player_level(player);
        void *sectorBase = ((PlayerGetSectorArrayFn)SEAM_Player_getSectorArray)(player);
        if (held && level && sectorBase) {
            int sector = *(int *)((char *)screen + 0x1ec);
            if (sector < 0 || sector >= 32) sector = 0;
            void *sectorItem = (char*)sectorBase + sector * 48;
            void *heldData = map_savedata_for(level, held);
            void **pSprite = (void**)((char*)screen + 0xd8 + sector * 4);
            void *origSprite = pSprite ? *pSprite : 0;

            if (heldData && origSprite) {
                MapPreviewTexture binding;
                char saved[48];

                int is_locator = is_locator_map(held);

                char *dec_begin = *(char**)((char*)heldData + 0x3c);
                char *dec_end = *(char**)((char*)heldData + 0x40);
                void *orig_dec_end = dec_end;

                map_preview_texture_begin(&binding,mc,origSprite,heldData);

                if (!is_locator) {
                    /* Regular Map: Bedrock rule - NO player cursor / locator marker */
                    *(void**)((char*)heldData + 0x40) = dec_begin;
                }

                cp(saved, sectorItem, 48);
                cp(sectorItem, held, 48);
                ((RenderBottomScreenFn)s->render_bottom_screen.trampoline)(screen, x, y, tick);
                map_preview_texture_end(&binding);
                cp(sectorItem, saved, 48);

                *(void**)((char*)heldData + 0x40) = orig_dec_end;

                return;
            }

            if (origSprite) {
                /* Map data not yet available: display blank parchment */
                MapPreviewTexture binding;
                char saved[48];
                char blank[48];
                zero(blank, sizeof(blank));
                cp(saved, sectorItem, 48);
                cp(sectorItem, blank, 48);
                map_preview_texture_begin(&binding,mc,origSprite,0);
                ((RenderBottomScreenFn)s->render_bottom_screen.trampoline)(screen, x, y, tick);
                map_preview_texture_end(&binding);
                cp(sectorItem, saved, 48);
                return;
            }
        }
    }

    /* Normal gameplay: render the authentic bottom screen minimap */
    ((RenderBottomScreenFn)s->render_bottom_screen.trampoline)(screen, x, y, tick);
}

int map_ui_install_hooks(void) {
    NuMC3DS_Hook *hook = &s->render_bottom_screen;
    zero(hook, sizeof(*hook));
    hook->target = 0x006762BCu; /* InGamePlayScreen::renderBottomScreen */
    hook->expected[0] = 0xE92D4FF0u;
    hook->expected[1] = 0xE1A05000u;
    hook->replacement = (numc3ds_u32)on_render_bottom_screen;
    return s->host.install_hook(hook);
}
