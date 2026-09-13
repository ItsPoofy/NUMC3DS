#include "world_settings_ui.h"
#include "ui_controls.h"
#include "ui_grid_row.h"
#include "ui_widgets.h"
#include "ui_text_style.h"
#include "ui_scrolling_pane.h"
#include "achievement_banner.h"
#include "../world_transfer/world_transfer_service.h"

static const char worldfail[]="NuMC3DS chat: world settings hook failed\n";
static const char createstartfail[]="NuMC3DS chat: create-world start hook failed\n";
static const char warningfail[]="NuMC3DS chat: achievement warning hook failed\n";

enum { WORLD_CHEATS_LABEL_ID=-1 };
static void on_world_cheats_pressed(void *screen,void *control);
static UiControlSpec world_control_specs[]={
    {
        .id = WORLD_CHEATS_LABEL_ID,
        .kind = UI_CTRL_LABEL,
        .label_text = "Cheats"
    },
    {
        .id = WORLD_CHEATS,
        .kind = UI_CTRL_SWITCH,
        .label_text = "Cheats",
        .on_press = on_world_cheats_pressed
    }
};
static UiControlSet world_controls={world_control_specs,sizeof(world_control_specs)/sizeof(world_control_specs[0])};

static void *screen_level(void*screen){void*game=ui_screen_game(screen),*clients,*client;if(!game)return 0;clients=*(void**)((u8*)game+0xC0);client=clients?*(void**)clients:0;return client?((GetLevel)SEAM_ClientInstance_getLevel)(client):0;}
static UiWorldSettingsScreenView *world_view(void*screen){return (UiWorldSettingsScreenView*)screen;}
static void *world_option_container(void*screen){UiShared*grid=screen?&world_view(screen)->option_grid:0;return grid&&grid->control?grid->object:0;}
static int get_screen_game_mode(void*screen){void**vtable;if(!screen)return 0;vtable=*(void***)screen;if(!vtable||!vtable[SEAM_WorldSettingsScreen_getGameModeVslot])return 0;return ((int(*)(void*))vtable[SEAM_WorldSettingsScreen_getGameModeVslot])(screen);}
static int world_screen_is_creative(void*screen){return screen&&get_screen_game_mode(screen)==1;}
static void *always_day_button(void*screen){UiShared*always=screen?&world_view(screen)->always_day:0;return always&&always->control?always->object:0;}
static int persisted_edit_world_cheats(void*screen,int*out){const char *world_name;u32 level_data[SEAM_LevelData_size/sizeof(u32)],world_name_obj=0,scratch=0;void*game,*client,*owner,*storage;void**vtable;if(!screen||!out)return 0;world_name=(const char *)((u8*)screen+SEAM_EditWorldScreen_worldNameOffset);game=ui_screen_game(screen);client=game?((void*(*)(void*))SEAM_MinecraftGame_getClientInstance)(game):0;owner=client?*(void**)((u8*)client+SEAM_ClientInstance_storageOwnerOffset):0;storage=owner?*(void**)((u8*)owner+SEAM_StorageOwner_levelStorageOffset):0;vtable=storage?*(void***)storage:0;if(!vtable||!vtable[5])return 0;((void*(*)(void*))SEAM_LevelData_ctor)(level_data);((StrCtor)SEAM_StrCtor)(&world_name_obj,world_name,&scratch);((void(*)(void*,void*,const void*))vtable[5])(level_data,storage,&world_name_obj);*out=*((u8*)level_data+SEAM_LevelData_commandsEnabledOffset)!=0;s->edit_world_achievements_disabled=((int(*)(void*))SEAM_LevelData_achievementsWillBeDisabledOnLoad)(level_data)!=0;((ThisFn)SEAM_LevelData_dtor)(level_data);((StrDtor)SEAM_StrDtor)(&world_name_obj);return 1;}
static void fix_world_type_button(void*screen){UiShared*wt_shared=screen?&world_view(screen)->world_type:0;UiButtonView*wt=(wt_shared&&wt_shared->control)?(UiButtonView*)wt_shared->object:0;if(!wt)return;wt->normal_label=ui_reference_text_color;wt->pressed_label=ui_reference_text_color;wt->inactive_label=ui_reference_text_color;wt->drop_shadow=0;}
static void fix_edit_buttons(void*screen){UiShared*play_shared=screen?(UiShared*)((u8*)screen+0x230):0,*del_shared=screen?(UiShared*)((u8*)screen+0x238):0;UiButtonView*play=(play_shared&&play_shared->control)?(UiButtonView*)play_shared->object:0,*del=(del_shared&&del_shared->control)?(UiButtonView*)del_shared->object:0;if(play)play->drop_shadow=0;if(del)del->drop_shadow=0;}
static void update_world_switches(void*screen){void*always=always_day_button(screen),*cheats=ui_controls_find(&world_controls,WORLD_CHEATS);int active=s->world_cheats!=0||world_screen_is_creative(screen);if(cheats)((UiSwitchButtonView*)cheats)->enabled=(u8)(s->world_cheats!=0);if(always)((UiElementView*)always)->active=(u8)(active!=0);}
static void update_achievement_banner_state(void*screen){int gm,eligible=0;if(!screen)return;gm=get_screen_game_mode(screen);if(s->world_kind==1){eligible=(s->world_cheats==0&&gm==0);}else if(s->world_kind==2){if(s->edit_world_achievements_disabled){eligible=0;}else{eligible=(s->world_cheats==0&&gm==0);}}achievement_banner_set_eligible(eligible);}
static void sync_world_cheats(void*screen){void*cheats=ui_controls_find(&world_controls,WORLD_CHEATS);if(cheats)s->world_cheats=((UiSwitchButtonView*)cheats)->enabled!=0;update_world_switches(screen);update_achievement_banner_state(screen);}
static void clear_world_gamerule_controls(void){s->world_gamerule_count=0;zero(s->world_gamerule_controls,sizeof(s->world_gamerule_controls));}
void world_settings_register_gamerule_control(void*control){unsigned i;if(!control)return;for(i=0;i<s->world_gamerule_count;i++)if(s->world_gamerule_controls[i]==control)return;if(s->world_gamerule_count<WORLD_GAMERULE_CONTROLS)s->world_gamerule_controls[s->world_gamerule_count++]=control;}
static int is_world_gamerule_control(void*control){unsigned i;for(i=0;i<s->world_gamerule_count;i++)if(s->world_gamerule_controls[i]==control)return 1;return 0;}
static void sync_after_world_setting(void*screen,void*control){(void)control;sync_world_cheats(screen);update_achievement_banner_state(screen);fix_world_type_button(screen);}

static void enable_daylight_cycle(void*screen){void*level=screen_level(screen),*rule;if(!level)return;rule=rule_dfs(rules_root((u8*)level+SEAM_GameRules_map_offset),"dodaylightcycle");if(rule)((RuleSetBoolFn)SEAM_GameRule_setBool)(rule,1);}
static void reset_cheat_gamerules(void*screen){void*always=always_day_button(screen);if(always)((UiSwitchButtonView*)always)->enabled=0;enable_daylight_cycle(screen);update_world_switches(screen);}

void relayout_world_options(void *screen, void *grid) {
    unsigned i;
    if (!screen || !grid) return;
    ui_scrolling_container_relayout(screen, grid);
    {
        UiIntVector *rows = &world_view(screen)->row_widths;
        unsigned row_count = (unsigned)(rows->end - rows->begin);
        int total_items = 0;
        for (i = 0; i < row_count; i++) total_items += rows->begin[i];
        world_view(screen)->grid_item_count = total_items;
    }
    ((ThisFn)SEAM_WorldSettingsScreen_refreshGridSelection)(screen);
}

static void install_world_controls(void*screen,unsigned kind){
    void*grid,*level,*always_day;UiShared*button,*label;int persisted;
    UiContainerView *container;
    UiSharedVector *children;
    UiElementView *grid_elem;
    unsigned child_count, i, always_idx = 0;
    UiElementView *always_elem = 0, *always_label = 0;
    UiElementView *cheats_btn_elem = 0, *cheats_label_elem = 0;

    if(!screen)return;
    ui_controls_destroy(&world_controls);
    s->world_kind=kind;
    s->world_screen=screen;
    s->world_cheats=0;
    clear_world_gamerule_controls();
    grid=world_option_container(screen);if(!grid)return;
    always_day=world_view(screen)->always_day.control?world_view(screen)->always_day.object:0;
    world_settings_register_gamerule_control(always_day);
    level=screen_level(screen);
    if(kind==2){
        if(persisted_edit_world_cheats(screen,&persisted))s->world_cheats=(u32)persisted;
        else if(level&&((HasCommandsEnabled)SEAM_HasCommandsEnabled)(level))s->world_cheats=1;
    }
    world_control_specs[1].initial_value=(int)s->world_cheats;
    if(ui_controls_build(screen,&world_controls)<0)return;
    button=(UiShared*)ui_controls_shared(&world_controls,WORLD_CHEATS);
    label=(UiShared*)ui_controls_shared(&world_controls,WORLD_CHEATS_LABEL_ID);
    sync_world_cheats(screen);

    container = (UiContainerView*)grid;
    children = &container->children;
    grid_elem = (UiElementView*)grid;

    /* 1. Find always_day and its label before we modify children */
    if(children->begin && children->end && always_day){
        child_count = (unsigned)(children->end - children->begin);
        for(i = 0; i < child_count; i++){
            if(children->begin[i].object == always_day){
                always_idx = i;
                always_elem = (UiElementView*)always_day;
                if(i > 0) always_label = (UiElementView*)children->begin[i-1].object;
                break;
            }
        }
    }

    /* 2. Position cheats controls to match always_day row */
    if(label && label->object && button && button->object && always_elem && always_label){
        cheats_label_elem = (UiElementView*)label->object;
        cheats_btn_elem = (UiElementView*)button->object;

        cheats_label_elem->x = always_label->x;
        cheats_label_elem->y = always_label->y;
        cheats_label_elem->width = always_label->width;
        cheats_label_elem->height = always_label->height;

        cheats_btn_elem->x = always_elem->x;
        cheats_btn_elem->y = always_elem->y;
        cheats_btn_elem->width = always_elem->width;
        cheats_btn_elem->height = always_elem->height;

        /* Shift always_day row and all elements below it down by 28 */
        for(i = (always_idx > 0 ? always_idx - 1 : always_idx); i < child_count; i++){
            UiElementView *e = (UiElementView*)children->begin[i].object;
            if(e) e->y += 28;
        }
    }

    if(!ui_grid_append_row(screen,grid,&world_view(screen)->row_widths,label,button,always_day))return;
    ui_controls_release_shared(&world_controls,WORLD_CHEATS);
    ui_controls_release_shared(&world_controls,WORLD_CHEATS_LABEL_ID);
    ui_controls_adopt(&world_controls,WORLD_CHEATS);
    ui_controls_adopt(&world_controls,WORLD_CHEATS_LABEL_ID);
    relayout_world_options(screen, grid);
}

static void install_achievement_banner(void*screen){
    void*grid=world_option_container(screen);
    UiContainerView*container;UiSharedVector*children;UiElementView*grid_elem,*banner_elem;
    UiIntVector*rows;
    UiShared banner_shared={0,0};
    unsigned child_count,row_count,i;
    int target_w=236,target_x;

    if(!grid)return;
    container=(UiContainerView*)grid;
    children=&container->children;
    if(!children->begin||!children->end)return;
    child_count=(unsigned)(children->end-children->begin);
    if(!child_count)return;

    grid_elem=(UiElementView*)grid;
    target_x=grid_elem->x+(grid_elem->width-target_w)/2;
    for(i=0;i<child_count;i++){
        UiElementView*e=(UiElementView*)children->begin[i].object;
        if(e&&e->width>150){
            target_w=e->width;
            target_x=e->x;
            break;
        }
    }

    if(!ui_achievement_banner_create_shared(screen,&banner_shared))return;
    ui_container_add_shared(grid,&banner_shared,1);
    ui_int_vector_push(&world_view(screen)->row_widths,1);
    ui_shared_release(&banner_shared);

    child_count=(unsigned)(children->end-children->begin);
    if(child_count>1){
        UiShared saved_child=children->begin[child_count-1];
        for(i=child_count-1;i>0;i--)children->begin[i]=children->begin[i-1];
        children->begin[0]=saved_child;
    }

    rows=&world_view(screen)->row_widths;
    row_count=(unsigned)(rows->end-rows->begin);
    if(row_count>1){
        int saved_width=rows->begin[row_count-1];
        for(i=row_count-1;i>0;i--)rows->begin[i]=rows->begin[i-1];
        rows->begin[0]=saved_width;
    }

    banner_elem=(UiElementView*)children->begin[0].object;
    if(banner_elem){
        banner_elem->width=target_w;
        banner_elem->height=44;
        banner_elem->x=target_x;
        banner_elem->y=grid_elem->y+4;
    }
    for(i=1;i<child_count;i++){
        UiElementView*e=(UiElementView*)children->begin[i].object;
        if(e)e->y+=48;
    }

    relayout_world_options(screen,grid);
}

static void after_create_setup(void*screen){
    ((ThisFn)s->world_create_setup.trampoline)(screen);
    s->edit_world_achievements_disabled=0;
    achievement_banner_setup_cache(screen);
    install_world_controls(screen,1);
    install_achievement_banner(screen);
    update_achievement_banner_state(screen);
    fix_world_type_button(screen);
}
static void after_edit_setup(void*screen){
    ((ThisFn)s->world_edit_setup.trampoline)(screen);
    achievement_banner_setup_cache(screen);
    install_world_controls(screen,2);
    world_transfer_edit_setup(screen);
    install_achievement_banner(screen);
    update_achievement_banner_state(screen);
    fix_world_type_button(screen);
    fix_edit_buttons(screen);
}
static int world_id(void*event){return ui_button_id(event);}
static void *on_warning_ctor(void*memory,void*game,void*client,int creative,int premade,int always_day){void*object=((WarningScreenCtor)s->warning_ctor.trampoline)(memory,game,client,creative,premade,always_day);if(object&&s->custom_warning)ui_warning_set_message(object,s->in_game_cheat_warning?"Achievements for this world will be permanently disabled because Cheats are enabled. The game will save if you continue.":"Achievements for this world will be permanently disabled because Cheats are enabled. Continue?");return object;}
static void on_create_world_start(void*screen,void*options){world_transfer_summary_invalidate();s->force_commands=(s->world_kind==1&&(s->world_cheats||world_screen_is_creative(screen)))?1u:0u;((CreateWorldStart)s->create_world_start.trampoline)(screen,options);}
static void on_world_cheats_pressed(void *screen,void *control){(void)control;sync_world_cheats(screen);if(!s->world_cheats)reset_cheat_gamerules(screen);}
static void on_create_world_press(void*screen,void*event,void*p3,void*p4){int id=world_id(event),force_warning=0;u8 old_restricted=0;if(world_controls.screen==screen&&ui_controls_dispatch(&world_controls,event))return;sync_world_cheats(screen);if(id==8&&(s->world_cheats||world_screen_is_creative(screen))){s->custom_warning=1;force_warning=1;old_restricted=world_view(screen)->restricted_setting;world_view(screen)->restricted_setting=1;}((WorldCreatePressFn)s->world_create_pressed.trampoline)(screen,event,p3,p4);if(force_warning){world_view(screen)->restricted_setting=old_restricted;s->custom_warning=0;}else sync_after_world_setting(screen,event);}
static void on_edit_world_press(void*screen,void*event){int id=world_id(event);void*level,*game,*player;if(world_transfer_edit_pressed(screen,event))return;if(world_controls.screen==screen&&ui_controls_dispatch(&world_controls,event))return;sync_world_cheats(screen);if(id==10){world_transfer_summary_invalidate();}if(id==9){level=screen_level(screen);game=ui_screen_game(screen);if(s->world_cheats)s->force_commands=1;else if(level&&((HasCommandsEnabled)SEAM_HasCommandsEnabled)(level)){((SetCommandsEnabled)SEAM_SetCommandsEnabled)(level,0);player=game?((GetPlayer)SEAM_Player_getPlayer)(game):0;if(player)((SetPermissionsLevel)SEAM_Player_setPermissionsLevel)(player,0);}}((WorldEditPressFn)s->world_edit_pressed.trampoline)(screen,event);if(id!=9)sync_after_world_setting(screen,event);fix_edit_buttons(screen);}

static int install(NuMC3DS_Hook*hook,u32 target,u32 replacement,u32 first,u32 second,const char*error,unsigned error_length,int code){hook->target=target;hook->replacement=replacement;hook->expected[0]=first;hook->expected[1]=second;if(s->host.install_hook(hook)){s->host.debug_string(error,error_length);return code;}return 0;}
int world_settings_ui_install_hooks(void){int result;if((result=install(&s->world_create_setup,SEAM_CreateWorldScreen_setup,(u32)after_create_setup,0xE92D4FF0u,0xE24DD014u,worldfail,sizeof(worldfail)-1,-9)))return result;if((result=install(&s->world_edit_setup,SEAM_EditWorldScreen_setup,(u32)after_edit_setup,0xE92D4FF0u,0xE1A04000u,worldfail,sizeof(worldfail)-1,-10)))return result;if((result=install(&s->world_create_pressed,SEAM_CreateWorldScreen_buttonPressed,(u32)on_create_world_press,0xE92D43F8u,0xE1A04000u,worldfail,sizeof(worldfail)-1,-11)))return result;if((result=install(&s->world_edit_pressed,SEAM_EditWorldScreen_buttonPressed,(u32)on_edit_world_press,0xE92D47F0u,0xE1A04000u,worldfail,sizeof(worldfail)-1,-12)))return result;if((result=install(&s->create_world_start,SEAM_CreateWorldScreen_start,(u32)on_create_world_start,0xE92D4FF0u,0xE1A04000u,createstartfail,sizeof(createstartfail)-1,-13)))return result;return install(&s->warning_ctor,SEAM_AchievementWarningScreen_ctor,(u32)on_warning_ctor,0xE92D43F0u,0xE24DD024u,warningfail,sizeof(warningfail)-1,-14);}
