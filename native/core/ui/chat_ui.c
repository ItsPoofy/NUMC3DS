#include "keyboard_ui.h"
#include "chat_ui.h"
#include "../world/world_thumbnail.h"
#include "chat_session_lifecycle.h"
#include "fps_overlay.h"
#include "command_block_screen.h"
#include "../internal.h"
#include "command_autocomplete.h"
#include "intellisense_item_renderer.h"
#include "../commands/command_conformance.h"
#include "../commands/command_test_runner.h"
#include "keyboard_input.h"
#include "quick_commands_ui.h"
#include "ui_text_layout.h"
#include "ui_widgets.h"
#include "../commands/native_registry.h"
#include "../extension.h"

static void *screen_level(void*screen){void*game=ui_screen_game(screen),*clients,*client;if(!game)return 0;clients=*(void**)((u8*)game+0xC0);client=clients?*(void**)clients:0;return client?((GetLevel)SEAM_ClientInstance_getLevel)(client):0;}
static const char*gtext(void*value);

void history_clear(void){s->history_count=0;s->chat_page=0;zero(s->history,sizeof(s->history));s->history_generation++;}
void chat_ui_flush_session(void){unsigned i;command_block_screen_reset();command_autocomplete_destroy();history_clear();s->sent_count=s->sent_pos=0;zero(s->sent_history,sizeof(s->sent_history));for(i=0;i<WRAPPED_LINES;i++)ui_cached_text_reset(&s->history_lines[i]);ui_cached_text_reset(&s->history_page_cache);zero(s->history_line_entry,sizeof(s->history_line_entry));s->history_cache_count=0;s->history_cache_font=0;s->history_cache_generation=~s->history_generation;s->session_game=0;s->session_level=0;}
void history_add(unsigned char type,const char*source,const char*message){HistoryEntry*entry;unsigned i;if(!message||!message[0])return;s->chat_page=0;if(s->history_count<HISTORY)entry=&s->history[s->history_count++];else{for(i=1;i<HISTORY;i++)cp(&s->history[i-1],&s->history[i],sizeof(HistoryEntry));entry=&s->history[HISTORY-1];}entry->type=type;entry->age=0;copy_text(entry->source,source,SOURCE_TEXT);copy_text(entry->message,message,MAX_TEXT);s->history_generation++;}
static int digit(char c){return c>='0'&&c<='9';}
static int same_digit_pattern(const char*a,const char*b){int found=0;if(!a||!b)return 0;while(*a&&*b){if(digit(*a)||digit(*b)){if(!digit(*a)||!digit(*b))return 0;found=1;while(digit(*a))a++;while(digit(*b))b++;continue;}if(*a++!=*b++)return 0;}return found&&!*a&&!*b;}
static int transient_system_key(const char*key){return streq(key,"menu.saveWarning1Minute")||streq(key,"menu.saveWarningNSeconds")||streq(key,"menu.saveWarningNMinutes");}
static void history_add_system(const char*raw,const char*message){HistoryEntry*entry;int transient=transient_system_key(raw);if(!message||!message[0])return;if(s->history_count){entry=&s->history[s->history_count-1];if(entry->type!=1&&entry->age<=120&&same_digit_pattern(entry->message,message)){entry->type=2;entry->age=0;entry->source[0]=0;copy_text(entry->message,message,MAX_TEXT);s->history_generation++;return;}}history_add((unsigned char)(transient?2:0),"",message);}
static void history_tick(void){unsigned i=0,j;while(i<s->history_count){if(s->history[i].age<=600)s->history[i].age++;if(s->history[i].type==2&&s->history[i].age>600){for(j=i+1;j<s->history_count;j++)cp(&s->history[j-1],&s->history[j],sizeof(HistoryEntry));s->history_count--;zero(&s->history[s->history_count],sizeof(HistoryEntry));s->history_generation++;continue;}i++;}}

static void apply_command_access(void*screen){void*game=ui_screen_game(screen),*player,*level;int enabled;if(!game)return;player=((GetPlayer)SEAM_Player_getPlayer)(game);level=screen_level(screen);if(!level)return;enabled=((HasCommandsEnabled)SEAM_HasCommandsEnabled)(level);if(s->force_commands&&!enabled){((SetCommandsEnabled)SEAM_SetCommandsEnabled)(level,1);enabled=1;}if(enabled&&player&&((GetPermissionsLevel)SEAM_Player_getPermissionsLevel)(player)<2)((SetPermissionsLevel)SEAM_Player_setPermissionsLevel)(player,2);s->force_commands=0;}
static void trace_command_error(const char*reason){static const char prefix[]="NuMC3DS command error: ";unsigned length=0;while(reason&&reason[length]&&length<127)length++;s->host.debug_string(prefix,sizeof(prefix)-1);s->host.debug_string(reason?reason:"",length);}

static void send_chat(void*screen){
    u32 source=0,message=0,error=0,source_scratch=0,message_scratch=0,error_scratch=0,vector[3],packet[8],result;void*game=ui_screen_game(screen),*player,*sender,*commands,*level,*memory,*origin,*saved_player,*saved_level,*saved_game;
    if(!s->keyboard.length||!game)return;
    player=((GetPlayer)SEAM_Player_getPlayer)(game);if(!player)return;
    if(s->keyboard.text[0]=='/'){
        native_command_trace('A');apply_command_access(screen);commands=native_registry_commands_for_game(game);if(!commands){native_command_trace('b');history_add(0,"","Command system is unavailable.");return;}native_command_trace('B');level=screen_level(screen);
        if(!level||!((HasCommandsEnabled)SEAM_HasCommandsEnabled)(level)){native_command_trace('c');history_add(0,"",command_unavailable);return;}
        native_command_trace('C');
        if(!native_registry_attach(commands,game)){native_command_trace('e');history_add(0,"","Command system is unavailable.");return;}native_command_trace('D');
        memory=((GameAllocWithSelector)SEAM_Heap_allocWithSelector)(0x38,(void*)SEAM_game_alloc_selector);if(!memory){native_command_trace('f');history_add(0,"",command_failed);return;}origin=((PlayerCommandOriginCtor)SEAM_ClientCommandOrigin_ctor)(memory,player);if(!origin){((void(*)(void*))SEAM_operator_delete)(memory);native_command_trace('o');history_add(0,"",command_failed);return;}native_command_trace('E');
        ((StrCtor)SEAM_StrCtor)(&message,s->keyboard.text,&message_scratch);((StrCtor)SEAM_StrCtor)(&error,"",&error_scratch);native_command_trace('F');saved_player=s->command_player;saved_level=s->command_level;saved_game=s->command_game;s->command_player=player;s->command_level=level;s->command_game=game;native_command_trace('Q');s->chat_command_active=1;command_conformance_capture_begin();result=((RequestCommandExecution)s->command_request.trampoline)(commands,&origin,&message,&error);command_conformance_capture_end();s->chat_command_active=0;native_command_trace((u8)result==1?'R':'r');s->command_player=saved_player;s->command_level=saved_level;s->command_game=saved_game;if(origin)((void(*)(void*))(*(void***)origin)[1])(origin);if((u8)result==1){unsigned msg_idx,msg_count=command_conformance_capture_count();for(msg_idx=0;msg_idx<msg_count;msg_idx++){const char*out_msg=command_conformance_capture_message(msg_idx);if(out_msg&&out_msg[0])history_add(0,"",out_msg);}}else{char*reason=0;cp(&reason,&error,sizeof(reason));trace_command_error(reason&&reason[0]?reason:"<empty>");history_add(0,"",reason&&reason[0]?reason:command_failed);}((StrDtor)SEAM_StrDtor)(&error);((StrDtor)SEAM_StrDtor)(&message);native_command_trace('S');return;
    }
    sender=*(void**)((u8*)player+SEAM_Entity_packetSenderOffset);if(!sender)return;zero(vector,sizeof(vector));zero(packet,sizeof(packet));((CopyEntityName)SEAM_CopyEntityName)(&source,player);{char*raw=0;cp(&raw,&source,sizeof(raw));if(!raw||!raw[0]){((StrDtor)SEAM_StrDtor)(&source);source=0;((StrCtor)SEAM_StrCtor)(&source,"Player",&source_scratch);}}((StrCtor)SEAM_StrCtor)(&message,s->keyboard.text,&message_scratch);((PacketCtor)SEAM_TextPacket_ctor)(packet,1,&source,&message,vector);((LoopbackSend)SEAM_LoopbackSend)(sender,packet);((PacketDtor)SEAM_TextPacket_dtor)(packet);history_add(1,gtext(&source),s->keyboard.text);((StrDtor)SEAM_StrDtor)(&message);((StrDtor)SEAM_StrDtor)(&source);s->host.debug_string(sent,text_len(sent));
}

static void sent_add(const char*t){unsigned i;if(!t||!t[0])return;if(s->sent_count<HISTORY)copy_text(s->sent_history[s->sent_count++],t,MAX_TEXT);else{for(i=1;i<HISTORY;i++)copy_text(s->sent_history[i-1],s->sent_history[i],MAX_TEXT);copy_text(s->sent_history[HISTORY-1],t,MAX_TEXT);}s->sent_pos=s->sent_count;}
static void recall_up(void){if(!s->sent_count)return;if(s->sent_pos==s->sent_count)copy_text(s->draft,s->keyboard.text,MAX_TEXT);if(s->sent_pos)s->sent_pos--;ui_keyboard_set_text(s->sent_history[s->sent_pos]);}
static void recall_down(void){if(s->sent_pos>=s->sent_count)return;if(s->sent_pos+1<s->sent_count){s->sent_pos++;ui_keyboard_set_text(s->sent_history[s->sent_pos]);}else{s->sent_pos=s->sent_count;ui_keyboard_set_text(s->draft);}}

static void chat_history_page_prev(void*screen){unsigned total_pages=s->history_cache_count>0?(s->history_cache_count+CHAT_VISIBLE_LINES-1)/CHAT_VISIBLE_LINES:1;if(total_pages>1&&s->chat_page+1<total_pages){s->chat_page++;ui_play_button_sound(screen);}}
static void chat_history_page_next(void*screen){if(s->chat_page>0){s->chat_page--;ui_play_button_sound(screen);}}
static void build_hud(void*screen){UiElementView*stock_pause;int half;void*game,*level;ui_keyboard_reset();quick_commands_reset();s->screen=screen;s->chat_page=0;s->sent_pos=s->sent_count;s->chat_input[0]=s->draft[0]=0;game=ui_screen_game(screen);level=screen_level(screen);if(s->session_level!=level){history_clear();}s->session_game=game;s->session_level=level;stock_pause=(UiElementView*)ui_ingame_pause_button(screen);s->chat=s->pause=0;if(stock_pause){half=stock_pause->width/2;ui_element_set_visible(stock_pause,0);s->chat=ui_create_hud_button(screen,CHAT,stock_pause->x,stock_pause->y,half,stock_pause->height,1,1);s->pause=ui_create_hud_button(screen,2,stock_pause->x+half,stock_pause->y,stock_pause->width-half,stock_pause->height,1,0);if(s->chat)ui_button_set_manual_render(s->chat,1);if(s->pause)ui_button_set_manual_render(s->pause,1);}}
static void after_setup(void*screen){((ThisFn)s->setup.trampoline)(screen);if(!ui_resources_ready())return;build_hud(screen);apply_command_access(screen);}

static void history_line(const HistoryEntry*entry,char*out){unsigned n=0,i=0;if(entry->type==1&&entry->source[0]){out[n++]='<';while(entry->source[i]&&n<150)out[n++]=entry->source[i++];if(n<150)out[n++]='>';if(n<150)out[n++]=' ';}i=0;while(entry->message[i]&&n<150)out[n++]=entry->message[i++];out[n]=0;}
static unsigned wrap_text(void*screen,const char*source,unsigned count,u8 entry_index){unsigned pos=0,next;char line[152];void*font=ui_screen_font(screen);while(source[pos]&&count<WRAPPED_LINES){if(!ui_text_wrap_line(font,source,pos,390,line,sizeof(line),&next)||next<=pos)break;ui_cached_text_set(screen,&s->history_lines[count],line);s->history_line_entry[count]=entry_index;count++;pos=next;}return count;}
static void rebuild_history_cache(void*screen){unsigned i,count=0;char line[152];if(s->history_cache_generation==s->history_generation&&s->history_cache_font==ui_screen_font(screen))return;for(i=0;i<s->history_count&&count<WRAPPED_LINES;i++){history_line(&s->history[i],line);count=wrap_text(screen,line,count,(u8)i);}s->history_cache_count=count;s->history_cache_generation=s->history_generation;s->history_cache_font=ui_screen_font(screen);}
static void draw_history(void*screen,int open){
    static const Color background={0.0f,0.0f,0.0f,0.50f},white={1.0f,1.0f,1.0f,1.0f},gold={1.0f,0.85f,0.2f,1.0f};
    unsigned begin=0,show,i,minimum_entry=0,total_pages;
    int y,last_y,max_width=0;
    rebuild_history_cache(screen);
    if(!s->history_cache_count){
        if(!open)return;
        total_pages=1;
        show=0;
    }else{
        total_pages=(s->history_cache_count+CHAT_VISIBLE_LINES-1)/CHAT_VISIBLE_LINES;
        if(!total_pages)total_pages=1;
        if(!open){
            minimum_entry=s->history_count;
            for(i=0;i<s->history_count;i++)if(s->history[i].age<=600){minimum_entry=i;break;}
            while(begin<s->history_cache_count&&s->history_line_entry[begin]<minimum_entry)begin++;
            show=s->history_cache_count-begin;
            if(!show)return;
            if(show>CHAT_VISIBLE_LINES){begin=s->history_cache_count-CHAT_VISIBLE_LINES;show=CHAT_VISIBLE_LINES;}
        }else{
            unsigned end;
            if(s->chat_page>=total_pages)s->chat_page=total_pages-1;
            end=s->history_cache_count-s->chat_page*CHAT_VISIBLE_LINES;
            begin=end>CHAT_VISIBLE_LINES?end-CHAT_VISIBLE_LINES:0;
            show=end-begin;
        }
    }
    y=198-(int)(show?show-1:0)*17;
    last_y=y+(int)(show?show-1:0)*17;
    if(!open){
        for(i=begin;i<begin+show;i++)if(s->history_lines[i].width>max_width)max_width=s->history_lines[i].width;
        if(max_width>390)max_width=390;
        ui_fill(screen,0,y-3,max_width+10>400?400:max_width+10,last_y+10,&background);
    }else{
        char page_str[64];unsigned plen=0;float page_x;
        page_str[0]=0;
        append(page_str,&plen,"Page ");
        append_int(page_str,&plen,(int)(total_pages-s->chat_page));
        append(page_str,&plen,"/");
        append_int(page_str,&plen,(int)total_pages);
        ui_cached_text_set(screen,&s->history_page_cache,page_str);
        page_x=(400.0f-(float)s->history_page_cache.width)/2.0f;
        ui_cached_text_draw(screen,&s->history_page_cache,page_x,5.0f,1.0f,&gold);
    }
    for(i=begin;i<begin+show;i++,y+=17)ui_cached_text_draw(screen,&s->history_lines[i],5.0f,(float)y,1.0f,&white);
}

static void draw_autocomplete(void*screen){
    static const Color white={1.0f,1.0f,1.0f,1.0f},gray={0.62f,0.62f,0.62f,1.0f};
    unsigned count=command_autocomplete_count(),selection=command_autocomplete_selected(),scroll=command_autocomplete_scroll();
    unsigned row,index,rows,match_start,match_length,option_length;int y;float text_x,match_x,rest_x;char before[AUTOCOMPLETE_TEXT],match[AUTOCOMPLETE_TEXT],rest[AUTOCOMPLETE_TEXT];
    const char*option;const void*item;
    if(!count)return;
    if(selection<scroll)scroll=selection;
    if(selection>=scroll+AUTOCOMPLETE_VISIBLE_LINES)scroll=selection-(AUTOCOMPLETE_VISIBLE_LINES-1);
    if(scroll+AUTOCOMPLETE_VISIBLE_LINES>count)scroll=count>AUTOCOMPLETE_VISIBLE_LINES?count-AUTOCOMPLETE_VISIBLE_LINES:0;
    command_autocomplete_set_scroll(scroll);
    rows=count-scroll;
    if(rows>AUTOCOMPLETE_VISIBLE_LINES)rows=AUTOCOMPLETE_VISIBLE_LINES;
    for(row=0;row<rows;row++){
        index=scroll+row;option=command_autocomplete_option(index);option_length=text_len(option);match_start=command_autocomplete_match_start(index);match_length=command_autocomplete_match_length(index);if(match_start>option_length)match_start=option_length;if(match_length>option_length-match_start)match_length=option_length-match_start;if(match_start>=sizeof(before))match_start=sizeof(before)-1;if(match_length>=sizeof(match))match_length=sizeof(match)-1;
        cp(before,option,match_start);before[match_start]=0;cp(match,option+match_start,match_length);match[match_length]=0;copy_text(rest,option+match_start+match_length,sizeof(rest)-1);
        ui_cached_text_set(screen,&s->autocomplete_lines[row],before);
        ui_cached_text_set(screen,&s->autocomplete_match_lines[row],match);
        ui_cached_text_set(screen,&s->autocomplete_rest_lines[row],rest);
        y=198-(int)(rows-1-row)*17;
        item=command_autocomplete_item(index);text_x=item?22.0f:5.0f;if(item)intellisense_item_renderer_draw(item,12.0f,(float)y);
        if(before[0])ui_cached_text_draw(screen,&s->autocomplete_lines[row],text_x,(float)y,1.0f,&gray);
        match_x=text_x+(float)s->autocomplete_lines[row].width;
        if(match[0])ui_cached_text_draw(screen,&s->autocomplete_match_lines[row],match_x,(float)y,1.0f,&white);
        rest_x=match_x+(float)s->autocomplete_match_lines[row].width;
        if(rest[0])ui_cached_text_draw(screen,&s->autocomplete_rest_lines[row],rest_x,(float)y,1.0f,&gray);
    }
}

static void draw_title(void*screen){static const Color white={1.0f,1.0f,1.0f,1.0f},gray={0.7f,0.7f,0.7f,1.0f};u32 stay,fade,elapsed;float alpha=1.0f,scale=3.0f,sub_scale=1.5f;Color title_color=white,subtitle_color=gray;if(!s->title_ticks)return;stay=s->title_stay?s->title_stay:70;fade=s->title_stay>20?10:2;elapsed=(stay+fade*2)>s->title_ticks?0:(stay+fade*2-s->title_ticks);if(elapsed<fade)alpha=(float)(elapsed+1)/(float)fade;else if(s->title_ticks<fade)alpha=(float)s->title_ticks/(float)fade;if(alpha>1.0f)alpha=1.0f;if(alpha<0.0f)alpha=0.0f;title_color.a=subtitle_color.a=alpha;ui_cached_text_set(screen,&s->title_cache,s->title_text);ui_cached_text_set(screen,&s->subtitle_cache,s->subtitle_text);if(s->title_text[0])ui_cached_text_draw(screen,&s->title_cache,(400.0f-(float)s->title_cache.width*scale)/2.0f,40.0f,scale,&title_color);if(s->subtitle_text[0])ui_cached_text_draw(screen,&s->subtitle_cache,(400.0f-(float)s->subtitle_cache.width*sub_scale)/2.0f,90.0f,sub_scale,&subtitle_color);s->title_ticks--;}
static unsigned build_input_slice(char*out,unsigned start,int show){unsigned i=start,n=0;while(i<=s->keyboard.length&&n<62){if(i==s->keyboard.cursor)out[n++]=show?'|':' ';if(i==s->keyboard.length)break;out[n++]=s->keyboard.text[i++];}out[n]=0;return n;}
static void input_preview(void*screen){unsigned start=s->keyboard.cursor>58?s->keyboard.cursor-58:0,n;int show=((s->keyboard.blink++/30)&1)==0;char line[64];n=build_input_slice(line,start,show);ui_cached_text_set(screen,&s->input_cache,line);while(start<s->keyboard.cursor&&s->input_cache.width>388){start++;n=build_input_slice(line,start,show);ui_cached_text_set(screen,&s->input_cache,line);}while(n>1&&s->input_cache.width>388){line[--n]=0;ui_cached_text_set(screen,&s->input_cache,line);}}

static void chat_keyboard_action(void*screen,int action){
    if(action==UI_KEYBOARD_SUBMIT){sent_add(s->keyboard.text);send_chat(screen);s->chat_input[0]=0;}
    else if(action==UI_KEYBOARD_CANCEL)copy_text(s->chat_input,s->keyboard.text,MAX_TEXT);
    else if(action==UI_KEYBOARD_CHANGED){if(s->keyboard.text[0]=='/')command_autocomplete_update(screen);else command_autocomplete_reset();}
    else if(action==UI_KEYBOARD_TAB&&s->keyboard.text[0]=='/')command_autocomplete_apply(screen);
    else if(action==UI_KEYBOARD_COMMAND)quick_commands_open(screen);
    else if(action==UI_KEYBOARD_UP)recall_up();
    else if(action==UI_KEYBOARD_DOWN)recall_down();
    else if(action==UI_KEYBOARD_PAGE_PREV)chat_history_page_prev(screen);
    else if(action==UI_KEYBOARD_PAGE_NEXT)chat_history_page_next(screen);
}
static int command_keys_allowed(void*screen){void*game=ui_screen_game(screen),*level=screen_level(screen),*player;if(!game||!level||!((HasCommandsEnabled)SEAM_HasCommandsEnabled)(level))return 0;player=((GetPlayer)SEAM_Player_getPlayer)(game);return player&&((GetPermissionsLevel)SEAM_Player_getPermissionsLevel)(player)>=2;}
static void open_keyboard(void*screen){unsigned flags=command_keys_allowed(screen)?UI_KEYBOARD_ALLOW_TAB|UI_KEYBOARD_ALLOW_COMMAND:0;s->sent_pos=s->sent_count;copy_text(s->draft,s->chat_input,MAX_TEXT);s->chat_page=0;command_autocomplete_reset();ui_keyboard_open(screen,s->chat_input,MAX_TEXT,flags,chat_keyboard_action);}
void chat_ui_set_input(const char*text){ui_keyboard_set_text(text);}
static void render_content(void*screen,int x,int y,int use_screen,float tick){static const Color overlay={0.0f,0.0f,0.0f,0.50f},white={1.0f,1.0f,1.0f,1.0f};u32 trigger=0;int hud_ready=ui_ingame_hud_ready(screen);if(!ui_resources_ready()){((void(*)(void*,int,int,int,float))s->render.trampoline)(screen,x,y,use_screen,tick);return;}if(use_screen==0x40)history_tick();if(!s->keyboard.active){if(use_screen==0x80&&hud_ready&&keyboard_input_sample(0,&trigger)&&(trigger&KEYBOARD_PAD_SELECT))open_keyboard(screen);((void(*)(void*,int,int,int,float))s->render.trampoline)(screen,x,y,use_screen,tick);if(use_screen==0x40){if(hud_ready){draw_title(screen);draw_history(screen,0);}fps_overlay_render(screen);}else if(use_screen==0x80){if(hud_ready){ui_button_draw_native(screen,s->chat,x,y);ui_button_draw_native(screen,s->pause,x,y);}}}else if(use_screen==0x40){ui_fill(screen,0,0,400,240,&overlay);if(command_autocomplete_active())draw_autocomplete(screen);else draw_history(screen,1);input_preview(screen);ui_cached_text_draw(screen,&s->input_cache,5.0f,225.0f,1.0f,&white);fps_overlay_render(screen);}if(use_screen==0x40){command_testall_tick();
if(hud_ready)numc3ds_extension_hud_tick(screen);
}}
static void after_render(void*screen,int x,int y,int use_screen,float tick){if(wt_thumbnail_capture_active())return;render_content(screen,x,y,use_screen,tick);}


static int handle_marker(const char*source,const char*message){const char*q;int value;if(!streq(source,"NuMC3DS"))return 0;if(message[0]=='R'&&message[1]=='|'){history_add(0,"",message+2);return 1;}if(message[0]=='T'&&message[1]=='|'){title_apply_local("title",message+2);return 1;}if(message[0]=='U'&&message[1]=='|'){title_apply_local("subtitle",message+2);return 1;}if(message[0]=='X'&&message[1]=='|'){title_apply_local("clear","");return 1;}if(message[0]=='P'&&message[1]=='|'){u32 hash;float volume=1.0f,pitch=1.0f;q=message+2;if(integer(&q,&value))hash=(u32)value;else return 1;if(integer(&q,&value)&&value>0)volume=(float)value;if(integer(&q,&value)&&value>0)pitch=(float)value/100.0f;if(s->session_game)((GamePlaySoundFn)SEAM_MinecraftGame_playSound)(s->session_game,hash,volume,pitch);return 1;}return 1;}
static const char*gtext(void*value){const char*text="";if(value)cp(&text,value,sizeof(text));return text?text:"";}
static void on_gui_entry(void*gui,void*entry,u32 param3,u32 param4){const char*from=gtext((u8*)entry+12),*text=gtext((u8*)entry+8),*formatted=gtext((u8*)entry+16);(void)gui;(void)param3;(void)param4;if(!handle_marker(from,text))history_add_system(text,formatted[0]?formatted:text);}
static void on_system_message(void*screen,void*message,int translate){u32 localized=0;const char*raw=gtext(message);(void)screen;if(translate){((LocalizationGetFn)SEAM_Localization_get)(&localized,message,0);history_add_system(raw,gtext(&localized));((StrDtor)SEAM_StrDtor)(&localized);}else history_add_system(raw,raw);}
static void on_press(void*screen,void*button){if(!button||!ui_ingame_hud_ready(screen))return;if(button==s->chat){if(!s->keyboard.active){ui_play_button_sound(screen);open_keyboard(screen);}return;}if(button==s->pause){if(!s->keyboard.active)((PressFn)s->pressed.trampoline)(screen,button);return;}if(!s->keyboard.active)((PressFn)s->pressed.trampoline)(screen,button);}
static void on_chat_action(void*client){(void)client;if(!s->keyboard.active&&s->screen&&ui_ingame_hud_ready(s->screen)){ui_play_button_sound(s->screen);open_keyboard(s->screen);}}

static int install(NuMC3DS_Hook*hook,u32 target,u32 replacement,u32 first,u32 second,int code){hook->target=target;hook->replacement=replacement;hook->expected[0]=first;hook->expected[1]=second;return s->host.install_hook(hook)?code:0;}
int chat_ui_install_hooks(void){int result;if((result=ui_install_button_color_hook()))return result;if((result=install(&s->setup,SEAM_InGamePlayScreen_setup,(u32)after_setup,0xE92D41F0u,0xE1A04000u,-4)))return result;if((result=install(&s->pressed,SEAM_InGamePlayScreen_buttonPressed,(u32)on_press,0xE92D4070u,0xE1A04000u,-5)))return result;if((result=install(&s->render,SEAM_InGamePlayScreen_render,(u32)after_render,0xE92D4FF0u,0xE3530040u,-6)))return result;if((result=install(&s->gui_entry,SEAM_GuiData_appendMessageEntry,(u32)on_gui_entry,0xE92D4FF0u,0xE1A04001u,-7)))return result;if((result=install(&s->system_message,SEAM_SystemMessagesScreen_pushMessage,(u32)on_system_message,0xE92D4FF0u,0xE1A04000u,-9)))return result;
    if((result=chat_session_lifecycle_install_hook()))return result;
    return install(&s->chat_action,SEAM_MinecraftInputHandler_handleChatAction,(u32)on_chat_action,0xE92D4010u,0xE1A04000u,-8);
}
