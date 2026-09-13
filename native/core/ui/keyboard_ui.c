#include "keyboard_ui.h"
#include "../internal.h"
#include "keyboard_input.h"
#include "ui_sprite.h"
#include "ui_screen_vtables.h"
#include "ui_widgets.h"
#include "keyboard_label_mesh.h"
static UiKeyboardAction action_callback;
static unsigned capabilities;
static int submitted;
static void notify_action(void*screen,int action){if(action_callback)action_callback(screen,action);}
static const char *const upper[KEYS]={"1","2","3","4","5","6","7","8","9","0","Q","W","E","R","T","Y","U","I","O","P","A","S","D","F","G","H","J","K","L","Z","X","C","V","B","N","M"};
const char *const lower[KEYS]={"1","2","3","4","5","6","7","8","9","0","q","w","e","r","t","y","u","i","o","p","a","s","d","f","g","h","j","k","l","z","x","c","v","b","n","m"};
static const char *const symbols1[KEYS]={"1","2","3","4","5","6","7","8","9","0","+","*","/","=","_","-","<",">","[","]","!","@","#","$","%","&","*","(",")","-","'","\"",":",";",",","?"};
static const char *const symbols2[KEYS]={"1","2","3","4","5","6","7","8","9","0","`","~","\\","|","{","}"};
enum { K_CAPS=KEYS, K_DEL, K_MODE, K_SLASH, K_SPACE, K_PERIOD, K_AT, K_TILDE, K_CARET, K_BACK, K_SEND, K_TAB, K_CMD, K_LEFT, K_UP, K_DOWN, K_RIGHT };
enum { SYMBOL_LEFT, SYMBOL_UP, SYMBOL_DOWN, SYMBOL_RIGHT, SYMBOL_CAPS_NORMAL, SYMBOL_CAPS_SHIFT, SYMBOL_CAPS_LOCKED, SYMBOL_DEL, SYMBOL_COUNT };
enum { KB_X=11, KB_BUTTON=28, KB_STEP=30, KB_ONE_HALF=43 };
static u8 keyboard_row_buttons[KEYBOARD_ROWS][10]={{K_TAB,K_CMD,K_LEFT,K_UP,K_DOWN,K_RIGHT,0,0,0,0},{0,1,2,3,4,5,6,7,8,9},{10,11,12,13,14,15,16,17,18,19},{20,21,22,23,24,25,26,27,28,0},{K_CAPS,29,30,31,32,33,34,35,K_DEL,0},{K_MODE,K_SLASH,K_AT,K_SPACE,K_PERIOD,K_TILDE,K_CARET,0,0,0},{K_BACK,K_SEND,0,0,0,0,0,0,0,0}};
static u8 keyboard_row_counts[KEYBOARD_ROWS]={6,10,10,9,9,7,2};
static const char keyboard_symbols_resource[]="textures/gui/spritesheet";

static const char*key_label(unsigned i){if(s->keyboard.page==1)return symbols1[i];if(s->keyboard.page==2)return symbols2[i]?symbols2[i]:"";return s->keyboard.caps?upper[i]:lower[i];}
static int keyboard_index_is_symbol(unsigned index){return index>=K_LEFT&&index<=K_RIGHT||index==K_DEL||(index==K_CAPS&&!s->keyboard.page);}
static int keyboard_button_is_arrow(void*button){unsigned i;for(i=K_LEFT;i<=K_RIGHT;i++)if(s->keyboard.buttons[i]==button)return 1;return 0;}
static void *keyboard_profile_font(void*screen){return ui_screen_font(screen);}
static int keyboard_abs(int value){return value<0?-value:value;}
static void keyboard_label_runs(void*screen){keyboard_label_mesh_build(screen,s->keyboard.buttons,s->keyboard.count);s->keyboard.keyboard_cache_font=ui_screen_font(screen);}
static void keyboard_labels(void*screen){unsigned i;for(i=0;i<KEYS;i++)ui_button_set_label(s->keyboard.buttons[i],key_label(i));ui_button_set_label(s->keyboard.buttons[K_CAPS],s->keyboard.page==1?"1/2":s->keyboard.page==2?"2/2":"");ui_button_set_label(s->keyboard.buttons[K_DEL],"");ui_button_set_label(s->keyboard.buttons[K_MODE],s->keyboard.page?"ABC":"!#1");for(i=K_LEFT;i<=K_RIGHT;i++)ui_button_set_label(s->keyboard.buttons[i],"");keyboard_label_runs(screen);}
static void keyboard_destroy_symbols(void){unsigned i;for(i=0;i<SYMBOL_COUNT;i++){ui_sprite_destroy(s->keyboard.keyboard_symbols[i]);s->keyboard.keyboard_symbols[i]=0;}}
static void keyboard_build_symbols(void*screen){
    static const int source_x[SYMBOL_COUNT]={112,128,144,160,176,192,208,224};
    static const int destination_x[SYMBOL_COUNT]={197,227,257,287,24,24,24,279};
    static const int destination_y[SYMBOL_COUNT]={17,17,17,17,147,147,147,147};
    void*symbols[SYMBOL_COUNT];unsigned i;
    for(i=0;i<SYMBOL_COUNT;i++)if(!s->keyboard.keyboard_symbols[i])break;
    if(i==SYMBOL_COUNT)return;
    keyboard_destroy_symbols();zero(symbols,sizeof(symbols));
    for(i=0;i<SYMBOL_COUNT;i++){
        symbols[i]=ui_sprite_create(screen,keyboard_symbols_resource,destination_x[i],destination_y[i],16,16,source_x[i],224,16,16);
        if(!symbols[i]){while(i)ui_sprite_destroy(symbols[--i]);return;}
    }
    cp(s->keyboard.keyboard_symbols,symbols,sizeof(symbols));
}
static void keyboard_draw_symbols(void){unsigned i;for(i=SYMBOL_LEFT;i<=SYMBOL_RIGHT;i++)ui_sprite_draw(s->keyboard.keyboard_symbols[i]);if(!s->keyboard.page)ui_sprite_draw(s->keyboard.keyboard_symbols[SYMBOL_CAPS_NORMAL+s->keyboard.caps]);ui_sprite_draw(s->keyboard.keyboard_symbols[SYMBOL_DEL]);}
static void autocomplete_after_edit(void){notify_action(s->keyboard.keyboard_screen,UI_KEYBOARD_CHANGED);}
static void insert_char(char c){unsigned i;if(s->keyboard.length>=s->keyboard.input_limit)return;for(i=s->keyboard.length;i>s->keyboard.cursor;i--)s->keyboard.text[i]=s->keyboard.text[i-1];s->keyboard.text[s->keyboard.cursor++]=c;s->keyboard.text[++s->keyboard.length]=0;s->keyboard.blink=0;autocomplete_after_edit();}
static void backspace(void){unsigned i;if(!s->keyboard.cursor)return;for(i=s->keyboard.cursor-1;i<s->keyboard.length;i++)s->keyboard.text[i]=s->keyboard.text[i+1];s->keyboard.cursor--;s->keyboard.length--;s->keyboard.blink=0;autocomplete_after_edit();}
static void set_edit_text(const char*t){copy_text(s->keyboard.text,t,s->keyboard.input_limit);s->keyboard.length=text_len(s->keyboard.text);s->keyboard.cursor=s->keyboard.length;s->keyboard.blink=0;autocomplete_after_edit();}
void ui_keyboard_set_text(const char*t){if(s->keyboard.active&&s->keyboard.text)set_edit_text(t?t:"");}
static void keyboard_render(void*screen,int x,int y,int use_screen,float tick);
static void keyboard_press(void*screen,void*button);
static void keyboard_submit(void*screen);
static void close_keyboard(void*screen);
static int keyboard_on_back(void*screen,int reason);

static unsigned keyboard_flat_index(unsigned row,unsigned column){unsigned index=column,i;for(i=0;i<row;i++)index+=keyboard_row_counts[i];return index;}
static int keyboard_location(void*button,unsigned*row,unsigned*column){unsigned r,c;for(r=0;r<KEYBOARD_ROWS;r++)for(c=0;c<keyboard_row_counts[r];c++)if(s->keyboard.buttons[keyboard_row_buttons[r][c]]==button){if(row)*row=r;if(column)*column=c;return 1;}return 0;}
static int keyboard_select(void*screen,unsigned row,unsigned column){void*before=ui_screen_selected(screen);if(row>=KEYBOARD_ROWS||column>=keyboard_row_counts[row])return 0;ui_screen_set_selection(screen,keyboard_flat_index(row,column));return ui_screen_selected(screen)!=before;}
static int keyboard_spatial_navigate(void*screen,int direction){UiElementView*selected,*candidate;unsigned row,column,target_row,c,best=0;int center,distance,best_distance=0x7fffffff;if(!keyboard_location(ui_screen_selected(screen),&row,&column))return 0;if(direction==UI_NAV_LEFT)return column?keyboard_select(screen,row,column-1):0;if(direction==UI_NAV_RIGHT)return column+1<keyboard_row_counts[row]?keyboard_select(screen,row,column+1):0;if(direction==UI_NAV_UP){if(!row)return 0;target_row=row-1;}else if(direction==UI_NAV_DOWN){if(row+1>=KEYBOARD_ROWS)return 0;target_row=row+1;}else return 0;selected=(UiElementView*)s->keyboard.buttons[keyboard_row_buttons[row][column]];center=selected->x+selected->width/2;for(c=0;c<keyboard_row_counts[target_row];c++){candidate=(UiElementView*)s->keyboard.buttons[keyboard_row_buttons[target_row][c]];distance=candidate->x+candidate->width/2-center;if(distance<0)distance=-distance;if(distance<best_distance){best_distance=distance;best=c;}}return keyboard_select(screen,target_row,best);}
static void keyboard_navigate(void*screen,int direction){if(keyboard_spatial_navigate(screen,direction))ui_play_button_sound(screen);}
static void keyboard_snap(void*screen,int right){unsigned row,column;if(keyboard_location(ui_screen_selected(screen),&row,&column)&&keyboard_select(screen,row,right?keyboard_row_counts[row]-1:0))ui_play_button_sound(screen);}
static void keyboard_move(void*screen,int source,int direction){(void)source;if(direction==1)keyboard_navigate(screen,UI_NAV_UP);else if(direction==2)keyboard_navigate(screen,UI_NAV_DOWN);else if(direction==3)keyboard_navigate(screen,UI_NAV_LEFT);else if(direction==4)keyboard_navigate(screen,UI_NAV_RIGHT);}
static void keyboard_next(void*screen){keyboard_navigate(screen,UI_NAV_RIGHT);}
static void keyboard_previous(void*screen){keyboard_navigate(screen,UI_NAV_LEFT);}
static int keyboard_binding(void*screen,const char*name){u32 key=0,scratch=0;void*client=ui_screen_client(screen),*handler;int binding=-1;if(!client)return -1;handler=((ClientInputHandlerFn)SEAM_ClientInstance_getClientInputHandler)(client);if(!handler)return -1;((StrCtor)SEAM_StrCtor)(&key,name,&scratch);binding=((ClientInputLookupFn)SEAM_ClientInputHandler_lookupBinding)(handler,&key);((StrDtor)SEAM_StrDtor)(&key);return binding;}
static void keyboard_resolve_bindings(void*screen){s->keyboard.keyboard_binding_up=keyboard_binding(screen,"button.menu_up");s->keyboard.keyboard_binding_down=keyboard_binding(screen,"button.menu_down");s->keyboard.keyboard_binding_left=keyboard_binding(screen,"button.menu_left");s->keyboard.keyboard_binding_right=keyboard_binding(screen,"button.menu_right");s->keyboard.keyboard_binding_ok=keyboard_binding(screen,"button.menu_ok");s->keyboard.keyboard_binding_select=keyboard_binding(screen,"button.controller_select");s->keyboard.keyboard_binding_cancel=keyboard_binding(screen,"button.menu_cancel");s->keyboard.keyboard_binding_clear=keyboard_binding(screen,"button.menu_clear");s->keyboard.keyboard_binding_mode=keyboard_binding(screen,"button.controller_secondary_select");s->keyboard.keyboard_binding_done=keyboard_binding(screen,"button.pause");s->keyboard.keyboard_binding_snap_left=keyboard_binding(screen,"button.inventory_left");s->keyboard.keyboard_binding_snap_right=keyboard_binding(screen,"button.inventory_right");s->keyboard.keyboard_binding_cursor_left=keyboard_binding(screen,"button.menu_tab_left");s->keyboard.keyboard_binding_cursor_right=keyboard_binding(screen,"button.menu_tab_right");}
static void keyboard_toggle_caps(void*screen){s->keyboard.page=0;s->keyboard.caps=(s->keyboard.caps+1)%3;keyboard_labels(screen);}
static void keyboard_cycle_caps_binding(void*screen){s->keyboard.page=0;if(s->keyboard.caps<2){s->keyboard.caps++;keyboard_labels(screen);}else close_keyboard(screen);}
static void keyboard_change_page(void*screen){s->keyboard.page=s->keyboard.page?0:1;s->keyboard.caps=0;keyboard_labels(screen);}
static void keyboard_mapped_button(void*screen,int binding){if(binding>=0&&binding==s->keyboard.keyboard_binding_up){keyboard_navigate(screen,UI_NAV_UP);return;}if(binding>=0&&binding==s->keyboard.keyboard_binding_down){keyboard_navigate(screen,UI_NAV_DOWN);return;}if(binding>=0&&binding==s->keyboard.keyboard_binding_left){keyboard_navigate(screen,UI_NAV_LEFT);return;}if(binding>=0&&binding==s->keyboard.keyboard_binding_right){keyboard_navigate(screen,UI_NAV_RIGHT);return;}if(binding>=0&&binding==s->keyboard.keyboard_binding_ok){ui_play_button_sound(screen);keyboard_press(screen,ui_screen_selected(screen));return;}if(binding>=0&&binding==s->keyboard.keyboard_binding_select){ui_play_button_sound(screen);close_keyboard(screen);return;}if(binding>=0&&binding==s->keyboard.keyboard_binding_cancel){ui_play_button_sound(screen);backspace();return;}if(binding>=0&&binding==s->keyboard.keyboard_binding_clear){ui_play_button_sound(screen);keyboard_cycle_caps_binding(screen);return;}if(binding>=0&&binding==s->keyboard.keyboard_binding_mode){ui_play_button_sound(screen);keyboard_change_page(screen);return;}if(binding>=0&&binding==s->keyboard.keyboard_binding_done){ui_play_button_sound(screen);keyboard_submit(screen);return;}if(binding>=0&&binding==s->keyboard.keyboard_binding_snap_left){notify_action(screen,UI_KEYBOARD_PAGE_PREV);return;}if(binding>=0&&binding==s->keyboard.keyboard_binding_snap_right){notify_action(screen,UI_KEYBOARD_PAGE_NEXT);return;}((void(*)(void*,int))SEAM_Screen_handleMappedButton)(screen,binding);}

static void keyboard_closed(void*screen){(void)screen;s->keyboard.active=s->keyboard.keyboard_closing=0;if(!submitted)notify_action(screen,UI_KEYBOARD_CANCEL);}
static void close_keyboard(void*screen){if(screen!=s->keyboard.keyboard_screen||s->keyboard.keyboard_closing)return;s->keyboard.keyboard_closing=1;ui_screen_close(screen);}
static void keyboard_setup(void*screen){unsigned i,row;int x,y,refresh=s->keyboard.caps||s->keyboard.page||s->keyboard.keyboard_cache_font!=keyboard_profile_font(screen);s->keyboard.caps=s->keyboard.page=s->keyboard.blink=0;s->keyboard.cursor=s->keyboard.length;s->keyboard.keyboard_focus_button=0;s->keyboard.keyboard_repeat_action=s->keyboard.keyboard_repeat_ticks=0;ui_mesh_cache_reset(&s->keyboard.keyboard_focus_background);keyboard_resolve_bindings(screen);if(s->keyboard.count){ui_screen_set_selection(screen,0);if(refresh)keyboard_labels(screen);return;}s->keyboard.keyboard_run_count=0;s->keyboard.keyboard_cache_font=0;
    for(i=0;i<KEYS;i++){if(i<10){x=KB_X+(int)i*KB_STEP;y=51;}else if(i<20){x=KB_X+(int)(i-10)*KB_STEP;y=81;}else if(i<29){x=26+(int)(i-20)*KB_STEP;y=111;}else{x=56+(int)(i-29)*KB_STEP;y=141;}s->keyboard.buttons[s->keyboard.count++]=ui_create_button(screen,KEY0+(int)i,x,y,KB_BUTTON,KB_BUTTON,lower[i],1,0x80);}
    s->keyboard.buttons[s->keyboard.count++]=ui_create_button(screen,CAPS,KB_X,141,KB_ONE_HALF,KB_BUTTON,"",1,0x80);
    s->keyboard.buttons[s->keyboard.count++]=ui_create_button(screen,DEL,266,141,KB_ONE_HALF,KB_BUTTON,"",1,0x80);
    s->keyboard.buttons[s->keyboard.count++]=ui_create_button(screen,MODE,KB_X,171,KB_BUTTON,KB_BUTTON,"!#1",1,0x80);
    s->keyboard.buttons[s->keyboard.count++]=ui_create_button(screen,SLASH,41,171,KB_BUTTON,KB_BUTTON,"/",1,0x80);
    s->keyboard.buttons[s->keyboard.count++]=ui_create_button(screen,SPACE,101,171,118,KB_BUTTON,"Space",1,0x80);
    s->keyboard.buttons[s->keyboard.count++]=ui_create_button(screen,PERIOD,221,171,KB_BUTTON,KB_BUTTON,".",1,0x80);
    s->keyboard.buttons[s->keyboard.count++]=ui_create_button(screen,AT_SIGN,71,171,KB_BUTTON,KB_BUTTON,"@",1,0x80);
    s->keyboard.buttons[s->keyboard.count++]=ui_create_button(screen,TILDE,251,171,KB_BUTTON,KB_BUTTON,"~",1,0x80);
    s->keyboard.buttons[s->keyboard.count++]=ui_create_button(screen,CARET,281,171,KB_BUTTON,KB_BUTTON,"^",1,0x80);
    s->keyboard.buttons[s->keyboard.count++]=ui_create_button(screen,BACK,KB_X,201,148,KB_BUTTON,"Back",1,0x80);
    s->keyboard.buttons[s->keyboard.count++]=ui_create_button(screen,SEND,161,201,148,KB_BUTTON,"Send",1,0x80);
    s->keyboard.buttons[s->keyboard.count++]=ui_create_button(screen,TAB,KB_X,11,KB_ONE_HALF,KB_BUTTON,"Tab",1,0x80);
    s->keyboard.buttons[s->keyboard.count++]=ui_create_button(screen,COMMAND_KEY,56,11,KB_ONE_HALF,KB_BUTTON,"Cmd",1,0x80);
    s->keyboard.buttons[s->keyboard.count++]=ui_create_button(screen,LEFT,191,11,KB_BUTTON,KB_BUTTON,"",1,0x80);
    s->keyboard.buttons[s->keyboard.count++]=ui_create_button(screen,UP,221,11,KB_BUTTON,KB_BUTTON,"",1,0x80);
    s->keyboard.buttons[s->keyboard.count++]=ui_create_button(screen,DOWN,251,11,KB_BUTTON,KB_BUTTON,"",1,0x80);
    s->keyboard.buttons[s->keyboard.count++]=ui_create_button(screen,RIGHT,281,11,KB_BUTTON,KB_BUTTON,"",1,0x80);
    ui_element_set_visible(s->keyboard.buttons[K_TAB],(capabilities&UI_KEYBOARD_ALLOW_TAB)!=0);ui_element_set_visible(s->keyboard.buttons[K_CMD],(capabilities&UI_KEYBOARD_ALLOW_COMMAND)!=0);for(i=0;i<s->keyboard.count;i++)ui_button_set_manual_render(s->keyboard.buttons[i],1);for(row=0;row<KEYBOARD_ROWS;row++)for(i=0;i<keyboard_row_counts[row];i++)ui_screen_add_selectable(screen,s->keyboard.buttons[keyboard_row_buttons[row][i]]);keyboard_build_symbols(screen);ui_screen_set_selection(screen,0);keyboard_labels(screen);
}
static void open_keyboard(void*hud_screen){
    static const UiCustomScreenHooks hooks={
        .setup=keyboard_setup,
        .render=keyboard_render,
        .press=keyboard_press,
        .on_back=keyboard_on_back,
        .closed=keyboard_closed,
        .mapped=keyboard_mapped_button,
        .move=keyboard_move,
        .next=keyboard_next,
        .previous=keyboard_previous
    };
    UiShared shared;
    void*game=ui_screen_game(hud_screen),*client=ui_screen_client(hud_screen);
    if(!client&&game){
        void*clients=*(void**)((u8*)game+0xC0);
        client=clients?*(void**)clients:0;
    }
    if(s->keyboard.active||!game||!client)return;
    shared.object=s->keyboard.keyboard_screen;
    shared.control=s->keyboard.keyboard_control;
    if(!ui_custom_screen_push(game,client,&s->keyboard.keyboard_vtable,&hooks,&shared))return;
    s->keyboard.keyboard_screen=shared.object;
    s->keyboard.keyboard_control=shared.control;
    s->keyboard.active=1;
    s->keyboard.keyboard_closing=0;
}
static void draw_keyboard(void*screen,int x,int y){static const Color base={0.545098f,0.545098f,0.545098f,1.0f},white={1.0f,1.0f,1.0f,1.0f};const Color*label_color=s->keyboard.buttons[0]?&((UiButtonView*)s->keyboard.buttons[0])->normal_label:&white;void*pressed=((UiScreenView*)screen)->dragging.object,*selected=ui_screen_selected(screen);unsigned i,visible_count=0;void*visible[KEYS+CONTROLS];(void)x;(void)y;if(!s->keyboard.keyboard_symbols[0])keyboard_build_symbols(screen);if(s->keyboard.keyboard_cache_font!=keyboard_profile_font(screen))keyboard_labels(screen);ui_fill(screen,0,0,320,240,&base);for(i=0;i<s->keyboard.count;i++){if(i==K_TAB&&!(capabilities&UI_KEYBOARD_ALLOW_TAB))continue;if(i==K_CMD&&!(capabilities&UI_KEYBOARD_ALLOW_COMMAND))continue;visible[visible_count++]=s->keyboard.buttons[i];}ui_buttons_draw_native_cached(&s->keyboard.keyboard_background,visible,visible_count,pressed);if(pressed!=s->keyboard.keyboard_focus_button){ui_mesh_cache_reset(&s->keyboard.keyboard_focus_background);s->keyboard.keyboard_focus_button=pressed;}if(pressed)ui_button_draw_background_cached(&s->keyboard.keyboard_focus_background,pressed,1);if(selected&&selected!=pressed)ui_button_draw_highlight(screen,selected);keyboard_label_mesh_draw(label_color);keyboard_draw_symbols();}
enum { REPEAT_NONE=0, REPEAT_DELETE, REPEAT_LEFT, REPEAT_RIGHT, REPEAT_UP, REPEAT_DOWN, REPEAT_DELAY=15, REPEAT_RATE=3 };
static void keyboard_repeat_action(void*screen,u32 action){if(action==REPEAT_DELETE)backspace();else if(action==REPEAT_LEFT)keyboard_navigate(screen,UI_NAV_LEFT);else if(action==REPEAT_RIGHT)keyboard_navigate(screen,UI_NAV_RIGHT);else if(action==REPEAT_UP)keyboard_navigate(screen,UI_NAV_UP);else if(action==REPEAT_DOWN)keyboard_navigate(screen,UI_NAV_DOWN);}
static void keyboard_poll_controller(void*screen){u32 hold=0,trigger=0,action=REPEAT_NONE;if(!keyboard_input_sample(&hold,&trigger))return;if(trigger&KEYBOARD_PAD_START)keyboard_submit(screen);else if(trigger&KEYBOARD_PAD_SELECT)close_keyboard(screen);if(s->keyboard.keyboard_closing)return;if(hold&KEYBOARD_PAD_B)action=REPEAT_DELETE;else if(hold&KEYBOARD_PAD_LEFT)action=REPEAT_LEFT;else if(hold&KEYBOARD_PAD_RIGHT)action=REPEAT_RIGHT;else if(hold&KEYBOARD_PAD_UP)action=REPEAT_UP;else if(hold&KEYBOARD_PAD_DOWN)action=REPEAT_DOWN;else if(((UiScreenView*)screen)->dragging.object==s->keyboard.buttons[K_DEL])action=REPEAT_DELETE;if(action!=s->keyboard.keyboard_repeat_action){s->keyboard.keyboard_repeat_action=action;s->keyboard.keyboard_repeat_ticks=0;return;}if(action==REPEAT_NONE)return;s->keyboard.keyboard_repeat_ticks++;if(s->keyboard.keyboard_repeat_ticks>=REPEAT_DELAY&&((s->keyboard.keyboard_repeat_ticks-REPEAT_DELAY)%REPEAT_RATE)==0)keyboard_repeat_action(screen,action);}
static void keyboard_render(void*screen,int x,int y,int use_screen,float tick){static const Color bg={0.18f,0.18f,0.18f,1},fg={1,1,1,1};(void)tick;if(use_screen&0x40)notify_action(screen,UI_KEYBOARD_DRAW);if(use_screen&0x80){keyboard_poll_controller(screen);if(!s->keyboard.keyboard_closing)draw_keyboard(screen,x,y);}}
static int is_keyboard_button(void*button){unsigned i;for(i=0;i<s->keyboard.count;i++)if(s->keyboard.buttons[i]==button)return 1;return 0;}
static int keyboard_on_back(void*screen,int reason){(void)screen;(void)reason;backspace();return 1;}
static void consume_shift(void*screen){if(!s->keyboard.page&&s->keyboard.caps==1){s->keyboard.caps=0;keyboard_labels(screen);}}
static void keyboard_submit(void*screen){if(s->keyboard.keyboard_closing)return;close_keyboard(screen);submitted=1;notify_action(screen,UI_KEYBOARD_SUBMIT);}

static void keyboard_press(void*screen,void*button){
    int id;unsigned i;const char*key;
    if(!s->keyboard.active||screen!=s->keyboard.keyboard_screen||!is_keyboard_button(button))return;
    id=ui_button_id(button);
    if(id>=KEY0&&id<KEY0+KEYS){
        i=(unsigned)(id-KEY0);key=key_label(i);
        if(key&&key[0]){insert_char(key[0]);consume_shift(screen);}
    }
    else if(id==CAPS){
        if(!s->keyboard.page)keyboard_toggle_caps(screen);
        else{s->keyboard.page=s->keyboard.page==1?2:1;keyboard_labels(screen);}
    }
    else if(id==MODE)keyboard_change_page(screen);
    else if(id==DEL)backspace();
    else if(id==SPACE){insert_char(' ');consume_shift(screen);}
    else if(id==SLASH){insert_char('/');consume_shift(screen);}
    else if(id==PERIOD){insert_char('.');consume_shift(screen);}
    else if(id==AT_SIGN){insert_char('@');consume_shift(screen);}
    else if(id==TILDE){insert_char('~');consume_shift(screen);}
    else if(id==CARET){insert_char('^');consume_shift(screen);}
    else if(id==TAB&&(capabilities&UI_KEYBOARD_ALLOW_TAB))notify_action(screen,UI_KEYBOARD_TAB);
    else if(id==COMMAND_KEY&&(capabilities&UI_KEYBOARD_ALLOW_COMMAND))notify_action(screen,UI_KEYBOARD_COMMAND);
    else if(id==LEFT){if(s->keyboard.cursor)s->keyboard.cursor--;s->keyboard.blink=0;autocomplete_after_edit();}
    else if(id==RIGHT){if(s->keyboard.cursor<s->keyboard.length)s->keyboard.cursor++;s->keyboard.blink=0;autocomplete_after_edit();}
    else if(id==UP)notify_action(screen,UI_KEYBOARD_UP);
    else if(id==DOWN)notify_action(screen,UI_KEYBOARD_DOWN);
    else if(id==SEND)keyboard_submit(screen);
    else if(id==BACK)close_keyboard(screen);
}

void ui_keyboard_reset(void){
    UiShared held={s->keyboard.keyboard_screen,s->keyboard.keyboard_control};unsigned i;
    if(s->keyboard.active&&!submitted)notify_action(s->keyboard.keyboard_screen,UI_KEYBOARD_CANCEL);
    keyboard_label_mesh_reset();keyboard_destroy_symbols();ui_mesh_cache_reset(&s->keyboard.keyboard_background);ui_mesh_cache_reset(&s->keyboard.keyboard_focus_background);
    for(i=0;i<KEYBOARD_LABEL_RUNS;i++)ui_cached_text_reset(&s->keyboard.keyboard_runs[i]);
    action_callback=0;ui_shared_release(&held);s->keyboard.keyboard_screen=s->keyboard.keyboard_control=s->keyboard.keyboard_focus_button=0;
    s->keyboard.count=s->keyboard.active=s->keyboard.keyboard_closing=s->keyboard.caps=s->keyboard.page=s->keyboard.length=s->keyboard.cursor=s->keyboard.blink=s->keyboard.keyboard_run_count=0;s->keyboard.keyboard_cache_font=0;
    if(s->keyboard.text)s->host.heap_free(s->keyboard.text);s->keyboard.text=0;action_callback=0;
}
int ui_keyboard_open(void*owner,const char*initial,unsigned limit,unsigned flags,UiKeyboardAction callback){
    unsigned length=text_len(initial?initial:""),i,n=0;char*buffer;
    if(s->keyboard.active||!limit||length>limit||!callback)return 0;
    ui_keyboard_reset();
    buffer=s->host.heap_alloc(limit+1);if(!buffer)return 0;copy_text(buffer,initial?initial:"",limit);
    s->keyboard.text=buffer;s->keyboard.length=s->keyboard.cursor=length;s->keyboard.input_limit=limit;capabilities=flags;submitted=0;action_callback=callback;
    if(flags&UI_KEYBOARD_ALLOW_TAB)keyboard_row_buttons[0][n++]=K_TAB;
    if(flags&UI_KEYBOARD_ALLOW_COMMAND)keyboard_row_buttons[0][n++]=K_CMD;
    for(i=K_LEFT;i<=K_RIGHT;i++)keyboard_row_buttons[0][n++]=i;keyboard_row_counts[0]=n;
    open_keyboard(owner);if(!s->keyboard.active){ui_keyboard_reset();return 0;}return 1;
}
