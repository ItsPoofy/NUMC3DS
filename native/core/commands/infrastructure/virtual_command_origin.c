#include "../virtual_command_origin.h"
#include "../../internal.h"

enum { COMMAND_ORIGIN_DATA_SIZE=0x20, ENTITY_COMMAND_ORIGIN_SIZE=0x38 };
typedef void (*OriginToDataFn)(void *,void *);
typedef void (*OriginDtorFn)(void *);
typedef void *(*OriginDataCtorFn)(void *);
typedef void *(*OriginDataCopyCtorFn)(void *,const void *);
typedef void (*OriginDataDtorFn)(void *);
typedef void (*OriginFromDataFn)(void **,const void *,void *);
typedef const u32 *(*EntityUniqueIdFn)(void *);

static void origin_destroy(void *origin){
    void **vtable=origin?*(void ***)origin:0;
    if(vtable&&vtable[1])((OriginDtorFn)vtable[1])(origin);
}

static void origin_to_data(void *data,void *origin){
    void **vtable=origin?*(void ***)origin:0;
    if(vtable&&vtable[19])((OriginToDataFn)vtable[19])(data,origin);
}

static void *origin_data_copy(const void *source){
    void *copy=((GameAllocWithSelector)SEAM_Heap_allocWithSelector)(COMMAND_ORIGIN_DATA_SIZE,(void*)SEAM_game_alloc_selector);
    return copy?((OriginDataCopyCtorFn)SEAM_CommandOriginData_copyCtor)(copy,source):0;
}

static void *entity_origin_create(void *entity,void *level){
    void *memory,*origin;const u32 *unique_id;
    memory=((GameAllocWithSelector)SEAM_Heap_allocWithSelector)(ENTITY_COMMAND_ORIGIN_SIZE,(void*)SEAM_game_alloc_selector);
    if(!memory)return 0;
    origin=((void *(*)(void *))SEAM_CommandOrigin_baseCtor)(memory);
    if(!origin){((void(*)(void*))SEAM_operator_delete)(memory);return 0;}
    *(u32*)origin=SEAM_EntityCommandOrigin_vtable;
    unique_id=((EntityUniqueIdFn)SEAM_Entity_getUniqueID)(entity);
    if(!unique_id){origin_destroy(origin);return 0;}
    *(u32*)((u8*)origin+0x28)=unique_id[0];
    *(u32*)((u8*)origin+0x2c)=unique_id[1];
    *(void**)((u8*)origin+0x30)=level;
    return origin;
}

int native_virtual_command_origin_begin(NativeVirtualCommandOriginScope *scope,
    void *base_origin,void *entity,void *level,const int position[3]){
    u8 base_data[COMMAND_ORIGIN_DATA_SIZE],entity_data[COMMAND_ORIGIN_DATA_SIZE],virtual_data[COMMAND_ORIGIN_DATA_SIZE];
    void *entity_origin=0,*base_copy=0,*entity_copy=0,*origin=0;
    if(!scope||!base_origin||!entity||!level||!position)return 0;
    scope->origin=0;zero(base_data,sizeof(base_data));zero(entity_data,sizeof(entity_data));zero(virtual_data,sizeof(virtual_data));
    ((OriginDataCtorFn)SEAM_CommandOriginData_ctor)(base_data);
    ((OriginDataCtorFn)SEAM_CommandOriginData_ctor)(entity_data);
    ((OriginDataCtorFn)SEAM_CommandOriginData_ctor)(virtual_data);
    entity_origin=entity_origin_create(entity,level);
    if(!entity_origin)goto cleanup;
    origin_to_data(base_data,base_origin);
    origin_to_data(entity_data,entity_origin);
    base_copy=origin_data_copy(base_data);entity_copy=origin_data_copy(entity_data);
    if(!base_copy||!entity_copy)goto cleanup;
    virtual_data[0]=8;
    *(void**)(virtual_data+8)=base_copy;base_copy=0;
    *(void**)(virtual_data+12)=entity_copy;entity_copy=0;
    *(int*)(virtual_data+16)=position[0];
    *(int*)(virtual_data+20)=position[1];
    *(int*)(virtual_data+24)=position[2];
    ((OriginFromDataFn)SEAM_CommandOrigin_fromCommandOriginData)(&origin,virtual_data,level);
    scope->origin=origin;
cleanup:
    if(base_copy){((OriginDataDtorFn)SEAM_CommandOriginData_dtor)(base_copy);((void(*)(void*))SEAM_operator_delete)(base_copy);}
    if(entity_copy){((OriginDataDtorFn)SEAM_CommandOriginData_dtor)(entity_copy);((void(*)(void*))SEAM_operator_delete)(entity_copy);}
    ((OriginDataDtorFn)SEAM_CommandOriginData_dtor)(virtual_data);
    ((OriginDataDtorFn)SEAM_CommandOriginData_dtor)(entity_data);
    ((OriginDataDtorFn)SEAM_CommandOriginData_dtor)(base_data);
    origin_destroy(entity_origin);
    return scope->origin!=0;
}

void native_virtual_command_origin_end(NativeVirtualCommandOriginScope *scope){
    if(!scope||!scope->origin)return;
    origin_destroy(scope->origin);scope->origin=0;
}

void *native_virtual_command_origin_get(const NativeVirtualCommandOriginScope *scope){return scope?scope->origin:0;}
