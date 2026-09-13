#include "intellisense_item_renderer.h"
#include "../internal.h"

typedef void *(*ItemRendererGetInstanceFn)(void);
typedef void (__attribute__((pcs("aapcs-vfp"))) *ItemRendererRenderGuiItemFn)(void*,void*,int,int,float,float,float,float,float);

void intellisense_item_renderer_draw(const void *item_instance,float x,float y){
    void *renderer;if(!item_instance)return;renderer=((ItemRendererGetInstanceFn)SEAM_ItemRenderer_getInstance)();if(renderer)((ItemRendererRenderGuiItemFn)SEAM_ItemRenderer_renderGuiItemNew)(renderer,(void*)item_instance,0,0,x-9.0f,y-4.0f,1.0f,1.0f,0.6875f);
}
