#include "progress_screen_ui.h"
#include "leave_screen_ui.h"
#include "progress_tips.h"
#include "xp_progress_bar.h"
#include "ui_widgets.h"
#include "../world_transfer/world_transfer_service.h"

enum { TOP_SCREEN=0x40, BOTTOM_SCREEN=0x80, TOP_WIDTH=400, PHASE_TEXT_Y=165 };

static const Color loading_text_color={1.0f,1.0f,1.0f,1.0f};

typedef void (*ProgressSetupFn)(void*);
typedef void (*ProgressMessageFn)(u32*,void*);
typedef void (*ProgressBackgroundFn)(void*,int);
typedef void (*ScreenRenderFn)(void*,int,int,int);
typedef int (*ProgressClientReadyFn)(void*);
typedef void *(*ProgressDtorFn)(void*);

typedef void (*PanoramaRenderFn)(void*,void*);

static int install(NuMC3DS_Hook*hook,u32 target,u32 replacement,u32 first,u32 second,int code){hook->target=target;hook->replacement=replacement;hook->expected[0]=first;hook->expected[1]=second;return s->host.install_hook(hook)?code:0;}
static int string_empty(const u32*value){const char*text=0;if(value)cp(&text,value,sizeof(text));return !text||!*text;}
static int progress_client_ready(UiProgressScreenView*view){return view&&view->progress_handler&&ui_screen_game(view)&&((ProgressClientReadyFn)SEAM_ProgressScreen_clientReady)(ui_screen_game(view))&&view->timer<=0.0f&&view->countdown<=0;}
static float progress_percent(UiProgressScreenView*view){float value=0.0f;if(view&&view->top_progress_widget)value=*(float*)((u8*)view->top_progress_widget+0x5C);if(value<0.0f)value=0.0f;if(value>100.0f)value=100.0f;return value;}

static void on_progress_setup(void*screen){u32 scratch=0;if(!ui_resources_ready()){((ProgressSetupFn)s->progress_setup.trampoline)(screen);return;}if(s->progress_screen&&s->progress_screen!=screen){xp_progress_bar_reset(&s->progress_bar);progress_tips_reset(&s->progress_tips);}((ProgressSetupFn)s->progress_setup.trampoline)(screen);s->progress_screen=screen;xp_progress_bar_build(screen,&s->progress_bar);progress_tips_begin(screen,&s->progress_tips);if(!s->progress_empty_splash_ready){((StrCtor)SEAM_StrCtor)(&s->progress_empty_splash,"",&scratch);s->progress_empty_splash_ready=1;}}

static void on_progress_render(void*screen,int touch_x,int touch_y,int use_screen){UiProgressScreenView*view=(UiProgressScreenView*)screen;void*handler=view->progress_handler,*font=ui_screen_font(screen);u32 phase=0,scratch=0;const u32*message=&view->localized_label;const char*transfer_message;int width,phase_ready=0;float percent;if(!ui_resources_ready()){((ScreenRenderFn)s->progress_render.trampoline)(screen,touch_x,touch_y,use_screen);return;}world_transfer_tick(screen);percent=world_transfer_is_active()?world_transfer_progress_percent():progress_percent(view);if(use_screen&BOTTOM_SCREEN){((ProgressBackgroundFn)(*(void***)screen)[75])(screen,use_screen);progress_tips_draw(screen,&s->progress_tips);return;}if(!(use_screen&TOP_SCREEN))return;if(world_transfer_is_active()){transfer_message=world_transfer_progress_message();if(transfer_message&&transfer_message[0]){((StrCtor)SEAM_StrCtor)(&phase,transfer_message,&scratch);message=&phase;phase_ready=1;}}else if(progress_client_ready(view)){ProgressMessageFn message_fn=(ProgressMessageFn)(*(void***)handler)[8];if(message_fn){message_fn(&phase,handler);if(!string_empty(&phase)){message=&phase;phase_ready=1;}}}if(font&&!string_empty(message)){width=((TextWidth)SEAM_TextWidth)(font,(void*)message,0,1.0f);((DrawText)SEAM_DrawText)(font,(float)((TOP_WIDTH-width)/2),(float)PHASE_TEXT_Y,1.0f,message,&loading_text_color,0,0xFFFFFFFFu);}xp_progress_bar_draw(screen,&s->progress_bar,percent,touch_x,touch_y);if(phase_ready)((StrDtor)SEAM_StrDtor)(&phase);}

static void on_panorama_render(void*screen,void*context){u32*begin=0,*end=0,*selected=0,saved=0;int index=0;if((s->progress_screen||leave_screen_ui_rendering())&&s->progress_empty_splash_ready){begin=*(u32**)((u8*)screen+0x120);end=*(u32**)((u8*)screen+0x124);index=*(int*)((u8*)screen+0x12C);if(begin&&end&&index>=0&&begin+index<end){selected=begin+index;saved=*selected;*selected=s->progress_empty_splash;}}((PanoramaRenderFn)s->panorama_render.trampoline)(screen,context);if(selected)*selected=saved;}

static void *on_progress_dtor(void*screen){int reopen=world_transfer_on_progress_destroy(screen);if(s->progress_screen==screen){xp_progress_bar_reset(&s->progress_bar);progress_tips_reset(&s->progress_tips);s->progress_screen=0;}screen=((ProgressDtorFn)s->progress_dtor.trampoline)(screen);if(reopen)world_transfer_reopen_after_progress();return screen;}

int progress_screen_ui_install_hooks(void){int result;if((result=install(&s->progress_setup,SEAM_ProgressScreen_setup,(u32)on_progress_setup,0xE92D4FF0u,0xE1A04000u,-16)))return result;if((result=install(&s->progress_render,SEAM_ProgressScreen_render,(u32)on_progress_render,0xE92D4FFFu,0xE1A01003u,-17)))return result;if((result=install(&s->panorama_render,SEAM_CubemapBackgroundScreen_render,(u32)on_panorama_render,0xE92D41F0u,0xE1A05000u,-18)))return result;if((result=install(&s->progress_dtor,SEAM_ProgressScreen_dtor,(u32)on_progress_dtor,0xE92D41F0u,0xE1A05000u,-19)))return result;return leave_screen_ui_install_hooks();}
