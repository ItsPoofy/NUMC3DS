#include "ui_widgets.h"
#include "ui_text_style.h"

static const char button_color_fail[]="NuMC3DS UI: button color hook failed\n";

static void *colored_gui_button_ctor(void*memory,void*game,int id,int x,int y,int width,int height,const char*label,int param9,int param10){
    void*object=((GuiButtonCtor)s->button_ctor.trampoline)(memory,game,id,x,y,width,height,label,param9,param10);
    if(object){((UiButtonView*)object)->normal_label=ui_reference_text_color;((UiButtonView*)object)->pressed_label=ui_reference_text_color;}
    return object;
}

int ui_install_button_color_hook(void){
    NuMC3DS_Hook*hook=&s->button_ctor;
    hook->target=SEAM_GuiButton_ctor;
    hook->replacement=(u32)colored_gui_button_ctor;
    hook->expected[0]=0xE92D43F0u;
    hook->expected[1]=0xE1A04003u;
    if(s->host.install_hook(hook)){
        s->host.debug_string(button_color_fail,sizeof(button_color_fail)-1);
        return -15;
    }
    return 0;
}

void ui_button_set_label(void*button,const char*text){u32 str=0,scratch=0;void*game,*font;if(!button)return;((StrCtor)SEAM_StrCtor)(&str,text,&scratch);((StrAssign)SEAM_StrAssign)(&((UiButtonView*)button)->label,&str);((StrDtor)SEAM_StrDtor)(&str);game=((UiButtonView*)button)->game;font=game?*(void**)((u8*)game+0x5C):0;if(font)((UiButtonView*)button)->cached_label_width=((TextWidth)SEAM_TextWidth)(font,&((UiButtonView*)button)->label,0,1.0f);}
void ui_button_set_latched(void*button,int latched){if(button)((UiButtonView*)button)->visual_press_on_drag=(u8)(latched!=0);}
void ui_button_set_manual_render(void*button,int manual){if(button)((UiButtonView*)button)->hidden_from_screen_pass=(u8)(manual!=0);}
void ui_play_button_sound(void*screen){void*game=ui_screen_game(screen);if(game)((GamePlaySoundFn2)SEAM_MinecraftGame_playSound)(game,SEAM_UiButtonClickSoundHash,1.0f,1.0f);}
const char *ui_button_label(void*button){const char*label=0;if(button)cp(&label,&((UiButtonView*)button)->label,sizeof(label));return label?label:"";}

static void prepare_button_background(UiButtonView*b){NinePatchSetSizeFn set_size=(NinePatchSetSizeFn)SEAM_NinePatchLayer_setSize;if(b->normal_background)set_size(b->normal_background,(float)b->element.width,(float)b->element.height);if(b->pressed_background)set_size(b->pressed_background,(float)b->element.width,(float)b->element.height);if(b->inactive_background)set_size(b->inactive_background,(float)b->element.width,(float)b->element.height);}

void *ui_create_button(void*screen,int id,int x,int y,int width,int height,const char*label,int visible,int screen_mask){
    UiShared shared={0,0};void*memory,*control,*object;GameAllocWithSelector alloc=(GameAllocWithSelector)SEAM_Heap_allocWithSelector;
    memory=alloc(0xA4,(void*)SEAM_game_alloc_selector);if(!memory)return 0;
    shared.object=((GuiButtonCtor)SEAM_GuiButton_ctor)(memory,ui_screen_game(screen),id,x,y,width,height,label,0,0x7FFFFFFF);
    if(!shared.object){s->host.heap_free(memory);return 0;}
    ((GuiButtonBuildBackground)SEAM_GuiButton_buildBackground)(shared.object);
    prepare_button_background((UiButtonView*)shared.object);
    control=((ControlAlloc)SEAM_ControlAlloc)();if(!control){release_obj(shared.object);return 0;}
    ((ControlFn)SEAM_ControlLock)(control);if(*((u8*)control+8)){shared.control=control;((ControlFn)SEAM_ControlReference)(control);}((ControlFn)SEAM_ControlUnlock)(control);
    if(!shared.control){release_obj(shared.object);return 0;}
    object=shared.object;ui_element_set_visible(object,visible);ui_element_set_screen(object,screen_mask);ui_screen_add_button(screen,&shared);ui_shared_release(&shared);return object;
}

void *ui_create_grid_button(void*screen,int id,int x,int y,int width,int height,const char*label,int visible,int screen_mask){
    void*memory,*object;GameAllocWithSelector alloc=(GameAllocWithSelector)SEAM_Heap_allocWithSelector;
    memory=alloc(0xA4,(void*)SEAM_game_alloc_selector);if(!memory)return 0;
    object=((GuiButtonCtor)SEAM_GuiButton_ctor)(memory,ui_screen_game(screen),id,x,y,width,height,label,0,0x7FFFFFFF);
    if(!object){s->host.heap_free(memory);return 0;}
    ((GuiButtonBuildBackground)SEAM_GuiButton_buildBackground)(object);
    prepare_button_background((UiButtonView*)object);
    /* The option grid owns and renders this button.  Giving it the screen's
     * normal callback owner lets the stock grid dispatch buttonPressed while
     * avoiding a second, floating registration in Screen::buttons. */
    ((UiButtonView*)object)->owner=ui_screen_button_owner(screen);
    ui_element_set_visible(object,visible);ui_element_set_screen(object,screen_mask);
    return object;
}

void *ui_create_hud_button(void*screen,int id,int x,int y,int width,int height,int visible,int chat_icon){
    UiShared shared={0,0};u8 type=(u8)id;u32 icon_resource[5],background_resource[5];void*object;
    Rect icon_normal={0x40,0x70,0x10,0x10},icon_pressed={0x40,0x70,0x10,0x10};
    Rect background_normal={0x48,0xB8,0x0E,0x0E},background_hover={0x56,0xB8,0x0E,0x0E},background_pressed={0x56,0xB8,0x0E,0x0E};
    zero(icon_resource,sizeof(icon_resource));zero(background_resource,sizeof(background_resource));
    ((HudButtonFactory)SEAM_HudButtonFactory)((Shared*)&shared,ui_screen_game(screen),&type,&x,&y,&width,&height);if(!shared.object||!shared.control)return 0;
    ((ResourceLocationCtor)SEAM_ResourceLocation_ctor)(background_resource,"textures/gui/spritesheet",0);
    if(chat_icon){icon_normal.x=icon_pressed.x=0x60;icon_normal.y=icon_pressed.y=0xE0;}
    ((ResourceLocationCtor)SEAM_ResourceLocation_ctor)(icon_resource,"textures/gui/spritesheet",0);
    ((HudButtonSkin)SEAM_HudButtonSkin)(shared.object,icon_resource,background_resource,&icon_normal,&icon_pressed,&background_normal,&background_hover,&background_pressed,4,4,1,4);
    if(chat_icon){*(int*)((u8*)shared.object+SEAM_HudButtonSkin_iconOffsetX)+=2;*(int*)((u8*)shared.object+SEAM_HudButtonSkin_iconOffsetY)-=2;}
    else {*(int*)((u8*)shared.object+SEAM_HudButtonSkin_iconOffsetX)-=1;}
    object=shared.object;ui_element_set_visible(object,visible);ui_element_set_screen(object,0x80);ui_screen_add_button(screen,&shared);ui_shared_release(&shared);return object;
}

void ui_button_draw_native(void*screen,void*button,int touch_x,int touch_y){void**vtable;UiButtonView*b=(UiButtonView*)button;if(!b||!b->element.visible)return;vtable=*(void***)b;((void(*)(void*,void*,int,int))vtable[7])(b,ui_screen_game(screen),touch_x,touch_y);}
int ui_button_label_width(void*screen,void*button){UiButtonView*b=(UiButtonView*)button;void*font=ui_screen_font(screen);return b&&font?((TextWidth)SEAM_TextWidth)(font,&b->label,0,1.0f):0;}
void ui_button_sync_label_width(void*screen,void*button){UiButtonView*b=(UiButtonView*)button;if(b)b->cached_label_width=ui_button_label_width(screen,button);}
int ui_button_label_y(void*screen,void*button){UiButtonView*b=(UiButtonView*)button;(void)screen;return b?b->element.y+(b->element.height-11)/2:0;}
static void emit_quad(void*tess,const UiNinePatchQuad*q,float x,float y){TessellatorVertexUvFn vertex=(TessellatorVertexUvFn)SEAM_Tessellator_vertexUVFloats;vertex(tess,x+q->x1,y+q->y2,q->z,q->u1,q->v2);vertex(tess,x+q->x2,y+q->y2,q->z,q->u2,q->v2);vertex(tess,x+q->x2,y+q->y1,q->z,q->u2,q->v1);vertex(tess,x+q->x1,y+q->y1,q->z,q->u1,q->v1);}
void ui_mesh_cache_reset(UiMeshCache*cache){if(!cache)return;if(cache->ready)((MeshDtorFn)SEAM_Mesh_dtor)(cache->words);zero(cache,sizeof(*cache));}
void *ui_buttons_draw_native_cached(UiMeshCache*cache,void**buttons,unsigned count,void*pressed){UiButtonView*b;UiNinePatchLayerView*layer=0;void*tess=(void*)SEAM_UiTessellator;unsigned i,j;if(!cache)return 0;if(!cache->ready){((TessellatorBeginFn)SEAM_Tessellator_beginMaxVertices)(tess,count*36u);for(i=0;i<count;i++){b=(UiButtonView*)buttons[i];if(!b||!b->element.visible)continue;layer=(UiNinePatchLayerView*)(b->element.active?b->normal_background:b->inactive_background);if(!layer)continue;if(!cache->texture)cache->texture=layer->texture;for(j=0;j<9;j++)if(!(layer->hidden_quads&(1u<<j)))emit_quad(tess,&layer->quads[j],(float)b->element.x,(float)b->element.y);}((TessellatorEndFn)SEAM_Tessellator_end)(cache->words,tess,0,0);cache->ready=1;}
    if(cache->texture){((GuiShaderColorFn)SEAM_GuiComponent_setShaderColor)((void*)SEAM_UiShaderColorGlobal,(const void*)SEAM_UiWhiteColor);((MeshRenderTexturedFn)SEAM_Mesh_renderTextured)(cache->words,(void*)SEAM_UiNinePatchMaterial,cache->texture,0,0);}return pressed;}
void ui_button_draw_background_cached(UiMeshCache*cache,void*button,int pressed){UiButtonView*b=(UiButtonView*)button;UiNinePatchLayerView*layer;void*tess=(void*)SEAM_UiTessellator;unsigned j;if(!cache||!b||!b->element.visible)return;if(!cache->ready){layer=(UiNinePatchLayerView*)(b->element.active?(pressed?b->pressed_background:b->normal_background):b->inactive_background);if(!layer)return;cache->texture=layer->texture;((TessellatorBeginFn)SEAM_Tessellator_beginMaxVertices)(tess,36u);for(j=0;j<9;j++)if(!(layer->hidden_quads&(1u<<j)))emit_quad(tess,&layer->quads[j],(float)b->element.x,(float)b->element.y);((TessellatorEndFn)SEAM_Tessellator_end)(cache->words,tess,0,0);cache->ready=1;}if(cache->texture){((GuiShaderColorFn)SEAM_GuiComponent_setShaderColor)((void*)SEAM_UiShaderColorGlobal,(const void*)SEAM_UiWhiteColor);((MeshRenderTexturedFn)SEAM_Mesh_renderTextured)(cache->words,(void*)SEAM_UiNinePatchMaterial,cache->texture,0,0);}}

void ui_fill(void*screen,int x1,int y1,int x2,int y2,const Color*color){((FillRect)SEAM_FillRect)(screen,x1,y1,x2,y2,color);}
void ui_button_draw_highlight(void*screen,void*button){static const Color yellow={1.0f,1.0f,0.0f,1.0f};UiElementView*e=(UiElementView*)button;if(!screen||!e)return;ui_fill(screen,e->x,e->y,e->x+e->width,e->y+1,&yellow);ui_fill(screen,e->x,e->y+e->height-1,e->x+e->width,e->y+e->height,&yellow);ui_fill(screen,e->x,e->y,e->x+1,e->y+e->height,&yellow);ui_fill(screen,e->x+e->width-1,e->y,e->x+e->width,e->y+e->height,&yellow);}

void ui_cached_text_set(void*screen,UiCachedText*cache,const char*text){u32 temporary=0,scratch=0;void*font=ui_screen_font(screen);unsigned i=0;int same;if(!cache)return;if(!text)text="";while(i<151&&cache->text[i]&&cache->text[i]==text[i])i++;same=i<151&&cache->text[i]==text[i];if(same&&cache->font==font)return;if(!cache->ready){((StrCtor)SEAM_StrCtor)(&cache->handle,text,&scratch);cache->ready=1;}else if(!same){((StrCtor)SEAM_StrCtor)(&temporary,text,&scratch);((StrAssign)SEAM_StrAssign)(&cache->handle,&temporary);((StrDtor)SEAM_StrDtor)(&temporary);}if(!same)copy_text(cache->text,text,151);cache->font=font;cache->width=font?((TextWidth)SEAM_TextWidth)(font,&cache->handle,0,1.0f):0;}
void ui_cached_text_draw(void*screen,UiCachedText*cache,float x,float y,float scale,const Color*color){void*font=ui_screen_font(screen);if(font&&cache&&cache->ready&&cache->text[0])((DrawText)SEAM_DrawText)(font,x,y,scale,&cache->handle,color,0,0xFFFFFFFFu);}
void ui_cached_text_reset(UiCachedText*cache){if(!cache)return;if(cache->ready)((StrDtor)SEAM_StrDtor)(&cache->handle);zero(cache,sizeof(*cache));}
int ui_text_width(void*screen,const char*text){UiCachedText temporary;int width;zero(&temporary,sizeof(temporary));ui_cached_text_set(screen,&temporary,text);width=temporary.width;if(temporary.ready)((StrDtor)SEAM_StrDtor)(&temporary.handle);return width;}

int ui_shared_from_object(void*object,UiShared*shared){void*control;if(!shared)return 0;shared->object=shared->control=0;if(!object)return 0;control=((ControlAlloc)SEAM_ControlAlloc)();if(!control){release_obj(object);return 0;}((ControlFn)SEAM_ControlLock)(control);if(*((u8*)control+8)){shared->object=object;shared->control=control;((ControlFn)SEAM_ControlReference)(control);}((ControlFn)SEAM_ControlUnlock)(control);if(!shared->control){release_obj(object);return 0;}return 1;}

int ui_create_world_switch_shared(void*screen,int enabled,int id,UiShared*shared){
    u32 path[13],track[18],temporary[18];void*memory,*object,*game=ui_screen_game(screen);GameAllocWithSelector alloc=(GameAllocWithSelector)SEAM_Heap_allocWithSelector;if(!shared)return 0;shared->object=shared->control=0;
    zero(path,sizeof(path));zero(track,sizeof(track));zero(temporary,sizeof(temporary));memory=alloc(0x148,(void*)SEAM_game_alloc_selector);if(!memory)return 0;
    ((TexturePathStateInit)SEAM_TexturePathStateInit)(path,*(void**)((u8*)game+0x58),(void*)SEAM_WorldSwitch_trackResource,0);
    ((TexturePtrCopyCtor)SEAM_TexturePtrCopyCtor)(track,path);track[8]=0;track[9]=0;track[10]=0x42040000u;track[11]=0x41900000u;track[12]=0;track[13]=0xC3u;track[14]=0x21u;track[15]=0x12u;*((u8*)track+0x40)=1;((TexturePtrDtor)SEAM_TexturePtrDtor)(path);
    ((TexturePtrCtor)SEAM_TexturePtrCtor)(temporary);temporary[12]=0;temporary[13]=0;temporary[14]=1;temporary[15]=1;((TexturePtrAssign)SEAM_TexturePtrAssign)(temporary,track);cp((u8*)temporary+0x20,(u8*)track+0x20,0x21);
    object=((WorldSwitchButtonCtor)SEAM_WorldSwitchButton_ctor)(memory,game,enabled,temporary,id);((TexturePtrDtor)SEAM_TexturePtrDtor)(temporary);((TexturePtrDtor)SEAM_TexturePtrDtor)(track);
    if(!object){s->host.heap_free(memory);return 0;}((UiButtonView*)object)->owner=ui_screen_button_owner(screen);ui_element_set_visible(object,1);return ui_shared_from_object(object,shared);
}
void *ui_create_world_switch(void*screen,int enabled,int id){UiShared shared={0,0};if(!ui_create_world_switch_shared(screen,enabled,id,&shared))return 0;return shared.object;}

void *ui_create_option_label(void*screen,const char*text){u32 str=0,scratch=0;void*memory,*object;GameAllocWithSelector alloc=(GameAllocWithSelector)SEAM_Heap_allocWithSelector;memory=alloc(0xC0,(void*)SEAM_game_alloc_selector);if(!memory)return 0;((StrCtor)SEAM_StrCtor)(&str,text,&scratch);object=((GuiTextCtor)SEAM_GuiText_ctor)(memory,ui_screen_game(screen),&str,*(void**)SEAM_CreateWorldLabelColorPtr,0,2,2,0,0);((StrDtor)SEAM_StrDtor)(&str);if(!object){s->host.heap_free(memory);return 0;}ui_element_set_visible(object,1);return object;}
void ui_warning_set_message(void*warning,const char*message){u32 value=0,scratch=0;if(!warning)return;((StrCtor)SEAM_StrCtor)(&value,message,&scratch);((StrAssign)SEAM_StrAssign)((u8*)warning+0xA4,&value);((StrDtor)SEAM_StrDtor)(&value);}
