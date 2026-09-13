#include "ui_form_style.h"
const Color ui_form_background={0.54f,0.54f,0.54f,1},ui_form_white={1,1,1,1};
static const Color edge={0.04f,0.04f,0.04f,1},shadow={0.25f,0.25f,0.25f,1},input={0.33f,0.33f,0.33f,1},output={0.18f,0.18f,0.18f,1},bar={0.77f,0.77f,0.77f,1},ink={0.30f,0.30f,0.30f,1};
void ui_form_field(void *screen,int x,int y,int width,int height,int editable){
    ui_fill(screen,x,y,x+width,y+height,&edge);
    ui_fill(screen,x+1,y+1,x+width-1,y+height-1,editable?&shadow:&output);
    if(editable)ui_fill(screen,x+3,y+3,x+width-1,y+height-1,&input);
}
void ui_form_header_reset(UiFormHeader *header){if(header->layer)release_obj(header->layer);header->layer=0;}
void ui_form_header(void *screen,int width,UiCachedText *title,UiFormHeader *header){
    typedef void (__attribute__((pcs("aapcs-vfp"))) *CreateFn)(void**,void*,const int*,int,int,float,float);
    typedef void (__attribute__((pcs("aapcs-vfp"))) *DrawFn)(void*,void*,float,float);
    if(!header->layer){
        u32 factory[9];int rect[4]={80,200,8,8};zero(factory,sizeof(factory));
        ((void(*)(void*,void*,void*))0x003382F8u)(factory,*(void**)((u8*)ui_screen_game(screen)+0x58),(void*)0x00ABFD74u);
        ((CreateFn)0x00338130u)(&header->layer,factory,rect,3,3,(float)width,30.0f);
    }
    if(header->layer){
        ((NinePatchSetSizeFn)SEAM_NinePatchLayer_setSize)(header->layer,(float)width,30.0f);
        ((GuiShaderColorFn)SEAM_GuiComponent_setShaderColor)((void*)SEAM_UiShaderColorGlobal,(void*)SEAM_UiWhiteColor);
        ((DrawFn)0x0027EFE0u)(header->layer,(void*)SEAM_UiTessellator,0,0);
    }
    ui_cached_text_draw(screen,title,(width-title->width)/2,9,1,&ink);
}
