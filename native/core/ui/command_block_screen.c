#include "command_block_screen.h"
#include "../internal.h"
#include "../world/command_block/command_block_model.h"
#include "ui_controls.h"
#include "ui_localize.h"
#include "ui_screen_vtables.h"
#include "ui_text_layout.h"
#include "text_entry_session.h"
#include "ui_dropdown.h"
#include "ui_form_style.h"
#include "ui_sprite.h"

enum { CB_COMMAND=0x700,CB_NAME,CB_MODE,CB_CONDITION,CB_REDSTONE,CB_TRACK,CB_CANCEL,CB_DONE };
static CommandBlockModel model;
static UiShared held;
static void *screen_vtable;
static int opened,closing,dropdown;
static UiCachedText text_cache[42];
static UiFormHeader header;
static void *mode_icons[3];
static NuMC3DS_Hook open_hook;
static const Color white={1,1,1,1};
static const Color mode_colors[]={{1.0f,0.60f,0.25f,1},{0.65f,0.50f,1.0f,1},{0.35f,0.85f,0.65f,1}};
static const char *cached_keys[42];
static int layout_dirty=1;
static char field_lines[2][152],output_lines[13][160];
static unsigned output_count;
static void action(void *screen,void *button);
static const char *mode_keys[]={"commandBlockScreen.blockType.impulse","commandBlockScreen.blockType.repeat","commandBlockScreen.blockType.chain"};
static const char *condition_keys[]={"commandBlockScreen.condition.unconditional","commandBlockScreen.condition.conditional"};
static const char *redstone_keys[]={"commandBlockScreen.redstone.always_on","commandBlockScreen.redstone.needs_redstone"};
static void dropdown_changed(void *listener,int id);
static void dropdown_toggled(void *listener,void *control,int expanded);
static void *dropdown_vtable[]={0,0,dropdown_changed,dropdown_toggled};
static UiDropdownListener dropdown_listener={dropdown_vtable};
static const UiDropdownOptions mode_options={mode_keys,3,&dropdown_listener},condition_options={condition_keys,2,&dropdown_listener},redstone_options={redstone_keys,2,&dropdown_listener};
static const UiControlSpec specs[]={
    {
        .id = CB_COMMAND,
        .kind = UI_CTRL_BUTTON,
        .label_key = "",
        .label_text = "",
        .x = UI_CENTER_X_BOTTOM(300),
        .y = 25,
        .width = 300,
        .height = 27,
        .visible = 1,
        .screen_mask = UI_SCREEN_BOTTOM,
        .on_press = action
    },
    {
        .id = CB_NAME,
        .kind = UI_CTRL_BUTTON,
        .label_key = "",
        .label_text = "",
        .x = UI_CENTER_X_BOTTOM(300),
        .y = 73,
        .width = 300,
        .height = 27,
        .visible = 1,
        .screen_mask = UI_SCREEN_BOTTOM,
        .on_press = action
    },
    {
        .id = CB_MODE,
        .kind = UI_CTRL_DROPDOWN,
        .label_key = "",
        .label_text = "",
        .x = 10,
        .y = 124,
        .width = 145,
        .height = 26,
        .visible = 1,
        .screen_mask = UI_SCREEN_BOTTOM,
        .on_press = action,
        .native_seam = &mode_options
    },
    {
        .id = CB_CONDITION,
        .kind = UI_CTRL_DROPDOWN,
        .label_key = "",
        .label_text = "",
        .x = UI_ALIGN_RIGHT_BOTTOM(145, 10),
        .y = 124,
        .width = 145,
        .height = 26,
        .visible = 1,
        .screen_mask = UI_SCREEN_BOTTOM,
        .on_press = action,
        .native_seam = &condition_options
    },
    {
        .id = CB_REDSTONE,
        .kind = UI_CTRL_DROPDOWN,
        .label_key = "",
        .label_text = "",
        .x = 10,
        .y = 173,
        .width = 145,
        .height = 26,
        .visible = 1,
        .screen_mask = UI_SCREEN_BOTTOM,
        .on_press = action,
        .native_seam = &redstone_options
    },
    {
        .id = CB_TRACK,
        .kind = UI_CTRL_SWITCH,
        .label_key = "",
        .label_text = "",
        .x = UI_ALIGN_RIGHT_BOTTOM(145, 10),
        .y = 177,
        .width = 33,
        .height = 18,
        .visible = 1,
        .screen_mask = UI_SCREEN_BOTTOM,
        .on_press = action
    },
    {
        .id = CB_CANCEL,
        .kind = UI_CTRL_BUTTON,
        .label_key = "gui.cancel",
        .label_text = "Cancel",
        .x = 10,
        .y = 207,
        .width = 145,
        .height = 26,
        .visible = 1,
        .screen_mask = UI_SCREEN_BOTTOM,
        .on_press = action
    },
    {
        .id = CB_DONE,
        .kind = UI_CTRL_BUTTON,
        .label_key = "gui.done",
        .label_text = "Done",
        .x = UI_ALIGN_RIGHT_BOTTOM(145, 10),
        .y = 207,
        .width = 145,
        .height = 26,
        .visible = 1,
        .screen_mask = UI_SCREEN_BOTTOM,
        .on_press = action
    }
};
static UiControlSet controls={specs,sizeof(specs)/sizeof(specs[0])};
static void *button(int id){return ui_controls_find(&controls,id);}

static void refresh(void){
    layout_dirty=1;
    ui_dropdown_set_value(button(CB_MODE),model.mode);
    ui_dropdown_set_value(button(CB_CONDITION),model.conditional);
    ui_dropdown_set_value(button(CB_REDSTONE),model.redstone);
    if(button(CB_MODE)){((UiButtonView*)button(CB_MODE))->normal_label=mode_colors[model.mode<=2?model.mode:0];((UiButtonView*)button(CB_MODE))->pressed_label=((UiButtonView*)button(CB_MODE))->normal_label;}
    if(button(CB_TRACK))*((u8*)button(CB_TRACK)+0xf8)=model.track_output;
}

static void close_screen(void *screen){
    if(closing)return;closing=1;
    ui_screen_close(screen);
}

static void dropdown_changed(void *listener,int id){
    int value=ui_dropdown_value(button(id));(void)listener;
    if(id==CB_MODE){model.mode=value;((UiButtonView*)button(CB_MODE))->normal_label=mode_colors[value];((UiButtonView*)button(CB_MODE))->pressed_label=mode_colors[value];}
    else if(id==CB_CONDITION)model.conditional=value;
    else if(id==CB_REDSTONE)model.redstone=value;
}
static void dropdown_toggled(void *listener,void *control,int expanded){
    (void)listener;dropdown=expanded?ui_button_id(control):0;
}

static void edited(void *context,const char *text,int accepted){
    if(accepted&&opened)command_block_model_set_text((u32*)context,text);
    if(opened)refresh();
}

static void action(void *screen,void *control){
    int id=ui_button_id(control);
    if(id==CB_MODE||id==CB_CONDITION||id==CB_REDSTONE)return;
    else if(id==CB_COMMAND){ui_text_entry_set_title("advMode.command");ui_text_entry_open(screen,command_block_model_text(model.command),32500,1,edited,&model.command);}
    else if(id==CB_NAME){ui_text_entry_set_title("commandBlockScreen.hoverNote");ui_text_entry_open(screen,command_block_model_text(model.name),15,0,edited,&model.name);}
    else if(id==CB_TRACK){model.track_output=*((u8*)control+0xf8);}
    else if(id==CB_CANCEL)close_screen(screen);
    else if(id==CB_DONE&&command_block_model_save(&model))close_screen(screen);
}

static void press(void *screen,void *control){if(control&&!closing){if(dropdown&&control!=button(dropdown))return;int id=ui_button_id(control);if(id==CB_MODE||id==CB_CONDITION||id==CB_REDSTONE)return;ui_play_button_sound(screen);ui_controls_dispatch(&controls,control);}}
static int back(void *screen,int reason){(void)reason;if(dropdown)ui_dropdown_input(screen,button(dropdown),s->keyboard.keyboard_binding_cancel);else close_screen(screen);return 1;}
static void closed(void *screen){(void)screen;opened=closing=0;command_block_model_destroy(&model);}
static void move(void *screen,int source,int direction){(void)source;if(dropdown)return;ui_screen_navigate(screen,direction);}
static void mapped(void *screen,int value){
    if(dropdown){ui_dropdown_input(screen,button(dropdown),value);return;}
    if(value==s->keyboard.keyboard_binding_ok){
        void *selected=ui_screen_selected(screen);int id=selected?ui_button_id(selected):0;
        if(id==CB_MODE||id==CB_CONDITION||id==CB_REDSTONE)ui_dropdown_input(screen,selected,value);
        else if(id==CB_TRACK)((void(*)(void*,void*,int))0x001DD3F8u)(selected,ui_screen_game(screen),value);
        else press(screen,selected);
        return;
    }
    if(value==s->keyboard.keyboard_binding_cancel){back(screen,0);return;}
    if(value>=0&&(value==s->keyboard.keyboard_binding_up||value==s->keyboard.keyboard_binding_down||value==s->keyboard.keyboard_binding_left||value==s->keyboard.keyboard_binding_right))return;
    if(value>=0&&(value==s->keyboard.keyboard_binding_snap_left||value==s->keyboard.keyboard_binding_cursor_left||value==s->keyboard.keyboard_binding_snap_right||value==s->keyboard.keyboard_binding_cursor_right))return;
    ((void(*)(void*,int))SEAM_Screen_handleMappedButton)(screen,value);
}

static void setup(void *screen){
    unsigned index;
    if(!controls.instance_count){
        if(ui_controls_build(screen,&controls)<0)return;
        for(index=0;index<controls.instance_count;index++){
            UiElementView *element=controls.instances[index].object;
            if(specs[index].kind==UI_CTRL_SWITCH){
                element->x=specs[index].x;element->y=specs[index].y;
                element->width=specs[index].width;element->height=specs[index].height;
                ui_element_set_screen(element,specs[index].screen_mask);
                ui_screen_add_button(screen,&controls.instances[index].shared);
            }
            ui_button_set_manual_render(controls.instances[index].object,1);
            ui_screen_add_selectable(screen,controls.instances[index].object);
        }
    }
    ui_button_set_label(button(CB_CANCEL),"");ui_button_set_label(button(CB_DONE),"");
    refresh();ui_screen_set_selection(screen,7);
}

static void text(void *screen,unsigned index,const char *value,int x,int y){
    ui_cached_text_set(screen,&text_cache[index],value);
    ui_cached_text_draw(screen,&text_cache[index],x,y,1,&white);
}
static void localized(void *screen,unsigned index,const char *key,int x,int y){
    if(cached_keys[index]!=key){char value[160];ui_localized_label(value,sizeof(value),key,"");ui_cached_text_set(screen,&text_cache[index],value);cached_keys[index]=key;}
    ui_cached_text_draw(screen,&text_cache[index],x,y,1,&white);
}

static void small_localized(void *screen,unsigned index,const char *key,int x,int y,float scale){
    if(cached_keys[index]!=key){char value[160];ui_localized_label(value,sizeof(value),key,"");ui_cached_text_set(screen,&text_cache[index],value);cached_keys[index]=key;}
    ui_cached_text_draw(screen,&text_cache[index],x,y,scale,index==31?&mode_colors[model.mode<=2?model.mode:0]:&white);
}
static void field(void *screen,int id,unsigned cache,const char *value){
    UiElementView *element=button(id);char line[152];unsigned next;
    ui_form_field(screen,element->x,element->y,element->width,element->height,1);
    (void)value;(void)line;(void)next;
    text(screen,cache,field_lines[id==CB_NAME],element->x+6,element->y+8);
}
static void prepare_layout(void *screen){
    unsigned at=0,next,i;char value[80];
    if(!layout_dirty)return;
    ui_text_wrap_line(ui_screen_font(screen),command_block_model_text(model.command),0,288,field_lines[0],sizeof(field_lines[0]),&next);
    ui_text_wrap_line(ui_screen_font(screen),command_block_model_text(model.name),0,288,field_lines[1],sizeof(field_lines[1]),&next);
    output_count=0;
    for(i=0;i<13;i++){
        if(!ui_text_wrap_line(ui_screen_font(screen),command_block_model_text(model.output),at,232,output_lines[i],sizeof(output_lines[i]),&next))break;
        output_count++;at=next;
    }
    ui_localized_label(value,sizeof(value),model.is_minecart?"item.command_block_minecart.name":"tile.command_block.name","");ui_cached_text_set(screen,&text_cache[40],value);
    ui_localized_label(value,sizeof(value),"gui.cancel","");ui_cached_text_set(screen,&text_cache[38],value);
    ui_localized_label(value,sizeof(value),"gui.done","");ui_cached_text_set(screen,&text_cache[39],value);
    layout_dirty=0;
}
static void render(void *screen,int x,int y,int mask,float tick){
    unsigned index,offset=0,next_offset;char line[160];(void)tick;
    if(!opened||closing)return;
    if(!command_block_model_valid(&model)){close_screen(screen);return;}
    prepare_layout(screen);
    if(mask&UI_SCREEN_TOP){
        ui_fill(screen,0,0,UI_SCREEN_TOP_WIDTH,UI_SCREEN_HEIGHT,&ui_form_background);ui_form_field(screen,7,55,246,178,model.track_output?0:1);
        

        ui_form_header(screen,UI_SCREEN_TOP_WIDTH,&text_cache[40],&header);
        for(index=0;index<3;index++)if(!mode_icons[index])mode_icons[index]=ui_sprite_create(screen,"textures/gui/command_block_modes",8,7,16,16,index*16,0,16,16);
        ui_sprite_draw(mode_icons[model.mode<=2?model.mode:0]);
        localized(screen,0,"advMode.previousOutput",10,37);
        for(index=0;model.track_output&&index<output_count;index++)text(screen,index+8,output_lines[index],13,62+index*13);
        small_localized(screen,1,"advMode.nearestPlayer",262,59,0.65f);
        small_localized(screen,2,"advMode.randomPlayer",262,77,0.65f);
        small_localized(screen,3,"advMode.allPlayers",262,95,0.65f);
        small_localized(screen,4,"advMode.allEntities",262,113,0.65f);
        if(!model.is_minecart){
            small_localized(screen,30,"commandBlockScreen.blockType",262,139,0.7f);
            small_localized(screen,31,mode_keys[model.mode<=2?model.mode:0],262,152,0.8f);
            small_localized(screen,32,"commandBlockScreen.condition",262,170,0.7f);
            small_localized(screen,33,condition_keys[!!model.conditional],262,183,0.8f);
            small_localized(screen,34,"commandBlockScreen.redstone",262,201,0.7f);
            small_localized(screen,35,redstone_keys[!!model.redstone],262,214,0.75f);
        }
    }
    if(mask&UI_SCREEN_BOTTOM){
        ui_fill(screen,0,0,UI_SCREEN_BOTTOM_WIDTH,UI_SCREEN_HEIGHT,&ui_form_background);
        localized(screen,23,"advMode.command",10,10);localized(screen,24,"commandBlockScreen.hoverNote",10,58);
        if(!model.is_minecart){
            localized(screen,25,"commandBlockScreen.blockType",10,107);localized(screen,26,"commandBlockScreen.condition",165,107);
            localized(screen,27,"commandBlockScreen.redstone",10,156);
        }
        small_localized(screen,28,"advMode.previousOutput",165,158,0.8f);
        for(index=0;index<controls.instance_count;index++){
            int id=specs[index].id;
            if(model.is_minecart&&(id==CB_MODE||id==CB_CONDITION||id==CB_REDSTONE))continue;
            if(id!=CB_COMMAND&&id!=CB_NAME)ui_button_draw_native(screen,controls.instances[index].object,x,y);
        }
        field(screen,CB_COMMAND,36,command_block_model_text(model.command));field(screen,CB_NAME,37,command_block_model_text(model.name));
        {
            static const Color ink={0.30f,0.30f,0.30f,1};char value[64];
            ui_cached_text_draw(screen,&text_cache[38],10+(145-text_cache[38].width)/2,215,1,&ink);
            ui_cached_text_draw(screen,&text_cache[39],165+(145-text_cache[39].width)/2,215,1,&ink);
        }
        if(!dropdown&&(ui_screen_selected(screen)==button(CB_COMMAND)||ui_screen_selected(screen)==button(CB_NAME)))ui_button_draw_highlight(screen,ui_screen_selected(screen));
        if(dropdown&&!model.is_minecart)ui_dropdown_draw_popup(screen,button(dropdown),x,y);
    }
}

static void show_screen(void *game){
    static const UiCustomScreenHooks hooks = {
        .setup = setup,
        .render = render,
        .press = press,
        .on_back = back,
        .closed = closed,
        .mapped = mapped,
        .move = move
    };
    void *client = s->screen ? ui_screen_client(s->screen) : 0;
    if (!ui_custom_screen_push(game, client, &screen_vtable, &hooks, &held)) {
        command_block_model_destroy(&model);
        return;
    }
    layout_dirty = 1; opened = 1; closing = dropdown = 0;
}

void command_block_screen_open(void *player,const int *position){
    void *game=s->session_game?s->session_game:s->host.minecraft_game;
    void *client=s->screen?ui_screen_client(s->screen):0;
    if(opened||!game||!client||!command_block_can_use(player))return;
    if(!command_block_model_init(&model,player,position)){command_block_model_destroy(&model);return;}
    show_screen(game);
}

void command_block_screen_open_entity(void *player,void *entity){
    void *game=s->session_game?s->session_game:s->host.minecraft_game;
    void *client=s->screen?ui_screen_client(s->screen):0;
    if(opened||!game||!client||!command_block_can_use(player))return;
    if(!command_block_model_init_entity(&model,player,entity)){command_block_model_destroy(&model);return;}
    show_screen(game);
}

void command_block_screen_reset(void){
    unsigned index;ui_text_entry_finish(0);opened=closing=0;
    for(index=0;index<3;index++){ui_sprite_destroy(mode_icons[index]);mode_icons[index]=0;}
    ui_form_header_reset(&header);ui_controls_destroy(&controls);ui_shared_release(&held);held.object=held.control=0;
    if(screen_vtable)((void(*)(void*))SEAM_operator_delete)((u8*)screen_vtable-8);
    screen_vtable=0;command_block_model_destroy(&model);
    for(index=0;index<42;index++){ui_cached_text_reset(&text_cache[index]);cached_keys[index]=0;}
}

int command_block_screen_install(void){
    open_hook.target=0x0019678Cu;open_hook.replacement=(u32)command_block_screen_open;
    open_hook.expected[0]=0xE92D4070u;open_hook.expected[1]=0xE1A04000u;
    return s->host.install_hook(&open_hook);
}
