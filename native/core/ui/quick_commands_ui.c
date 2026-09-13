#include "quick_commands_ui.h"
#include "chat_ui.h"
#include "ui_localize.h"
#include "ui_controls.h"
#include "ui_sprite.h"
#include "ui_screen_vtables.h"
#include "ui_widgets.h"

enum { QUICK_MAIN=0, QUICK_TELEPORT=1, QUICK_PLAYERS=2, QUICK_TIME=3, QUICK_WEATHER=4 };
enum {
    QUICK_SET_SPAWN=0x600, QUICK_OPEN_TELEPORT, QUICK_OPEN_TIME, QUICK_OPEN_WEATHER, QUICK_MAIN_BACK,
    QUICK_TP_WHO, QUICK_TP_WHERE, QUICK_TP_BACK,
    QUICK_PLAYER_0, QUICK_PLAYER_1, QUICK_PLAYER_2, QUICK_PLAYER_3,
    QUICK_PLAYER_4, QUICK_PLAYER_5, QUICK_PLAYER_6, QUICK_PLAYER_7, QUICK_PLAYER_BACK,
    QUICK_SUNRISE, QUICK_DAY, QUICK_NOON, QUICK_SUNSET, QUICK_NIGHT, QUICK_MIDNIGHT, QUICK_TIME_BACK,
    QUICK_CLEAR, QUICK_RAIN, QUICK_THUNDER, QUICK_WEATHER_BACK
};
enum { MAIN_BEGIN=0, MAIN_END=5, TP_BEGIN=5, TP_END=8, PLAYER_BEGIN=8, PLAYER_BACK=16, TIME_BEGIN=17, TIME_END=24, WEATHER_BEGIN=24, WEATHER_END=28 };

static const char atlas_resource[]="textures/gui/spritesheet";
static const Color background={0.545098f,0.545098f,0.545098f,1.0f};

static void quick_action(void *screen,void *button);
static const UiControlSpec quick_control_specs[]={
    {
        .id = QUICK_SET_SPAWN,
        .kind = UI_CTRL_BUTTON,
        .label_key = "hostOption.setWorldSpawn",
        .label_text = "Set World Spawn",
        .x = UI_CENTER_X_BOTTOM(260),
        .y = 18,
        .width = 260,
        .height = 29,
        .visible = 1,
        .screen_mask = UI_SCREEN_BOTTOM,
        .on_press = quick_action
    },
    {
        .id = QUICK_OPEN_TELEPORT,
        .kind = UI_CTRL_BUTTON,
        .label_key = "hostOption.teleport",
        .label_text = "Teleport",
        .x = UI_CENTER_X_BOTTOM(260),
        .y = 58,
        .width = 260,
        .height = 29,
        .visible = 1,
        .screen_mask = UI_SCREEN_BOTTOM,
        .on_press = quick_action
    },
    {
        .id = QUICK_OPEN_TIME,
        .kind = UI_CTRL_BUTTON,
        .label_key = "hostOption.time",
        .label_text = "Time",
        .x = UI_CENTER_X_BOTTOM(260),
        .y = 98,
        .width = 260,
        .height = 29,
        .visible = 1,
        .screen_mask = UI_SCREEN_BOTTOM,
        .on_press = quick_action
    },
    {
        .id = QUICK_OPEN_WEATHER,
        .kind = UI_CTRL_BUTTON,
        .label_key = "hostOption.weather",
        .label_text = "Weather",
        .x = UI_CENTER_X_BOTTOM(260),
        .y = 138,
        .width = 260,
        .height = 29,
        .visible = 1,
        .screen_mask = UI_SCREEN_BOTTOM,
        .on_press = quick_action
    },
    {
        .id = QUICK_MAIN_BACK,
        .kind = UI_CTRL_BUTTON,
        .label_key = "gui.back",
        .label_text = "Back",
        .x = UI_CENTER_X_BOTTOM(260),
        .y = 198,
        .width = 260,
        .height = 29,
        .visible = 1,
        .screen_mask = UI_SCREEN_BOTTOM,
        .on_press = quick_action
    },
    {
        .id = QUICK_TP_WHO,
        .kind = UI_CTRL_BUTTON,
        .label_key = "hostOption.teleport.who",
        .label_text = "Who",
        .x = UI_CENTER_X_BOTTOM(260),
        .y = 58,
        .width = 260,
        .height = 29,
        .visible = 1,
        .screen_mask = UI_SCREEN_BOTTOM,
        .on_press = quick_action
    },
    {
        .id = QUICK_TP_WHERE,
        .kind = UI_CTRL_BUTTON,
        .label_key = "hostOption.teleport.where",
        .label_text = "Where",
        .x = UI_CENTER_X_BOTTOM(260),
        .y = 98,
        .width = 260,
        .height = 29,
        .visible = 1,
        .screen_mask = UI_SCREEN_BOTTOM,
        .on_press = quick_action
    },
    {
        .id = QUICK_TP_BACK,
        .kind = UI_CTRL_BUTTON,
        .label_key = "gui.back",
        .label_text = "Back",
        .x = UI_CENTER_X_BOTTOM(260),
        .y = 198,
        .width = 260,
        .height = 29,
        .visible = 1,
        .screen_mask = UI_SCREEN_BOTTOM,
        .on_press = quick_action
    },
    {
        .id = QUICK_PLAYER_0,
        .kind = UI_CTRL_BUTTON,
        .label_key = "",
        .label_text = "Player",
        .x = 10,
        .y = 35,
        .width = 145,
        .height = 29,
        .visible = 1,
        .screen_mask = UI_SCREEN_BOTTOM,
        .on_press = quick_action
    },
    {
        .id = QUICK_PLAYER_1,
        .kind = UI_CTRL_BUTTON,
        .label_key = "",
        .label_text = "Player",
        .x = UI_ALIGN_RIGHT_BOTTOM(145, 10),
        .y = 35,
        .width = 145,
        .height = 29,
        .visible = 1,
        .screen_mask = UI_SCREEN_BOTTOM,
        .on_press = quick_action
    },
    {
        .id = QUICK_PLAYER_2,
        .kind = UI_CTRL_BUTTON,
        .label_key = "",
        .label_text = "Player",
        .x = 10,
        .y = 75,
        .width = 145,
        .height = 29,
        .visible = 1,
        .screen_mask = UI_SCREEN_BOTTOM,
        .on_press = quick_action
    },
    {
        .id = QUICK_PLAYER_3,
        .kind = UI_CTRL_BUTTON,
        .label_key = "",
        .label_text = "Player",
        .x = UI_ALIGN_RIGHT_BOTTOM(145, 10),
        .y = 75,
        .width = 145,
        .height = 29,
        .visible = 1,
        .screen_mask = UI_SCREEN_BOTTOM,
        .on_press = quick_action
    },
    {
        .id = QUICK_PLAYER_4,
        .kind = UI_CTRL_BUTTON,
        .label_key = "",
        .label_text = "Player",
        .x = 10,
        .y = 115,
        .width = 145,
        .height = 29,
        .visible = 1,
        .screen_mask = UI_SCREEN_BOTTOM,
        .on_press = quick_action
    },
    {
        .id = QUICK_PLAYER_5,
        .kind = UI_CTRL_BUTTON,
        .label_key = "",
        .label_text = "Player",
        .x = UI_ALIGN_RIGHT_BOTTOM(145, 10),
        .y = 115,
        .width = 145,
        .height = 29,
        .visible = 1,
        .screen_mask = UI_SCREEN_BOTTOM,
        .on_press = quick_action
    },
    {
        .id = QUICK_PLAYER_6,
        .kind = UI_CTRL_BUTTON,
        .label_key = "",
        .label_text = "Player",
        .x = 10,
        .y = 155,
        .width = 145,
        .height = 29,
        .visible = 1,
        .screen_mask = UI_SCREEN_BOTTOM,
        .on_press = quick_action
    },
    {
        .id = QUICK_PLAYER_7,
        .kind = UI_CTRL_BUTTON,
        .label_key = "",
        .label_text = "Player",
        .x = UI_ALIGN_RIGHT_BOTTOM(145, 10),
        .y = 155,
        .width = 145,
        .height = 29,
        .visible = 1,
        .screen_mask = UI_SCREEN_BOTTOM,
        .on_press = quick_action
    },
    {
        .id = QUICK_PLAYER_BACK,
        .kind = UI_CTRL_BUTTON,
        .label_key = "gui.back",
        .label_text = "Back",
        .x = UI_CENTER_X_BOTTOM(260),
        .y = 198,
        .width = 260,
        .height = 29,
        .visible = 1,
        .screen_mask = UI_SCREEN_BOTTOM,
        .on_press = quick_action
    },
    {
        .id = QUICK_SUNRISE,
        .kind = UI_CTRL_BUTTON,
        .label_key = "hostOption.time.sunrise",
        .label_text = "Sunrise",
        .x = 10,
        .y = 40,
        .width = 145,
        .height = 29,
        .visible = 1,
        .screen_mask = UI_SCREEN_BOTTOM,
        .on_press = quick_action
    },
    {
        .id = QUICK_DAY,
        .kind = UI_CTRL_BUTTON,
        .label_key = "hostOption.time.day",
        .label_text = "Day",
        .x = UI_ALIGN_RIGHT_BOTTOM(145, 10),
        .y = 40,
        .width = 145,
        .height = 29,
        .visible = 1,
        .screen_mask = UI_SCREEN_BOTTOM,
        .on_press = quick_action
    },
    {
        .id = QUICK_NOON,
        .kind = UI_CTRL_BUTTON,
        .label_key = "hostOption.time.noon",
        .label_text = "Noon",
        .x = 10,
        .y = 85,
        .width = 145,
        .height = 29,
        .visible = 1,
        .screen_mask = UI_SCREEN_BOTTOM,
        .on_press = quick_action
    },
    {
        .id = QUICK_SUNSET,
        .kind = UI_CTRL_BUTTON,
        .label_key = "hostOption.time.sunset",
        .label_text = "Sunset",
        .x = UI_ALIGN_RIGHT_BOTTOM(145, 10),
        .y = 85,
        .width = 145,
        .height = 29,
        .visible = 1,
        .screen_mask = UI_SCREEN_BOTTOM,
        .on_press = quick_action
    },
    {
        .id = QUICK_NIGHT,
        .kind = UI_CTRL_BUTTON,
        .label_key = "hostOption.time.night",
        .label_text = "Night",
        .x = 10,
        .y = 130,
        .width = 145,
        .height = 29,
        .visible = 1,
        .screen_mask = UI_SCREEN_BOTTOM,
        .on_press = quick_action
    },
    {
        .id = QUICK_MIDNIGHT,
        .kind = UI_CTRL_BUTTON,
        .label_key = "hostOption.time.midnight",
        .label_text = "Midnight",
        .x = UI_ALIGN_RIGHT_BOTTOM(145, 10),
        .y = 130,
        .width = 145,
        .height = 29,
        .visible = 1,
        .screen_mask = UI_SCREEN_BOTTOM,
        .on_press = quick_action
    },
    {
        .id = QUICK_TIME_BACK,
        .kind = UI_CTRL_BUTTON,
        .label_key = "gui.back",
        .label_text = "Back",
        .x = UI_CENTER_X_BOTTOM(260),
        .y = 198,
        .width = 260,
        .height = 29,
        .visible = 1,
        .screen_mask = UI_SCREEN_BOTTOM,
        .on_press = quick_action
    },
    {
        .id = QUICK_CLEAR,
        .kind = UI_CTRL_BUTTON,
        .label_key = "hostOption.weather.clear",
        .label_text = "Clear",
        .x = UI_CENTER_X_BOTTOM(260),
        .y = 48,
        .width = 260,
        .height = 29,
        .visible = 1,
        .screen_mask = UI_SCREEN_BOTTOM,
        .on_press = quick_action
    },
    {
        .id = QUICK_RAIN,
        .kind = UI_CTRL_BUTTON,
        .label_key = "hostOption.weather.rain",
        .label_text = "Rain",
        .x = UI_CENTER_X_BOTTOM(260),
        .y = 93,
        .width = 260,
        .height = 29,
        .visible = 1,
        .screen_mask = UI_SCREEN_BOTTOM,
        .on_press = quick_action
    },
    {
        .id = QUICK_THUNDER,
        .kind = UI_CTRL_BUTTON,
        .label_key = "hostOption.weather.thunderstorm",
        .label_text = "Thunderstorm",
        .x = UI_CENTER_X_BOTTOM(260),
        .y = 138,
        .width = 260,
        .height = 29,
        .visible = 1,
        .screen_mask = UI_SCREEN_BOTTOM,
        .on_press = quick_action
    },
    {
        .id = QUICK_WEATHER_BACK,
        .kind = UI_CTRL_BUTTON,
        .label_key = "gui.back",
        .label_text = "Back",
        .x = UI_CENTER_X_BOTTOM(260),
        .y = 198,
        .width = 260,
        .height = 29,
        .visible = 1,
        .screen_mask = UI_SCREEN_BOTTOM,
        .on_press = quick_action
    }
};
static UiControlSet quick_controls={quick_control_specs,sizeof(quick_control_specs)/sizeof(quick_control_specs[0])};
static void *quick_button(unsigned index){return index<quick_controls.instance_count?quick_controls.instances[index].object:0;}

static void *quick_level(void*screen){void*client=ui_screen_client(screen);return client?((GetLevel)SEAM_ClientInstance_getLevel)(client):0;}
static int quick_has_player(void*player){unsigned i;for(i=0;i<s->quick_player_count;i++)if(s->quick_players[i]==player)return 1;return 0;}
static void quick_set_label(void*screen,void*button,const char*label){ui_button_set_label(button,label);ui_button_sync_label_width(screen,button);}
static void quick_teleport_labels(void*screen){
    char label[96],base[40],name[32];unsigned n;
    ui_localized_label(base,sizeof(base),"hostOption.teleport.who","Who");n=0;label[0]=0;append(label,&n,base);append(label,&n,": ");
    if(s->quick_tp_source){entity_name(s->quick_tp_source,name,sizeof(name));append(label,&n,name);}else append(label,&n,"-");quick_set_label(screen,quick_button(TP_BEGIN),label);
    ui_localized_label(base,sizeof(base),"hostOption.teleport.where","Where");n=0;label[0]=0;append(label,&n,base);append(label,&n,": ");
    if(s->quick_tp_destination){entity_name(s->quick_tp_destination,name,sizeof(name));append(label,&n,name);}else append(label,&n,"-");quick_set_label(screen,quick_button(TP_BEGIN+1),label);
}
static void quick_refresh_players(void*screen){
    void*game=ui_screen_game(screen),*level=quick_level(screen),*local=game?((GetPlayer)SEAM_Player_getPlayer)(game):0;unsigned i;
    s->quick_player_count=level?collect_targets(local,level,"@a",s->quick_players,8):0;
    if(!s->quick_player_count&&local){s->quick_players[0]=local;s->quick_player_count=1;}
    if(!quick_has_player(s->quick_tp_source))s->quick_tp_source=local&&quick_has_player(local)?local:(s->quick_player_count?s->quick_players[0]:0);
    if(!quick_has_player(s->quick_tp_destination))s->quick_tp_destination=0;
    for(i=0;i<8;i++){if(i<s->quick_player_count){entity_name(s->quick_players[i],s->quick_player_names[i],sizeof(s->quick_player_names[i]));quick_set_label(screen,quick_button(PLAYER_BEGIN+i),s->quick_player_names[i]);}else s->quick_player_names[i][0]=0;}
    quick_teleport_labels(screen);
}

static void quick_set_page(void*screen,u32 page){
    unsigned i,begin=0,end=0,selection=0;
    s->quick_page=page;
    if(page==QUICK_MAIN){begin=MAIN_BEGIN;end=MAIN_END;}
    else if(page==QUICK_TELEPORT){begin=TP_BEGIN;end=TP_END;}
    else if(page==QUICK_TIME){begin=TIME_BEGIN;end=TIME_END;}
    else if(page==QUICK_WEATHER){begin=WEATHER_BEGIN;end=WEATHER_END;}
    for(i=0;i<quick_controls.instance_count;i++)ui_element_set_visible(quick_button(i),0);
    if(page==QUICK_PLAYERS){for(i=0;i<s->quick_player_count;i++)ui_element_set_visible(quick_button(PLAYER_BEGIN+i),1);ui_element_set_visible(quick_button(PLAYER_BACK),1);selection=s->quick_player_count?PLAYER_BEGIN:PLAYER_BACK;}
    else {for(i=begin;i<end;i++)ui_element_set_visible(quick_button(i),1);selection=begin;}
    ui_screen_set_selection(screen,selection);
}

static void quick_close(void*screen){if(screen!=s->quick_screen||s->quick_closing)return;s->quick_closing=1;ui_screen_close(screen);}
static void quick_finish(void*screen,const char*command){chat_ui_set_input(command);quick_close(screen);}
static void quick_back(void*screen){if(s->quick_page==QUICK_MAIN)quick_close(screen);else if(s->quick_page==QUICK_PLAYERS)quick_set_page(screen,QUICK_TELEPORT);else quick_set_page(screen,QUICK_MAIN);}

static void quick_append_target(char*out,unsigned*length,void*entity,void*local){char name[32];if(entity==local){append(out,length,"@p");return;}entity_name(entity,name,sizeof(name));append(out,length,"\"");append(out,length,name);append(out,length,"\"");}
static void quick_finish_teleport(void*screen){char command[MAX_TEXT+1];unsigned n=0;void*game=ui_screen_game(screen),*local=game?((GetPlayer)SEAM_Player_getPlayer)(game):0;if(!s->quick_tp_source||!s->quick_tp_destination)return;command[0]=0;append(command,&n,"/tp ");quick_append_target(command,&n,s->quick_tp_source,local);append(command,&n," ");quick_append_target(command,&n,s->quick_tp_destination,local);quick_finish(screen,command);}

static void quick_action(void*screen,void*button){
    int id;if(!button||screen!=s->quick_screen)return;id=ui_button_id(button);
    if(id==QUICK_SET_SPAWN)quick_finish(screen,"/setworldspawn ");
    else if(id==QUICK_OPEN_TELEPORT){quick_refresh_players(screen);quick_set_page(screen,QUICK_TELEPORT);}
    else if(id==QUICK_OPEN_TIME)quick_set_page(screen,QUICK_TIME);
    else if(id==QUICK_OPEN_WEATHER)quick_set_page(screen,QUICK_WEATHER);
    else if(id==QUICK_TP_WHO||id==QUICK_TP_WHERE){s->quick_tp_selecting=(id==QUICK_TP_WHERE);quick_refresh_players(screen);quick_set_page(screen,QUICK_PLAYERS);}
    else if(id>=QUICK_PLAYER_0&&id<=QUICK_PLAYER_7){unsigned index=(unsigned)(id-QUICK_PLAYER_0);if(index>=s->quick_player_count)return;if(s->quick_tp_selecting){s->quick_tp_destination=s->quick_players[index];quick_finish_teleport(screen);}else{s->quick_tp_source=s->quick_players[index];quick_teleport_labels(screen);quick_set_page(screen,QUICK_TELEPORT);}}
    else if(id==QUICK_MAIN_BACK||id==QUICK_TP_BACK||id==QUICK_PLAYER_BACK||id==QUICK_TIME_BACK||id==QUICK_WEATHER_BACK)quick_back(screen);
    else if(id==QUICK_SUNRISE)quick_finish(screen,"/time set 23000");
    else if(id==QUICK_DAY)quick_finish(screen,"/time set 1000");
    else if(id==QUICK_NOON)quick_finish(screen,"/time set 6000");
    else if(id==QUICK_SUNSET)quick_finish(screen,"/time set 12000");
    else if(id==QUICK_NIGHT)quick_finish(screen,"/time set 13000");
    else if(id==QUICK_MIDNIGHT)quick_finish(screen,"/time set 18000");
    else if(id==QUICK_CLEAR)quick_finish(screen,"/weather clear");
    else if(id==QUICK_RAIN)quick_finish(screen,"/weather rain");
    else if(id==QUICK_THUNDER)quick_finish(screen,"/weather thunder");
}

static void quick_press(void*screen,void*button){if(!button||screen!=s->quick_screen)return;ui_play_button_sound(screen);(void)ui_controls_dispatch(&quick_controls,button);}

static int quick_on_back(void*screen,int reason){(void)reason;quick_back(screen);return 1;}
static void quick_closed(void*screen){(void)screen;s->quick_active=s->quick_closing=0;}

static void quick_mapped(void*screen,int value){
    if(value==s->keyboard.keyboard_binding_ok){quick_press(screen,ui_screen_selected(screen));return;}
    if(value==s->keyboard.keyboard_binding_cancel){quick_back(screen);return;}
    if(value>=0&&(value==s->keyboard.keyboard_binding_up||value==s->keyboard.keyboard_binding_down||value==s->keyboard.keyboard_binding_left||value==s->keyboard.keyboard_binding_right))return;
    if(value>=0&&(value==s->keyboard.keyboard_binding_snap_left||value==s->keyboard.keyboard_binding_cursor_left||value==s->keyboard.keyboard_binding_snap_right||value==s->keyboard.keyboard_binding_cursor_right))return;
    ((void(*)(void*,int))SEAM_Screen_handleMappedButton)(screen,value);
}

static void quick_build_sprites(void*screen){
    s->quick_sprites[0]=ui_sprite_create(screen,atlas_resource,274,68,8,8,200,40,8,8);
    s->quick_sprites[1]=ui_sprite_create(screen,atlas_resource,274,108,8,8,200,40,8,8);
    s->quick_sprites[2]=ui_sprite_create(screen,atlas_resource,274,148,8,8,200,40,8,8);
    s->quick_sprites[3]=0;
    s->quick_sprites[4]=ui_sprite_create(screen,atlas_resource,18,46,17,17,216,32,17,17);
    s->quick_sprites[5]=ui_sprite_create(screen,atlas_resource,173,46,17,17,233,32,17,17);
    s->quick_sprites[6]=ui_sprite_create(screen,atlas_resource,18,91,17,17,215,56,17,17);
    s->quick_sprites[7]=ui_sprite_create(screen,atlas_resource,173,91,17,17,232,56,17,17);
    s->quick_sprites[8]=ui_sprite_create(screen,atlas_resource,18,136,17,17,200,73,17,17);
    s->quick_sprites[9]=ui_sprite_create(screen,atlas_resource,173,136,17,17,217,73,17,17);
    s->quick_sprites[10]=ui_sprite_create(screen,atlas_resource,38,54,17,17,234,73,17,17);
    s->quick_sprites[11]=ui_sprite_create(screen,atlas_resource,38,99,17,17,208,90,17,17);
    s->quick_sprites[12]=ui_sprite_create(screen,atlas_resource,38,144,17,17,225,90,17,17);
}

static void quick_setup(void*screen){
    unsigned i;
    if(quick_controls.instance_count){quick_set_page(screen,QUICK_MAIN);return;}
    if(ui_controls_build(screen,&quick_controls)<0)return;
    for(i=0;i<quick_controls.instance_count;i++){ui_button_set_manual_render(quick_button(i),1);ui_screen_add_selectable(screen,quick_button(i));}
    quick_build_sprites(screen);quick_set_page(screen,QUICK_MAIN);
}

static void quick_draw_icons(void){unsigned i;if(s->quick_page==QUICK_MAIN){for(i=0;i<3;i++)ui_sprite_draw(s->quick_sprites[i]);}else if(s->quick_page==QUICK_TELEPORT){ui_sprite_draw(s->quick_sprites[0]);ui_sprite_draw(s->quick_sprites[1]);}else if(s->quick_page==QUICK_TIME){for(i=4;i<=9;i++)ui_sprite_draw(s->quick_sprites[i]);}else if(s->quick_page==QUICK_WEATHER){for(i=10;i<=12;i++)ui_sprite_draw(s->quick_sprites[i]);}}
static void quick_render(void*screen,int x,int y,int use_screen,float tick){unsigned i;(void)tick;if(!(use_screen&0x80))return;ui_fill(screen,0,0,320,240,&background);for(i=0;i<quick_controls.instance_count;i++)ui_button_draw_native(screen,quick_button(i),x,y);quick_draw_icons();if(ui_screen_selected(screen))ui_button_draw_highlight(screen,ui_screen_selected(screen));}

void quick_commands_open(void*keyboard_screen){
    static const UiCustomScreenHooks hooks={
        .setup=quick_setup,
        .render=quick_render,
        .press=quick_press,
        .on_back=quick_on_back,
        .closed=quick_closed,
        .mapped=quick_mapped
    };
    UiShared shared;
    void*game=ui_screen_game(keyboard_screen),*client=ui_screen_client(keyboard_screen);
    if(s->quick_active||!game||!client)return;
    shared.object=s->quick_screen;
    shared.control=s->quick_control;
    if(!ui_custom_screen_push(game,client,&s->quick_vtable,&hooks,&shared))return;
    s->quick_screen=shared.object;
    s->quick_control=shared.control;
    s->quick_active=1;
    s->quick_closing=0;
    s->quick_page=QUICK_MAIN;
    s->quick_tp_source=s->quick_tp_destination=0;
    s->quick_tp_selecting=0;
}

void quick_commands_reset(void){UiShared held={s->quick_screen,s->quick_control};unsigned i;for(i=0;i<13;i++){ui_sprite_destroy(s->quick_sprites[i]);s->quick_sprites[i]=0;}ui_controls_destroy(&quick_controls);ui_shared_release(&held);s->quick_screen=s->quick_control=s->quick_tp_source=s->quick_tp_destination=0;s->quick_active=s->quick_closing=s->quick_page=s->quick_player_count=s->quick_tp_selecting=0;zero(s->quick_players,sizeof(s->quick_players));zero(s->quick_player_names,sizeof(s->quick_player_names));}
