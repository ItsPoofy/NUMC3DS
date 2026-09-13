#include "leave_screen_ui.h"
#include "progress_tips.h"
#include "ui_localize.h"

typedef void *(*ConstructorFn)(void*,void*,void*,void*);
typedef void (*RenderFn)(void*,int,int,int);
typedef void (*BackgroundFn)(void*,int);
static NuMC3DS_Hook constructor_hook,render_hook,dtor_hook,set_leave_screen_hook;
static UiProgressTips tips;
static int tips_ready,leave_screen_active;
static const Color text_color={1,1,1,1};

int leave_screen_ui_rendering(void){return leave_screen_active;}

static void push_chooser_screen(void *chooser, u32 size, void (*ctor)(void*, void*, void*), int clear) {
    void *r4 = *(void**)chooser;
    void *r5 = *(void**)((u8*)chooser + 4);
    typedef void *(*AllocFn)(u32, void*);
    void *screen = ((AllocFn)0x0011493cu)(size, (void*)0x00994898u);
    void *cntrl;
    void *sp_screen = 0;
    void *sp_cntrl = 0;
    u32 sp[2];
    if (!screen) return;
    ctor(screen, r4, r5);
    cntrl = ((void*(*)(void))0x00110E50u)();
    if (cntrl) {
        ((void(*)(void*))0x00123168u)(cntrl);
        if (*(signed char*)((u8*)cntrl + 8) != 0) {
            sp_cntrl = cntrl;
            sp_screen = screen;
            ((void(*)(void*))0x00119C64u)(sp_cntrl);
        }
        ((void(*)(void*))0x001231A8u)(cntrl);
    }
    ((void(*)(void*, void*, void*))0x008AF848u)(sp, sp_cntrl, sp_screen);
    ((void(*)(void*, void*, int))0x0023AC6Cu)(chooser, sp, clear);
    ((void(*)(void*))0x008AF8A0u)(sp);
    if (sp_cntrl) {
        ((void(*)(void*))0x00123168u)(sp_cntrl);
        if (((int(*)(void*))0x00119C80u)(sp_cntrl) && sp_screen) {
            void (**vtable)(void*) = *(void(***)(void*))sp_screen;
            vtable[1](sp_screen);
        }
        ((void(*)(void*))0x001231A8u)(sp_cntrl);
    }
}

static void on_set_leave_level_screen(void *chooser) {
    leave_screen_active = 1;
    push_chooser_screen(chooser, 316, (void (*)(void*, void*, void*))0x004639A8u, 1);
    push_chooser_screen(chooser, 180, (void (*)(void*, void*, void*))0x00321E58u, 0);
}

static void *on_dtor(void *screen) {
    leave_screen_active = 0;
    return ((void*(*)(void*))dtor_hook.trampoline)(screen);
}

static void *on_construct(void *screen,void *game,void *client,void *context){
    void *result=((ConstructorFn)constructor_hook.trampoline)(screen,game,client,context);
    leave_screen_active = 1;
    progress_tips_reset(&tips);
    tips_ready=0;
    return result;
}

static void on_render(void *screen,int touch_x,int touch_y,int use_screen){
    void *font;
    u32 message=0;
    int width;
    if(!ui_resources_ready()){
        ((RenderFn)render_hook.trampoline)(screen,touch_x,touch_y,use_screen);
        return;
    }
    if(!tips_ready){progress_tips_begin(screen,&tips);tips_ready=1;}
    if(use_screen&0x80){
        ((BackgroundFn)(*(void***)screen)[75])(screen,use_screen);
        progress_tips_draw(screen,&tips);return;
    }
    if(!(use_screen&0x40))return;
    font=ui_screen_font(screen);
    if(!font||!ui_localized_string("menu.LeaveLevel.prompt","",&message))return;
    width=((TextWidth)SEAM_TextWidth)(font,&message,0,1.0f);
    ((DrawText)SEAM_DrawText)(font,(float)((400-width)/2),165.0f,1.0f,&message,&text_color,0,0xFFFFFFFFu);
    ((StrDtor)SEAM_StrDtor)(&message);
}

int leave_screen_ui_install_hooks(void){
    constructor_hook.target=0x00321E58u;
    constructor_hook.expected[0]=0xE92D4070u;
    constructor_hook.expected[1]=0xE24DD008u;
    constructor_hook.replacement=(u32)on_construct;
    if(s->host.install_hook(&constructor_hook))return -24;

    render_hook.target=0x00321D78u;
    render_hook.expected[0]=0xE92D4030u;
    render_hook.expected[1]=0xE1A04000u;
    render_hook.replacement=(u32)on_render;
    if(s->host.install_hook(&render_hook))return -25;

    dtor_hook.target=0x00321EE8u;
    dtor_hook.expected[0]=0xE59F2074u;
    dtor_hook.expected[1]=0xE92D4070u;
    dtor_hook.replacement=(u32)on_dtor;
    if(s->host.install_hook(&dtor_hook))return -26;

    set_leave_screen_hook.target=0x0023D5D4u;
    set_leave_screen_hook.expected[0]=0xE92D43F0u;
    set_leave_screen_hook.expected[1]=0xE24DD00Cu;
    set_leave_screen_hook.replacement=(u32)on_set_leave_level_screen;
    return s->host.install_hook(&set_leave_screen_hook)?-27:0;
}
