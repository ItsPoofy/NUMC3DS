#ifndef NUMC3DS_VIRTUAL_COMMAND_ORIGIN_H
#define NUMC3DS_VIRTUAL_COMMAND_ORIGIN_H

typedef struct {
    void *origin;
} NativeVirtualCommandOriginScope;

int native_virtual_command_origin_begin(NativeVirtualCommandOriginScope *scope,
    void *base_origin,void *entity,void *level,const int position[3]);
void native_virtual_command_origin_end(NativeVirtualCommandOriginScope *scope);
void *native_virtual_command_origin_get(const NativeVirtualCommandOriginScope *scope);

#endif
