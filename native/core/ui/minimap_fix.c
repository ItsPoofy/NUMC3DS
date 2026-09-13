#include "minimap_fix.h"
#include "../internal.h"
#include "../state.h"

typedef void *(*GetPlayerFn)(void *);
typedef void *(*GetSectorArrayFn)(void *);
typedef void  (*GetItemInstanceUuidFn)(u32 *, void *);
typedef void *(*GetMapSavedDataFn)(void *, int, u32, u32);
typedef void  (*UploadMapTextureFn)(void *);
#define SEAM_MinecraftGame_getPlayer        0x0025118Cu
#define SEAM_Player_getSectorArray          0x00606924u
#define SEAM_ItemInstance_getUuid           0x0062F3F4u
#define SEAM_Level_getMapSavedData          0x005C869Cu

static void on_upload_map_texture(void *screen)
{
    void *game;
    void *player;
    void *dimension;
    int dim_id;
    int sector;
    char *sectors;
    void *sector_item;
    u32 uuid[2];
    void *level;
    void *storage;
    void *map_data;
    void *pixels;

    if (!screen) return;
    game = *(void**)((char*)screen + 0x04);
    if (!game) return;

    player = ((GetPlayerFn)SEAM_MinecraftGame_getPlayer)(game);
    if (!player) return;

    /* Check dimension: only Overworld has the 32 sector grid */
    dimension = *(void**)((char*)player + 0x1a0);
    dim_id = dimension ? *(int*)((char*)dimension + 0x08) : 0;
    if (dim_id != 0) return;

    /* Check sector index bounds */
    sector = *(int*)((char*)screen + 0x1ec);
    if (sector < 0 || sector >= 32) return;

    /* Check sector ItemInstance */
    sectors = (char*)((GetSectorArrayFn)SEAM_Player_getSectorArray)(player);
    if (!sectors) return;
    sector_item = (void*)(sectors + sector * 48);
    if (!*(unsigned char*)sector_item) return; /* item count must be > 0 */

    /* Extract UUID */
    uuid[0] = 0; uuid[1] = 0;
    ((GetItemInstanceUuidFn)SEAM_ItemInstance_getUuid)(uuid, sector_item);
    if ((uuid[0] == 0 && uuid[1] == 0) || (uuid[0] == 0xFFFFFFFFu && uuid[1] == 0xFFFFFFFFu)) return;

    /* Query MapItemSavedData from Level storage */
    level = *(void**)((char*)player + 0x210);
    if (!level) return;
    storage = *(void**)((char*)level + 0x0c);
    if (!storage) return;

    map_data = ((GetMapSavedDataFn)SEAM_Level_getMapSavedData)(storage, 0, uuid[0], uuid[1]);
    if (!map_data) return;

    pixels = *(void**)((char*)map_data + 0x20);
    if (!pixels) return;

    /* Safe to execute authentic hardware upload */
    ((UploadMapTextureFn)s->minimap_upload_texture.trampoline)(screen);
}

int minimap_fix_install_hooks(void)
{
    /* Authentic engine uploader 0x00677574 runs unhindered */
    return 0;
}
