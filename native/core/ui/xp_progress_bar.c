#include "xp_progress_bar.h"
#include "ui_widgets.h"
#include "ui_renderer.h"

enum { BAR_SOURCE_WIDTH=182, BAR_SOURCE_HEIGHT=5, BAR_SCALE=2, BAR_X=(400-BAR_SOURCE_WIDTH*BAR_SCALE)/2, BAR_Y=204, BAR_EMPTY_Y=64, BAR_FULL_Y=69 };

static void build_mesh(UiMeshCache*cache,UiXpProgressBar*bar,int source_y,int source_width){
    void*tess=(void*)SEAM_UiTessellator;
    TessellatorVertexUvFn vertex=(TessellatorVertexUvFn)SEAM_Tessellator_vertexUVFloats;
    float left=(float)BAR_X,right=(float)(BAR_X+source_width*BAR_SCALE),top=(float)BAR_Y,bottom=(float)(BAR_Y+BAR_SOURCE_HEIGHT*BAR_SCALE);
    float u2=(float)source_width/(float)bar->texture_width,v1=(float)source_y/(float)bar->texture_height,v2=(float)(source_y+BAR_SOURCE_HEIGHT)/(float)bar->texture_height;
    ((TessellatorBeginFn)SEAM_Tessellator_beginMaxVertices)(tess,4u);
    vertex(tess,left,bottom,0.0f,0.0f,v2);
    vertex(tess,right,bottom,0.0f,u2,v2);
    vertex(tess,right,top,0.0f,u2,v1);
    vertex(tess,left,top,0.0f,0.0f,v1);
    ((TessellatorEndFn)SEAM_Tessellator_end)(cache->words,tess,0,0);
    cache->texture=bar->texture;
    cache->ready=1;
}

void xp_progress_bar_reset(UiXpProgressBar*bar){
    if(!bar)return;
    ui_mesh_cache_reset(&bar->empty);
    ui_mesh_cache_reset(&bar->fill);
    zero(bar,sizeof(*bar));
    bar->source_fill=-1;
}

int xp_progress_bar_build(void*screen,UiXpProgressBar*bar){
    void*client,*gui,*texture;
    if(!screen||!bar)return 0;
    xp_progress_bar_reset(bar);
    client=ui_screen_client(screen);
    gui=client?((void*(*)(void*))SEAM_ClientInstance_getGui)(client):0;
    if(!gui)return 0;
    bar->material=(u8*)gui+SEAM_GuiData_uiTexturedMaterialOffset;
    bar->texture=(u8*)gui+SEAM_GuiData_iconsTextureOffset;
    texture=((TexturePtrDerefFn)SEAM_TexturePtr_deref)(bar->texture);
    if(!texture)return 0;
    bar->texture_width=*(int*)((u8*)texture+4);
    bar->texture_height=*(int*)((u8*)texture+8);
    if(bar->texture_width<BAR_SOURCE_WIDTH||bar->texture_height<BAR_FULL_Y+BAR_SOURCE_HEIGHT)return 0;
    build_mesh(&bar->empty,bar,BAR_EMPTY_Y,BAR_SOURCE_WIDTH);
    bar->ready=bar->empty.ready;
    return bar->ready;
}

void xp_progress_bar_draw(void*screen,UiXpProgressBar*bar,float percent,int touch_x,int touch_y){
    int source_width;
    (void)screen;(void)touch_x;(void)touch_y;
    if(!bar||!bar->ready||!bar->material||!bar->texture)return;
    if(percent<0.0f)percent=0.0f;
    if(percent>100.0f)percent=100.0f;
    source_width=(int)(percent*(float)BAR_SOURCE_WIDTH/100.0f);
    if(source_width!=bar->source_fill){
        ui_mesh_cache_reset(&bar->fill);
        bar->source_fill=source_width;
        if(source_width>0)build_mesh(&bar->fill,bar,BAR_FULL_Y,source_width);
    }
    ((GuiShaderColorFn)SEAM_GuiComponent_setShaderColor)((void*)SEAM_UiShaderColorGlobal,(const void*)SEAM_UiWhiteColor);
    ((MeshRenderTexturedFn)SEAM_Mesh_renderTextured)(bar->empty.words,bar->material,bar->texture,0,0);
    if(bar->fill.ready)((MeshRenderTexturedFn)SEAM_Mesh_renderTextured)(bar->fill.words,bar->material,bar->texture,0,0);
}
