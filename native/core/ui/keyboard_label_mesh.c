#include "keyboard_label_mesh.h"
#include "ui_widgets.h"
#include "ui_text_style.h"
#include "../internal.h"

static UiMeshCache mesh;
static u32 texture[13];
static void *font;
static int texture_ready;

static const int *glyph(unsigned code){
    u8 *header=*(u8**)((u8*)font+0x2b4),*node,*candidate;
    if(!header)return 0;
    node=*(u8**)(header+4);candidate=header;
    while(node){
        if(*(unsigned*)(node+0x10)<code)node=*(u8**)(node+12);
        else{candidate=node;node=*(u8**)(node+8);}
    }
    return candidate!=header&&*(unsigned*)(candidate+0x10)==code?(int*)(candidate+0x14):0;
}

void keyboard_label_mesh_reset(void){
    ui_mesh_cache_reset(&mesh);
    if(texture_ready)((TexturePtrDtor)SEAM_TexturePtrDtor)(texture);
    zero(texture,sizeof(texture));texture_ready=0;font=0;
}

void keyboard_label_mesh_build(void *screen,void **buttons,unsigned count){
    unsigned i,j;
    void *image,*tess=(void*)SEAM_UiTessellator;
    float tw,th;
    keyboard_label_mesh_reset();font=ui_screen_font(screen);
    if(!font)return;
    ((TexturePathStateInit)SEAM_TexturePathStateInit)(texture,*(void**)((u8*)font+0x280),(u8*)font+0x26c,0);
    texture_ready=1;
    image=((void*(*)(void*))0x00714860u)(texture);
    if(!image)return;
    tw=(float)*(unsigned*)((u8*)image+4);th=(float)*(unsigned*)((u8*)image+8);
    if(tw<=0||th<=0)return;
    ((TessellatorBeginFn)SEAM_Tessellator_beginMaxVertices)(tess,2048);
    ((void(*)(void*,const Color*))0x001B1F00u)(tess,&ui_reference_text_color);
    for(i=0;i<count;i++){
        UiButtonView *button=(UiButtonView*)buttons[i];
        const char *label;
        int pen=0,left=0x7fffffff,right=-0x7fffffff,top=0x7fffffff,bottom=-0x7fffffff;
        float origin_x,origin_y;
        if(!button||!button->element.visible)continue;
        label=ui_button_label(button);if(!label||!label[0])continue;
        for(j=0;label[j];j++){
            const int *g=glyph((u8)label[j]);if(!g)continue;
            if(g[2]&&g[3]){
                if(pen+g[4]<left)left=pen+g[4];
                if(pen+g[4]+g[2]>right)right=pen+g[4]+g[2];
                if(g[5]<top)top=g[5];
                if(g[5]+g[3]>bottom)bottom=g[5]+g[3];
            }
            pen+=g[6];
        }
        if(left>right)continue;
        origin_x=(float)(button->element.x+(button->element.width-(right-left))/2-left);
        origin_y=(float)(button->element.y+(button->element.height-(bottom-top))/2-top);
        pen=0;
        for(j=0;label[j];j++){
            const int *g=glyph((u8)label[j]);
            float x,y,u,v,u2,v2;
            TessellatorVertexUvFn vertex=(TessellatorVertexUvFn)SEAM_Tessellator_vertexUVFloats;
            if(!g)continue;
            x=origin_x+(float)(pen+g[4]);y=origin_y+(float)g[5];
            u=(float)g[0]/tw;v=(float)g[1]/th;u2=(float)(g[0]+g[2])/tw;v2=(float)(g[1]+g[3])/th;
            if(g[2]&&g[3]){
                vertex(tess,x,y+(float)g[3],0,u,v2);
                vertex(tess,x+(float)g[2],y+(float)g[3],0,u2,v2);
                vertex(tess,x+(float)g[2],y,0,u2,v);
                vertex(tess,x,y,0,u,v);
            }
            pen+=g[6];
        }
    }
    ((TessellatorEndFn)SEAM_Tessellator_end)(mesh.words,tess,0,0);mesh.ready=1;
}

void keyboard_label_mesh_draw(const Color *color){
    static const Color white={1,1,1,1};
    (void)color;
    if(!mesh.ready||!font)return;
    ((GuiShaderColorFn)SEAM_GuiComponent_setShaderColor)((void*)SEAM_UiShaderColorGlobal,&white);
    ((MeshRenderTexturedFn)SEAM_Mesh_renderTextured)(mesh.words,(u8*)font+0x30,texture,0,0);
}
