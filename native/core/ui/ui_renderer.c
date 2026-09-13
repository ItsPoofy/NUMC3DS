#include "ui_renderer.h"

int ui_renderer_mesh_ready(const void *material){
    const u8 *bytes=(const u8*)material;
    void *shader_state;
    if(!bytes)return 0;
    shader_state=*(void * volatile *)(bytes+0x14);
    return shader_state&&*(void * volatile *)((u8*)shader_state+0x14)!=0;
}
