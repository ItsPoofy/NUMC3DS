#include "text_entry_view.h"
#include "ui_form_style.h"
#include "ui_text_layout.h"
#include "ui_localize.h"
#include "text_entry_session.h"
static UiCachedText lines[15],title;
static UiFormHeader header;
static u32 blink_start;
static unsigned last_cursor,last_length;
void ui_text_entry_view_reset(void){unsigned i;for(i=0;i<15;i++)ui_cached_text_reset(&lines[i]);ui_cached_text_reset(&title);ui_form_header_reset(&header);blink_start=0;}
void ui_text_entry_draw(void *screen){
    char line[152],label[64];unsigned at=0,next=0,row=0,start=0,cursor_line=0,i;int caret_x=12,caret_y=42;
    ui_localized_label(label,sizeof(label),ui_text_entry_title(),"");ui_cached_text_set(screen,&title,label);
    ui_fill(screen,0,0,400,240,&ui_form_background);ui_form_header(screen,400,&title,&header);ui_form_field(screen,6,36,388,198,1);
    while(ui_text_wrap_input_line(ui_screen_font(screen),s->keyboard.text,at,372,line,sizeof(line),&next)){
        if(s->keyboard.cursor<next||next>=s->keyboard.length){cursor_line=row;break;}at=next;row++;
    }
    at=0;row=0;start=cursor_line>=15?cursor_line-14:0;
    while(row<start&&ui_text_wrap_input_line(ui_screen_font(screen),s->keyboard.text,at,372,line,sizeof(line),&next)){at=next;row++;}
    for(i=0;i<15;i++){
        unsigned length=ui_text_wrap_input_line(ui_screen_font(screen),s->keyboard.text,at,372,line,sizeof(line),&next);
        ui_cached_text_set(screen,&lines[i],line);ui_cached_text_draw(screen,&lines[i],12,42+i*12,1,&ui_form_white);
        if(s->keyboard.cursor>=at&&(s->keyboard.cursor<next||next>=s->keyboard.length)){
            unsigned prefix=s->keyboard.cursor-at;if(prefix>length)prefix=length;line[prefix]=0;
            caret_x=12+ui_text_width(screen,line);caret_y=42+i*12;
        }
        if(!length||next>=s->keyboard.length)break;at=next;
    }
    u32 now=((u32(*)(void))SEAM_RakNet_GetTimeMS)();
    if(!blink_start||s->keyboard.cursor!=last_cursor||s->keyboard.length!=last_length){blink_start=now;last_cursor=s->keyboard.cursor;last_length=s->keyboard.length;}
    if((now-blink_start)%1000<500)ui_fill(screen,caret_x,caret_y,caret_x+1,caret_y+11,&ui_form_white);
}
