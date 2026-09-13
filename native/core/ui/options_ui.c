#include "options_ui.h"
#include "fps_overlay.h"
#include "ui_controls.h"
#include "ui_localize.h"
#include "hand_visibility.h"
#include "../opt/render/leaf_cull_fast.h"

typedef void (*OptionsCategoriesFn)(void*);
typedef void (*OptionsOwnerPressedFn)(void*,void*);
typedef void (*WarningPressedFn)(void*,void*);
typedef void *(*OptionsRowCtorFn)(void*,const void*,UiShared*);
typedef void (*PushAchievementWarningFn)(void*,int,int,int);
typedef void (*PushSavingScreenFn)(void*,int);
typedef void (*OptionsRenderFn)(void*,void*,void*,unsigned);
typedef void (*OptionItemLocalizedNameFn)(u32*,void*,void*);
typedef void (*OptionsVectorPushFn)(void*,Shared*);
typedef void (*SliderUpdateFromTouchFn)(void*,void*);
typedef int (*SliderUsesDirectionalInputFn)(void*);
typedef int (*TouchPadCoordFn)(void);
typedef void *(*GetScreenFn)(void*);
typedef void (*ScreenTransformTouchFn)(void*,int,int,int*,int*);
typedef void (*SliderOnValueChangedFn)(void*,void*);

static const char options_fail[]="NuMC3DS UI: options category hook failed\n";
static const char options_vector_push_fail[]="NuMC3DS UI: options vector push hook failed\n";
static const char options_slider_touch_fail[]="NuMC3DS UI: slider touch hook failed\n";
static const char options_render_fail[]="NuMC3DS UI: options render hook failed\n";
static const char option_item_name_fail[]="NuMC3DS UI: option label hook failed\n";
static const char options_press_fail[]="NuMC3DS UI: options button hook failed\n";
static const char warning_press_fail[]="NuMC3DS UI: warning button hook failed\n";

static void on_hide_hand_pressed(void*screen,void*button);
static void on_hide_hud_pressed(void*screen,void*button);
static void on_show_fps_pressed(void*screen,void*button);
static void on_cheats_pressed(void*screen,void*button);
static void on_always_day_pressed(void*screen,void*button);

static const UiControlSpec graphics_control_specs[]={
    {
        .id = OPTIONS_HIDE_HAND,
        .kind = UI_CTRL_SWITCH,
        .label_text = "Hide Hand",
        .visible = 1,
        .on_press = on_hide_hand_pressed
    },
    {
        .id = OPTIONS_HIDE_HUD,
        .kind = UI_CTRL_SWITCH,
        .label_text = "Hide HUD",
        .visible = 1,
        .on_press = on_hide_hud_pressed
    },
    {
        .id = OPTIONS_SHOW_FPS,
        .kind = UI_CTRL_SWITCH,
        .label_text = "Show FPS",
        .visible = 1,
        .on_press = on_show_fps_pressed
    }
};
static const UiControlSpec game_control_specs[]={
    {
        .id = OPTIONS_CHEATS,
        .kind = UI_CTRL_SWITCH,
        .label_key = "selectWorld.cheats",
        .label_text = "Cheats",
        .visible = 1,
        .on_press = on_cheats_pressed
    },
    {
        .id = OPTIONS_ALWAYS_DAY,
        .kind = UI_CTRL_SWITCH,
        .label_key = "createWorldScreen.alwaysDay",
        .label_text = "Always Day",
        .visible = 1,
        .on_press = on_always_day_pressed
    }
};
static UiControlSet graphics_controls={graphics_control_specs,sizeof(graphics_control_specs)/sizeof(graphics_control_specs[0]),{{0}},0,0};
static UiControlSet game_controls={game_control_specs,sizeof(game_control_specs)/sizeof(game_control_specs[0]),{{0}},0,0};

static void *screen_level(void*screen){void*game=ui_screen_game(screen),*clients,*client;if(!game)return 0;clients=*(void**)((u8*)game+0xC0);client=clients?*(void**)clients:0;return client?((GetLevel)SEAM_ClientInstance_getLevel)(client):0;}
static int is_in_game(void*screen){return screen&&*((u8*)screen+SEAM_Options_inGameOffset)!=0;}
static int commands_enabled(void*screen){void*level=screen_level(screen);return level&&((HasCommandsEnabled)SEAM_HasCommandsEnabled)(level);}
static int world_achievements_disabled(void*screen){void*level=screen_level(screen);void*data=level?(u8*)level+SEAM_Level_levelDataOffset:0;return data&&((int(*)(void*))SEAM_LevelData_achievementsWillBeDisabledOnLoad)(data)!=0;}
static void *daylight_rule(void*screen){void*level=screen_level(screen);return level?rule_dfs(rules_root((u8*)level+SEAM_GameRules_map_offset),"dodaylightcycle"):0;}
static int always_day_enabled(void*screen){void*rule=daylight_rule(screen);return rule?!((RuleGetBoolFn)SEAM_GameRule_getBool)(rule):0;}
static void set_always_day(void*screen,int enabled){void*rule=daylight_rule(screen);if(rule)((RuleSetBoolFn)SEAM_GameRule_setBool)(rule,!enabled);}
static void set_command_access(void*screen,int enabled){void*level=screen_level(screen),*game=ui_screen_game(screen),*player;if(level)((SetCommandsEnabled)SEAM_SetCommandsEnabled)(level,enabled!=0);player=game?((GetPlayer)SEAM_Player_getPlayer)(game):0;if(player)((SetPermissionsLevel)SEAM_Player_setPermissionsLevel)(player,enabled?2:0);}
static void refresh_switches(void*screen){int cheats=commands_enabled(screen);void*cheat=ui_controls_find(&game_controls,OPTIONS_CHEATS),*always=ui_controls_find(&game_controls,OPTIONS_ALWAYS_DAY);if(cheat)((UiSwitchButtonView*)cheat)->enabled=(u8)cheats;if(always){((UiSwitchButtonView*)always)->enabled=(u8)always_day_enabled(screen);((UiElementView*)always)->active=(u8)cheats;}}
static void move_component_to(UiSharedVector*vector,void*component,unsigned index){UiShared value,*at,*found=0;if(!vector||!vector->begin||!component||index>=(unsigned)(vector->end-vector->begin))return;for(at=vector->begin;at<vector->end;at++)if(at->object==component){found=at;break;}if(!found||found==vector->begin+index)return;value=*found;if(found>vector->begin+index){for(at=found;at>vector->begin+index;at--)*at=at[-1];}else{for(at=found;at<vector->begin+index;at++)*at=at[1];}vector->begin[index]=value;}
static void label_append(char *out,unsigned capacity,unsigned *length,const char *text){while(text&&*text&&*length+1<capacity)out[(*length)++]=*text++;out[*length]=0;}
static void label_append_int(char *out,unsigned capacity,unsigned *length,unsigned value){char digits[10];unsigned count=0;do{digits[count++]=(char)('0'+value%10u);value/=10u;}while(value&&count<sizeof(digits));while(count&&*length+1<capacity)out[(*length)++]=digits[--count];out[*length]=0;}
static void format_fixed(char *out,int value,int decimals){unsigned length=0,scale=1,whole,fraction;int i;out[0]=0;label_append(out,OPTIONS_LABEL_CAP,&length,": ");for(i=0;i<decimals;i++)scale*=10;if(value<0){label_append(out,OPTIONS_LABEL_CAP,&length,"-");value=-value;}whole=(unsigned)value/(unsigned)scale;fraction=(unsigned)value%(unsigned)scale;label_append_int(out,OPTIONS_LABEL_CAP,&length,whole);if(decimals){label_append(out,OPTIONS_LABEL_CAP,&length,".");for(i=scale/10;i>1&&fraction<(unsigned)i;i/=10)label_append(out,OPTIONS_LABEL_CAP,&length,"0");label_append_int(out,OPTIONS_LABEL_CAP,&length,fraction);}}
static void format_option_label(char *out,unsigned capacity,const char *base,const char *suffix){unsigned length=0;if(!capacity)return;out[0]=0;label_append(out,capacity,&length,base);label_append(out,capacity,&length,suffix);}
static void update_option_value_suffix(char*cached,int value,int decimals){char label[OPTIONS_LABEL_CAP];format_fixed(label,value,decimals);if(!streq(label,cached))copy_text(cached,label,OPTIONS_LABEL_CAP-1);}
static int options_float(void*screen,const void*key,float*out){void*game=ui_screen_game(screen),*options;if(!game||!out)return 0;options=((MinecraftGameGetOptionsFn)SEAM_MinecraftGame_getOptions)(game);if(!options)return 0;*out=((OptionsGetFloatFn)SEAM_Options_getFloat)(options,key);return 1;}
static void update_graphics_value_labels(void*screen){float value;if(!s->options_brightness_row||!s->options_fov_row)return;if(options_float(screen,(const void*)SEAM_Options_brightness,&value))update_option_value_suffix(s->options_brightness_value,(int)(value*100.0f+0.5f),0);if(options_float(screen,(const void*)SEAM_Options_fov,&value))update_option_value_suffix(s->options_fov_value,(s32)(value*100.0f+(value>=0.0f?0.5f:-0.5f)),2);}
static int option_vector_contains(const UiSharedVector*vector,void*component){UiShared*entry;if(!vector||!component)return 0;for(entry=vector->begin;entry&&entry<vector->end;entry++)if(entry->object==component)return 1;return 0;}
int options_ui_is_option_row(void*component){UiSharedVector*graphics,*game;if(!s->options_screen||!component)return 0;graphics=(UiSharedVector*)((u8*)s->options_screen+SEAM_Options_graphicsVectorOffset);game=(UiSharedVector*)((u8*)s->options_screen+SEAM_Options_gameVectorOffset);return option_vector_contains(graphics,component)||option_vector_contains(game,component);}
static int append_switch_row(void*screen,UiControlSet*controls,UiSharedVector*vector,int id,int enabled,void**row_out){UiShared row={0,0};const UiShared*toggle;const UiControlSpec*spec;u32 label=0;void*memory,*object;GameAllocWithSelector alloc=(GameAllocWithSelector)SEAM_Heap_allocWithSelector;if(row_out)*row_out=0;spec=ui_controls_spec(controls,id);toggle=ui_controls_shared(controls,id);if(!spec||!toggle)return 0;((UiSwitchButtonView*)toggle->object)->enabled=(u8)(enabled!=0);if(!ui_localized_string(spec->label_key,spec->label_text,&label))return 0;memory=alloc(0x70,(void*)SEAM_game_alloc_selector);object=memory?((OptionsRowCtorFn)SEAM_OptionsRow_ctor)(memory,&label,(UiShared*)toggle):0;((StrDtor)SEAM_StrDtor)(&label);if(!object)return 0;((ThisFn)SEAM_OptionItem_layoutChildren)(object);if(!ui_shared_from_object(object,&row))return 0;((PushFn)SEAM_OptionsVector_push)(vector,(Shared*)&row);if(row_out)*row_out=object;ui_shared_release(&row);ui_controls_release_shared(controls,id);ui_controls_adopt(controls,id);return 1;}

static void append_native_fov_slider(void*screen){UiShared built={0,0},held={0,0};UiSharedVector*graphics=(UiSharedVector*)((u8*)screen+SEAM_Options_graphicsVectorOffset);if(!graphics||s->options_fov_row)return;((void(*)(UiShared*,void*,const void*))SEAM_OptionsScreen_buildSlider)(&built,screen,(const void*)SEAM_Options_fov);if(built.object&&built.control){((void(*)(UiShared*,void*,void*))SEAM_Shared_copyFromParts)(&held,built.control,built.object);if(held.object&&held.control){((PushFn)SEAM_OptionsVector_push)(graphics,(Shared*)&held);s->options_fov_row=built.object;((ThisFn)SEAM_Shared_localDtor)(&held);}}((ThisFn)SEAM_OptionBuildResult_dtor)(&built);}
static void *append_native_switch(void*screen,const void*option){UiShared built={0,0},held={0,0};UiSharedVector*graphics=(UiSharedVector*)((u8*)screen+SEAM_Options_graphicsVectorOffset);void*row=0;if(!graphics)return 0;((void(*)(UiShared*,void*,const void*))SEAM_OptionsScreen_buildSwitch)(&built,screen,option);if(built.object&&built.control){((void(*)(UiShared*,void*,void*))SEAM_Shared_copyFromParts)(&held,built.control,built.object);if(held.object&&held.control){((PushFn)SEAM_OptionsVector_push)(graphics,(Shared*)&held);row=built.object;((ThisFn)SEAM_Shared_localDtor)(&held);}}((ThisFn)SEAM_OptionBuildResult_dtor)(&built);return row;}
static void on_options_vector_push(void*vector,Shared*value){UiSharedVector*graphics;((OptionsVectorPushFn)s->options_vector_push.trampoline)(vector,value);if(!s->options_building_categories||s->options_inserting_fov||!s->options_screen||s->options_fov_row)return;graphics=(UiSharedVector*)((u8*)s->options_screen+SEAM_Options_graphicsVectorOffset);if(vector!=graphics||!graphics->begin||graphics->end-graphics->begin!=1)return;s->options_inserting_fov=1;append_native_fov_slider(s->options_screen);s->options_inserting_fov=0;}
static void append_graphics_controls(void*screen){UiSharedVector*graphics=(UiSharedVector*)((u8*)screen+SEAM_Options_graphicsVectorOffset);void*view_bobbing_row,*fancy_graphics_row,*beautiful_skies_row;s->options_brightness_row=s->options_hide_hand_row=s->options_hide_hud_row=0;zero(s->options_brightness_value,sizeof(s->options_brightness_value));zero(s->options_fov_value,sizeof(s->options_fov_value));ui_localized_label(s->options_brightness_base,sizeof(s->options_brightness_base),"options.brightness","Brightness");ui_localized_label(s->options_fov_base,sizeof(s->options_fov_base),"options.fov","FOV");ui_controls_set_context(&graphics_controls,graphics);if(ui_controls_build(screen,&graphics_controls)<0)return;if(!graphics||!graphics->begin||graphics->end-graphics->begin<3||graphics->begin[1].object!=s->options_fov_row){ui_controls_destroy(&graphics_controls);return;}s->options_brightness_row=graphics->begin[0].object;view_bobbing_row=graphics->begin[2].object;fancy_graphics_row=append_native_switch(screen,(const void*)SEAM_Options_fancyGraphics);beautiful_skies_row=append_native_switch(screen,(const void*)SEAM_Options_fancySkies);append_switch_row(screen,&graphics_controls,graphics,OPTIONS_HIDE_HUD,top_hud_visibility_hidden(),&s->options_hide_hud_row);append_switch_row(screen,&graphics_controls,graphics,OPTIONS_HIDE_HAND,hand_visibility_hidden(),&s->options_hide_hand_row);move_component_to(graphics,s->options_brightness_row,0);move_component_to(graphics,s->options_fov_row,1);move_component_to(graphics,view_bobbing_row,2);move_component_to(graphics,fancy_graphics_row,3);move_component_to(graphics,beautiful_skies_row,4);move_component_to(graphics,s->options_hide_hud_row,5);move_component_to(graphics,s->options_hide_hand_row,6);append_switch_row(screen,&graphics_controls,graphics,OPTIONS_SHOW_FPS,fps_overlay_enabled(),0);}
static void append_in_game_world_controls(void*screen){UiSharedVector*game_options;if(!is_in_game(screen))return;game_options=(UiSharedVector*)((u8*)screen+SEAM_Options_gameVectorOffset);if(ui_controls_build(screen,&game_controls)<0)return;if(!append_switch_row(screen,&game_controls,game_options,OPTIONS_CHEATS,commands_enabled(screen),0))return;if(!append_switch_row(screen,&game_controls,game_options,OPTIONS_ALWAYS_DAY,always_day_enabled(screen),0))return;refresh_switches(screen);}
static void after_create_categories(void*screen){ui_controls_destroy(&graphics_controls);ui_controls_destroy(&game_controls);s->options_screen=screen;s->options_brightness_row=s->options_fov_row=s->options_hide_hand_row=s->options_hide_hud_row=0;s->options_building_categories=1;s->options_inserting_fov=0;((OptionsCategoriesFn)s->options_categories.trampoline)(screen);s->options_building_categories=0;append_graphics_controls(screen);append_in_game_world_controls(screen);}
static void on_options_render(void*screen,void*param2,void*param3,unsigned flags){if(screen==s->options_screen){update_graphics_value_labels(screen);}((OptionsRenderFn)s->options_render.trampoline)(screen,param2,param3,flags);}
static void on_option_item_localized_name(u32*out,void*row,void*game){const char*base=0,*suffix=0;char label[OPTIONS_LABEL_CAP*2];u32 scratch=0;if(row==s->options_brightness_row){base=s->options_brightness_base;suffix=s->options_brightness_value;}else if(row==s->options_fov_row){base=s->options_fov_base;suffix=s->options_fov_value;}else{((OptionItemLocalizedNameFn)s->option_item_name.trampoline)(out,row,game);return;}format_option_label(label,sizeof(label),base,suffix);((StrCtor)SEAM_StrCtor)(out,label,&scratch);}

static void on_hide_hand_pressed(void*screen,void*button){if(!screen||!button)return;ui_play_button_sound(screen);hand_visibility_set_hidden(((UiSwitchButtonView*)button)->enabled!=0);}
static void on_hide_hud_pressed(void*screen,void*button){if(!screen||!button)return;ui_play_button_sound(screen);top_hud_visibility_set_hidden(((UiSwitchButtonView*)button)->enabled!=0);}
static void on_show_fps_pressed(void*screen,void*button){if(!screen||!button)return;ui_play_button_sound(screen);fps_overlay_set_enabled(((UiSwitchButtonView*)button)->enabled!=0);}
static void on_always_day_pressed(void*screen,void*button){if(!screen||!button)return;if(!commands_enabled(screen)){refresh_switches(screen);return;}ui_play_button_sound(screen);set_always_day(screen,((UiSwitchButtonView*)button)->enabled!=0);refresh_switches(screen);}
static void on_cheats_pressed(void*screen,void*button){void*game,*chooser;int enabled;if(!screen||!button)return;ui_play_button_sound(screen);enabled=((UiSwitchButtonView*)button)->enabled!=0;if(!enabled){set_command_access(screen,0);refresh_switches(screen);return;}if(world_achievements_disabled(screen)){set_command_access(screen,1);refresh_switches(screen);return;}((UiSwitchButtonView*)button)->enabled=0;refresh_switches(screen);game=ui_screen_game(screen);chooser=game?*(void**)((u8*)game+SEAM_MinecraftGame_screenChooserOffset):0;if(!chooser)return;s->custom_warning=1;s->in_game_cheat_warning=1;((PushAchievementWarningFn)SEAM_PushAchievementWarning)(chooser,1,0,0);s->custom_warning=0;}
static void on_options_owner_pressed(void*owner,void*button){void*screen=owner?(u8*)owner-0x0C:0;if(screen==s->options_screen&&ui_controls_dispatch(&graphics_controls,button)){leaf_refresh_graphics_mode(0);return;}if(screen==s->options_screen&&is_in_game(screen)&&ui_controls_dispatch(&game_controls,button))return;((OptionsOwnerPressedFn)s->options_pressed.trampoline)(owner,button);leaf_refresh_graphics_mode(0);}
static void on_warning_pressed(void*warning,void*button){void*game,*chooser;int id;if(!s->in_game_cheat_warning){((WarningPressedFn)s->warning_pressed.trampoline)(warning,button);return;}id=ui_button_id(button);game=ui_screen_game(warning);chooser=game?*(void**)((u8*)game+SEAM_MinecraftGame_screenChooserOffset):0;ui_play_button_sound(warning);if(id==1){s->in_game_cheat_warning=0;if(chooser)((void(*)(void*,int))SEAM_ScreenChooser_schedulePopScreen)(chooser,1);return;}if(id!=0)return;if(s->options_screen)set_command_access(s->options_screen,1);if(ui_controls_find(&game_controls,OPTIONS_CHEATS))((UiSwitchButtonView*)ui_controls_find(&game_controls,OPTIONS_CHEATS))->enabled=1;if(s->options_screen)refresh_switches(s->options_screen);s->in_game_cheat_warning=0;if(chooser){((void(*)(void*,int))SEAM_ScreenChooser_schedulePopScreen)(chooser,1);((PushSavingScreenFn)SEAM_PushSavingScreen)(chooser,0);}}

static void on_slider_update_from_touch(void*slider,void*game){
    int touch_x,touch_y,slider_x,slider_w;
    float pos;
    u8*slider_bytes=(u8*)slider;
    void*screen;
    void(**screen_vtable)(void*,int,int,int*,int*);
    void(**slider_vtable)(void*,void*);
    SliderOnValueChangedFn on_value_changed;

    if(!slider||!game)return;
    if(((SliderUsesDirectionalInputFn)SEAM_Slider_usesDirectionalInput)(game)&&((signed char*)slider_bytes)[0x30]){
        ((SliderUpdateFromTouchFn)s->options_slider_touch.trampoline)(slider,game);
        return;
    }
    touch_x=((TouchPadCoordFn)SEAM_TouchPad_getX)();
    touch_y=((TouchPadCoordFn)SEAM_TouchPad_getY)();
    screen=((GetScreenFn)SEAM_MinecraftGame_getScreen)(game);
    if(screen){
        screen_vtable=*(void(***)(void*,int,int,int*,int*))screen;
        if(screen_vtable&&screen_vtable[264/sizeof(void*)])
            ((ScreenTransformTouchFn)screen_vtable[264/sizeof(void*)])(screen,320,240,&touch_x,&touch_y);
    }
    if(!slider_bytes[0x4C])return;
    slider_x=*(int*)(slider_bytes+0x0C);
    slider_w=*(int*)(slider_bytes+0x14);
    if(slider_w<=0)return;
    pos=(float)(touch_x-slider_x)/(float)slider_w;
    if(pos>1.0f)pos=1.0f;
    if(pos<0.0f)pos=0.0f;
    *(float*)(slider_bytes+0x50)=pos;
    slider_vtable=*(void(***)(void*,void*))slider;
    if(slider_vtable){
        on_value_changed=(SliderOnValueChangedFn)slider_vtable[0x80/sizeof(void*)];
        if(on_value_changed)on_value_changed(slider,game);
    }
}

static int install(NuMC3DS_Hook*hook,u32 target,u32 replacement,u32 first,u32 second,const char*error,unsigned length,int code){hook->target=target;hook->replacement=replacement;hook->expected[0]=first;hook->expected[1]=second;if(s->host.install_hook(hook)){s->host.debug_string(error,length);return code;}return 0;}
int options_ui_install_hooks(void){int result;result=install(&s->options_categories,SEAM_OptionsScreen_createCategorySprites,(u32)after_create_categories,0xE92D43F0u,0xE1A04000u,options_fail,sizeof(options_fail)-1,-28);if(result)return result;result=install(&s->options_vector_push,SEAM_OptionsVector_push,(u32)on_options_vector_push,0xE92D4FF0u,0xE1A06000u,options_vector_push_fail,sizeof(options_vector_push_fail)-1,-33);if(result)return result;result=install(&s->options_slider_touch,SEAM_Slider_updateFromTouch,(u32)on_slider_update_from_touch,0xE92D40F0u,0xE1A04000u,options_slider_touch_fail,sizeof(options_slider_touch_fail)-1,-34);if(result)return result;result=install(&s->options_render,SEAM_OptionsScreen_render,(u32)on_options_render,0xE92D41F0u,0xE3130040u,options_render_fail,sizeof(options_render_fail)-1,-29);if(result)return result;result=install(&s->option_item_name,SEAM_OptionItem_getLocalizedName,(u32)on_option_item_localized_name,0xE92D4070u,0xE1A04001u,option_item_name_fail,sizeof(option_item_name_fail)-1,-32);if(result)return result;result=install(&s->options_pressed,SEAM_OptionsScreen_ownerButtonPressed,(u32)on_options_owner_pressed,0xE5911078u,0xE240000Cu,options_press_fail,sizeof(options_press_fail)-1,-30);if(result)return result;return install(&s->warning_pressed,SEAM_AchievementWarningScreen_buttonPressed,(u32)on_warning_pressed,0xE92D4FF0u,0xE1A04001u,warning_press_fail,sizeof(warning_press_fail)-1,-31);}
