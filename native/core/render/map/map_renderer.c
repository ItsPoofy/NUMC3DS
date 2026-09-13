#include "map_renderer.h"
#include "map_update_batch.h"
#include "../../state.h"

static NuMC3DS_Hook dirty_hook;
typedef void(__attribute__((pcs("aapcs-vfp")))*MapShaderSetupFn)(void *,void *,const int *,Color,const float *);

static void on_map_set_dirty(void *data,int x,int y){
    ((void(*)(void*,int,int))dirty_hook.trampoline)(data,x,y);
    map_update_batch_notify(data);
}

static Color map_light_color(const int *position,void *source){
    static const signed char neighbors[7][3]={{0,0,0},{0,1,0},{0,-1,0},{0,0,-1},{0,0,1},{1,0,0},{-1,0,0}};
    u8 maximum[2]={0,0},sample[2],minimum=*(u8*)0x00A34726u;
    int p[3];
    unsigned i,j;
    for(i=0;i<7;i++){
        for(j=0;j<3;j++)p[j]=position[j]+neighbors[i][j];
        ((void(*)(u8*,void*,const int*,const u8*))0x00175F28u)(sample,source,p,&minimum);
        for(j=0;j<2;j++)if(sample[j]>maximum[j])maximum[j]=sample[j];
    }
    return ((Color(__attribute__((pcs("aapcs-vfp")))*)(const u8*))0x001D9440u)(maximum);
}

void map_renderer_render(void *renderer,const int *position,void *source,void *data,float partial,int in_frame,int hide_markers){
    const float *shader_vec=(const float*)*(u32*)0x003615D8u;
    Color shader_color={1.0f,1.0f,1.0f,1.0f};
    Color light;
    MapInstance *instance;
    (void)partial;
    ((MapShaderSetupFn)0x003BB7ACu)(renderer,source,position,shader_color,shader_vec);
    if(!((int(*)(void*))0x00718070u)((u8*)renderer+0x10c))
        ((void(*)(void*))0x0019A9E8u)(renderer);
    instance=map_renderer_get_instance(renderer,data);
    if(!instance)return;
    if(instance->dirty&&!map_instance_update_texture(instance))return;
    light=map_light_color(position,source);
    map_instance_draw(instance,&light,in_frame,hide_markers);
}

int map_renderer_install(void){
    int result;
    dirty_hook.target=0x00327858u;
    dirty_hook.expected[0]=0xE92D41F0u;
    dirty_hook.expected[1]=0xE1A04002u;
    dirty_hook.replacement=(u32)on_map_set_dirty;
    result=s->host.install_hook(&dirty_hook);
    if(result)return result;
    return map_update_batch_install();
}
