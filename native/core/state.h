#ifndef NUMC3DS_STATE_H
#define NUMC3DS_STATE_H
#include "rt.h"
#include "ui/keyboard_state.h"

typedef struct {
    numc3ds_u32 magic, size; NuMC3DS_HostAbi host;
    UiKeyboardState keyboard;
    NuMC3DS_Hook setup, pressed, render, gui_entry, system_message, chat_action, chat_session_end, button_ctor, keyboard_input_poll, commands_ctor, command_request, command_valid, command_name;
    NuMC3DS_Hook command_json, command_target_serialize, command_target_from_property, command_target_parse_property, command_target_resolve, command_overload, command_permission, command_parse, command_get, command_inner_overload, command_origin_permission, command_payload_validation;
    NuMC3DS_Hook ui_text, ui_centered_text, ui_centered_at_text, container_labels, options_categories, options_vector_push, options_slider_touch, options_render, option_item_name, option_item_render, options_pressed, warning_pressed, item_in_hand_render, top_hud_render;
    NuMC3DS_Hook world_create_setup, world_edit_setup, world_create_pressed, world_edit_pressed, create_world_start, warning_ctor, world_settings_render;
    void *screen, *chat, *pause;
    void *minecraft_commands, *command_parser, *command_game, *command_player, *command_level;
    void *quick_screen, *quick_control, *quick_vtable, *quick_sprites[13];
    void *quick_players[8], *quick_tp_source, *quick_tp_destination;
    void *world_screen, *world_gamerule_controls[WORLD_GAMERULE_CONTROLS], *session_game, *session_level;
    void *options_screen, *options_brightness_row, *options_fov_row, *options_hide_hand_row, *options_hide_hud_row, *option_item_rendering;
    numc3ds_u32 history_count, chat_page;
    numc3ds_u32 quick_active, quick_closing, quick_page, quick_player_count, quick_tp_selecting;
    numc3ds_u32 sent_count, sent_pos;
    void *intellisense_handler;
    numc3ds_u32 autocomplete_scroll;
    numc3ds_u32 world_kind, world_cheats, world_gamerule_count, force_commands, custom_warning, in_game_cheat_warning, hide_hand, hide_top_hud, edit_world_achievements_disabled;
    numc3ds_u32 options_building_categories, options_inserting_fov;
    char options_brightness_base[OPTIONS_LABEL_CAP], options_fov_base[OPTIONS_LABEL_CAP];
    char options_brightness_value[OPTIONS_LABEL_CAP], options_fov_value[OPTIONS_LABEL_CAP];
    char chat_input[MAX_TEXT+1], draft[MAX_TEXT+1], sent_history[HISTORY][MAX_TEXT+1];
    char quick_player_names[8][32];
    HistoryEntry history[HISTORY];
    UiCachedText history_lines[WRAPPED_LINES], history_page_cache, input_cache, title_cache, subtitle_cache;
    UiCachedText autocomplete_lines[AUTOCOMPLETE_VISIBLE_LINES];
    UiCachedText autocomplete_match_lines[AUTOCOMPLETE_VISIBLE_LINES];
    UiCachedText autocomplete_rest_lines[AUTOCOMPLETE_VISIBLE_LINES];
    unsigned char history_line_entry[WRAPPED_LINES];
    numc3ds_u32 history_generation, history_cache_generation, history_cache_count;
    void *history_cache_font;
    numc3ds_u32 exec_active;
    float exec_pos[3];
    void *exec_entity;
    void *exec_output_bag;
    u32 exec_result;
    numc3ds_u32 exec_native_callback;
    numc3ds_u32 exec_target_count, exec_target_explicit;
    void *exec_targets[EXEC_TARGET_CAPACITY];
    numc3ds_u32 title_ticks, title_stay;
    char title_text[64], subtitle_text[64];
    NuMC3DS_Hook progress_setup, progress_render, progress_dtor, panorama_render, saving_render, saving_dtor, resource_reload;
    NuMC3DS_Hook render_bottom_screen, recipes_register, creative_initialize, empty_map_use;
    NuMC3DS_Hook minimap_upload_texture, map_saved_data_ctor;
    NuMC3DS_Hook boat_control, fishing_hook_hit_check;
    NuMC3DS_Hook mob_get_armor_slot, container_inventory_screen_set_item_slot, container_inventory_screen_setup_navigation;
    NuMC3DS_Hook entity_get_interaction, player_interact, minecart_cb_tick;
    void *progress_screen, *saving_screen, *held_map_sprite;
    UiXpProgressBar progress_bar;
    UiProgressTips progress_tips;
    u32 resources_ready, resource_epoch;
    u32 progress_empty_splash;
    u8 progress_empty_splash_ready;
    u8 chat_command_active;
} State;

extern State *s;

#endif
