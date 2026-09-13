#ifndef NUMC3DS_MAP_ITEM_H
#define NUMC3DS_MAP_ITEM_H

#include "../rt.h"

void register_map_items(void);
void map_log3(char tag, unsigned a, unsigned b);
void* map_savedata_for(void* level, void* item);
int is_locator_map(void* item);
void* map_upload_pixels(void* mc, void* mapData);
void map_copy_pixels_to_texture(void* tex, void* mapData);
void map_fill_blank_texture(void* tex);
void map_upload_held_pixels(void* mc, void* sectorData, void* heldData);
void map_item_add_creative_items(void);
void on_recipes_register_recipes(void* recipes);
void* map_player_server_level(void* player);
void* on_empty_map_use(void* item_ptr, void* held, void* player);
int map_item_install_hooks(void);

#endif /* NUMC3DS_MAP_ITEM_H */
