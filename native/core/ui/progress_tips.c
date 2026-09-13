#include "progress_tips.h"
#include "ui_localize.h"
#include "ui_text_layout.h"

enum { TIP_COUNT=69, TIP_INTERVAL_MS=5000, TIP_MAX_WIDTH=304, TIP_LINE_STEP=17, TIP_TEXT_HEIGHT=11, BOTTOM_WIDTH=320, BOTTOM_HEIGHT=240 };

static const Color tip_color={1.0f,1.0f,1.0f,1.0f};

static u32 next_random(UiProgressTips*tips){u32 value=tips->rng;if(!value)value=0x6D2B79F5u;value^=value<<13;value^=value>>17;value^=value<<5;tips->rng=value;return value;}
static void make_tip_key(int index,char*out){unsigned length=0;int number;if(index==0){copy_text(out,"tips.game.skinpacks",31);return;}if(index<=43){number=index;if(index>=21)number++;if(index>=39)number++;append(out,&length,"tips.game.");append_int(out,&length,number);return;}append(out,&length,"tips.trivia.");append_int(out,&length,index-43);}
static void add_line(UiProgressTips*tips,void*font,const char*text){u32 scratch=0;unsigned line=tips->line_count;if(line>=PROGRESS_TIP_LINES)return;((StrCtor)SEAM_StrCtor)(&tips->lines[line],text,&scratch);tips->widths[line]=font?((TextWidth)SEAM_TextWidth)(font,&tips->lines[line],0,1.0f):0;tips->line_count++;}
static void wrap_text(UiProgressTips*tips,void*font,const char*text){unsigned at=0,next;char line[152];while(text[at]&&tips->line_count<PROGRESS_TIP_LINES){if(!ui_text_wrap_line(font,text,at,TIP_MAX_WIDTH,line,sizeof(line),&next))break;add_line(tips,font,line);at=next;}}
static void choose_tip(void*screen,UiProgressTips*tips,u32 now){char key[32],text[256];int chosen;void*font=ui_screen_font(screen);do{chosen=(int)(next_random(tips)%TIP_COUNT);}while(chosen==tips->current);progress_tips_reset(tips);tips->current=chosen;tips->rng^=now|1u;make_tip_key(chosen,key);ui_localized_label(text,sizeof(text),key,key);wrap_text(tips,font,text);tips->next_change=now+TIP_INTERVAL_MS;}

void progress_tips_reset(UiProgressTips*tips){unsigned i;if(!tips)return;for(i=0;i<tips->line_count&&i<PROGRESS_TIP_LINES;i++)if(tips->lines[i])((StrDtor)SEAM_StrDtor)(&tips->lines[i]);for(i=0;i<PROGRESS_TIP_LINES;i++){tips->lines[i]=0;tips->widths[i]=0;}tips->line_count=0;}
void progress_tips_begin(void*screen,UiProgressTips*tips){u32 now=((u32(*)(void))SEAM_RakNet_GetTimeMS)();if(!tips)return;progress_tips_reset(tips);zero(tips,sizeof(*tips));tips->current=-1;tips->rng=now^(u32)screen^0x9E3779B9u;choose_tip(screen,tips,now);}
void progress_tips_draw(void*screen,UiProgressTips*tips){u32 now;void*font;int y,total_height;unsigned i;if(!screen||!tips)return;now=((u32(*)(void))SEAM_RakNet_GetTimeMS)();if((s32)(now-tips->next_change)>=0)choose_tip(screen,tips,now);font=ui_screen_font(screen);if(!font||!tips->line_count)return;total_height=((int)tips->line_count-1)*TIP_LINE_STEP+TIP_TEXT_HEIGHT;y=(BOTTOM_HEIGHT-total_height)/2;for(i=0;i<tips->line_count;i++)((DrawText)SEAM_DrawText)(font,(float)((BOTTOM_WIDTH-tips->widths[i])/2),(float)(y+(int)i*TIP_LINE_STEP),1.0f,&tips->lines[i],&tip_color,0,0xFFFFFFFFu);}
