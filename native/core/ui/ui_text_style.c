#include "ui_text_style.h"
typedef void (*UiTextFn)(void*,void*,const void*,int,int,const Color*,int);
typedef void (*OptionItemRenderFn)(void*,void*,u32,u32);

const Color ui_reference_text_color={76.0f/255.0f,76.0f/255.0f,76.0f/255.0f,1.0f};
static const Color ui_settings_text_color={1.0f,1.0f,1.0f,1.0f};

static const char text_fail[]="NuMC3DS UI: text color hook failed\n";
static const char centered_text_fail[]="NuMC3DS UI: centered text color hook failed\n";
static const char centered_at_text_fail[]="NuMC3DS UI: centered-at text color hook failed\n";
static const char option_item_render_fail[]="NuMC3DS UI: option text color hook failed\n";
static const char container_text_fail[]="NuMC3DS UI: container label hook failed\n";

static int is_pure_black(const Color*color){return color&&color->r==0.0f&&color->g==0.0f&&color->b==0.0f;}
static int is_selection_yellow(const Color*color){return color&&color->r==1.0f&&color->g==1.0f&&color->b==0.0f;}
static int is_dropdown_component(void*component){return component&&*(void**)component==(void*)0x009DA5B8u;}
static int is_creative_container_component(void*component){return component&&(u32)component>=0x00100008u&&*(void**)((u8*)component-8)==(void*)0x009C30BCu;}
static int is_container_inventory_component(void*component){return component&&(u32)component>=0x00100008u&&*(void**)((u8*)component-8)==(void*)0x009C0EF0u;}
static int is_inventory_title_text(const void*text){
    const char*raw=0;
    if(!text)return 0;
    raw=*(const char* const*)text;
    if(!raw||(u32)raw<0x00100000u||(u32)raw>=0x20000000u)return 0;
    return raw[0]=='I'&&raw[1]=='n'&&raw[2]=='v'&&raw[3]=='e'&&raw[4]=='n'&&raw[5]=='t'&&raw[6]=='o'&&raw[7]=='r'&&raw[8]=='y';
}
static int is_inventory_title(void*component,const void*text){
    return is_container_inventory_component(component)||is_inventory_title_text(text);
}
static const Color*styled_color(void*component,const void*text,const Color*color){
    if(is_inventory_title(component,text))return &ui_settings_text_color;
    if(is_dropdown_component(component)){
        if(is_selection_yellow(color))return color;
        if(color&&(color->r!=color->g||color->g!=color->b))return color;
        if(color&&color->r>0.6f&&color->r<0.65f)return &ui_settings_text_color;
        return &ui_reference_text_color;
    }
    if(is_creative_container_component(component)&&!is_selection_yellow(color))return &ui_reference_text_color;
    if(component&&component==s->option_item_rendering&&!is_selection_yellow(color))return &ui_settings_text_color;
    return is_pure_black(color)?&ui_reference_text_color:color;
}
static void on_ui_text(void*component,void*font,const void*text,int x,int y,const Color*color,int shadow){
    int use_shadow=is_inventory_title(component,text)?1:(is_dropdown_component(component)?0:shadow);
    ((UiTextFn)s->ui_text.trampoline)(component,font,text,x,y,styled_color(component,text,color),use_shadow);
}
static void on_ui_centered_text(void*component,void*font,const void*text,int x,int y,const Color*color,int shadow){
    int use_shadow=is_inventory_title(component,text)?1:(is_dropdown_component(component)?0:shadow);
    ((UiTextFn)s->ui_centered_text.trampoline)(component,font,text,x,y,styled_color(component,text,color),use_shadow);
}
static void on_ui_centered_at_text(void*component,void*font,const void*text,int x,int y,const Color*color,int shadow){
    int use_shadow=is_inventory_title(component,text)?1:(is_dropdown_component(component)?0:shadow);
    ((UiTextFn)s->ui_centered_at_text.trampoline)(component,font,text,x,y,styled_color(component,text,color),use_shadow);
}
static void on_option_item_render(void*item,void*game,u32 param3,u32 param4){void*prior=s->option_item_rendering;s->option_item_rendering=item;((OptionItemRenderFn)s->option_item_render.trampoline)(item,game,param3,param4);s->option_item_rendering=prior;}

static void on_container_labels(int*screen,u32 use_screen){
    void**vtable=*(void***)screen;
    int is_dark_bg=(vtable==(void**)0x009C30BCu||vtable==(void**)0x009C0EF0u);
    const Color*title_color=is_dark_bg?&ui_settings_text_color:&ui_reference_text_color;
    int shadow=is_dark_bg?1:0;
    ((UiTextFn)s->ui_text.trampoline)(screen+2,(void*)screen[0x19],screen+0x31,screen[0x32],screen[0x33],title_color,shadow);
    ((void(*)(int*,u32))vtable[0x290/4])(screen,use_screen);
    ((void(*)(int*,int))vtable[0x28C/4])(screen,1);
}

static int install(NuMC3DS_Hook*hook,u32 target,u32 replacement,u32 first,u32 second,const char*error,unsigned length,int code){hook->target=target;hook->replacement=replacement;hook->expected[0]=first;hook->expected[1]=second;if(s->host.install_hook(hook)){s->host.debug_string(error,length);return code;}return 0;}

int ui_text_style_install_hooks(void){int result;result=install(&s->ui_text,SEAM_GuiComponent_drawString,(u32)on_ui_text,0xE92D4038u,0xE1A0C003u,text_fail,sizeof(text_fail)-1,-20);if(result)return result;result=install(&s->ui_centered_text,SEAM_GuiComponent_drawCenteredString,(u32)on_ui_centered_text,0xE92D43F0u,0xE1A05001u,centered_text_fail,sizeof(centered_text_fail)-1,-33);if(result)return result;result=install(&s->ui_centered_at_text,SEAM_GuiComponent_drawCenteredAtString,(u32)on_ui_centered_at_text,0xE92D41F0u,0xE1A06001u,centered_at_text_fail,sizeof(centered_at_text_fail)-1,-34);if(result)return result;result=install(&s->option_item_render,SEAM_OptionItem_render,(u32)on_option_item_render,0xE92D4FFFu,0xE24DD024u,option_item_render_fail,sizeof(option_item_render_fail)-1,-35);if(result)return result;result=install(&s->container_labels,SEAM_ContainerScreen_renderLabels,(u32)on_container_labels,0xE92D4030u,0xE1A05001u,container_text_fail,sizeof(container_text_fail)-1,-23);if(result)return result;return 0;}
