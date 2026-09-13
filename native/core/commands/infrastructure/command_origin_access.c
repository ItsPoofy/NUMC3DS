#include "../command_origin_access.h"
#include "../../internal.h"

typedef void *(*OriginPointerFn)(void *);
typedef void (*OriginBlockPositionFn)(int *,void *);
typedef struct {float x,y,z;} OriginWorldPosition;
typedef OriginWorldPosition (__attribute__((pcs("aapcs-vfp"))) *OriginWorldPositionFn)(void *);
typedef void (*OriginNameFn)(u32 *,void *);

static void **origin_vtable(void *origin){return origin?*(void ***)origin:0;}

void *native_command_origin_level(void *origin){
    void **vtable=origin_vtable(origin);
    return vtable&&vtable[6]?((OriginPointerFn)vtable[6])(origin):0;
}

void *native_command_origin_entity(void *origin){
    void **vtable=origin_vtable(origin);
    return vtable&&vtable[7]?((OriginPointerFn)vtable[7])(origin):0;
}

void *native_command_origin_block_source(void *origin){
    void **vtable=origin_vtable(origin);
    return vtable&&vtable[8]?((OriginPointerFn)vtable[8])(origin):0;
}

int native_command_origin_block_position(void *origin,int position[3]){
    void **vtable=origin_vtable(origin);
    if(!vtable||!vtable[4]||!position)return 0;
    ((OriginBlockPositionFn)vtable[4])(position,origin);return 1;
}

int native_command_origin_world_position(void *origin,float position[3]){
    void **vtable=origin_vtable(origin);OriginWorldPosition result;
    if(!vtable||!vtable[5]||!position)return 0;
    result=((OriginWorldPositionFn)vtable[5])(origin);
    position[0]=result.x;position[1]=result.y;position[2]=result.z;return 1;
}

int native_command_origin_name(void *origin,char *out,unsigned capacity){
    void **vtable=origin_vtable(origin);u32 value=0;const char *text=0;
    if(!out||!capacity)return 0;out[0]=0;
    if(!vtable||!vtable[3])return 0;
    ((OriginNameFn)vtable[3])(&value,origin);if(!value)return 0;cp(&text,&value,sizeof(text));
    if(text)copy_text(out,text,capacity-1);
    ((StrDtor)SEAM_StrDtor)(&value);
    return out[0]!=0;
}
