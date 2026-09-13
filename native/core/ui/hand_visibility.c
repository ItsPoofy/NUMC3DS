#include "hand_visibility.h"

typedef void (*ItemInHandRenderFn)(void*,float);
typedef void (__attribute__((pcs("aapcs-vfp"))) *TopHudRenderFn)(void*,float);

static const char hand_render_fail[]="NuMC3DS UI: hand renderer hook failed\n";
static const char top_hud_render_fail[]="NuMC3DS UI: top HUD renderer hook failed\n";

int hand_visibility_hidden(void){return s->hide_hand!=0;}
void hand_visibility_set_hidden(int hidden){s->hide_hand=hidden?1u:0u;}
int top_hud_visibility_hidden(void){return s->hide_top_hud!=0;}
void top_hud_visibility_set_hidden(int hidden){s->hide_top_hud=hidden?1u:0u;}

static void on_item_in_hand_render(void*renderer,float tick){
    if(!hand_visibility_hidden())((ItemInHandRenderFn)s->item_in_hand_render.trampoline)(renderer,tick);
}

static void __attribute__((pcs("aapcs-vfp"))) on_top_hud_render(void*screen,float tick){
    if(!top_hud_visibility_hidden())((TopHudRenderFn)s->top_hud_render.trampoline)(screen,tick);
    else ((GuiShaderColorFn)SEAM_GuiComponent_setShaderColor)((void*)SEAM_UiShaderColorGlobal,(const void*)SEAM_UiWhiteColor);
}

int hand_visibility_install_hook(void){
    NuMC3DS_Hook*hook=&s->item_in_hand_render;
    hook->target=SEAM_ItemInHandRenderer_render;
    hook->replacement=(u32)on_item_in_hand_render;
    hook->expected[0]=0xE92D4FF3u;
    hook->expected[1]=0xE1A04000u;
    if(s->host.install_hook(hook)){
        s->host.debug_string(hand_render_fail,sizeof(hand_render_fail)-1);
        return -31;
    }
    hook=&s->top_hud_render;
    hook->target=SEAM_InGamePlayScreen_renderTopHud;
    hook->replacement=(u32)on_top_hud_render;
    hook->expected[0]=0xE92D4010u;
    hook->expected[1]=0xE1A04000u;
    if(s->host.install_hook(hook)){
        s->host.debug_string(top_hud_render_fail,sizeof(top_hud_render_fail)-1);
        return -32;
    }
    return 0;
}
