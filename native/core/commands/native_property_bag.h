#ifndef NUMC3DS_NATIVE_PROPERTY_BAG_H
#define NUMC3DS_NATIVE_PROPERTY_BAG_H

#include "native_schema.h"

unsigned native_bag_collect_targets(void *origin,void *bag,const char *name,
    void **out,unsigned max);
typedef int (*NativeCommandTargetVisitor)(void *user,void *entity);
unsigned native_bag_visit_targets(void *origin,void *bag,const char *name,
    NativeCommandTargetVisitor visitor,void *user);
int native_bag_has(void *bag,const char *name);

/* Returns a borrowed pointer to a string stored in a CommandPropertyBag.
 * The pointer remains valid until the bag is mutated or destroyed. */
const char *native_bag_get_string(void *bag,const char *name);
int native_bag_get_int(void *bag,const char *name,int *value);
int native_bag_get_float(void *bag,const char *name,float *value);
int native_bag_get_bool(void *bag,const char *name,int *value);
int native_bag_get_rotation(void *bag,const char *name,float *value,int *relative);
int native_bag_get_blockpos(void *bag,const char *name,int value[3],int relative[3]);
int native_bag_value_not_empty(void *bag,const char *name,u8 parameter_type);
int native_bag_value_text(void *bag,const char *name,char *out,unsigned capacity);
void native_bag_set_string(void *bag,const char *name,const char *value);
void native_bag_set_int(void *bag,const char *name,int value);
void native_bag_set_float(void *bag,const char *name,float value);
void native_bag_set_bool(void *bag,const char *name,int value);
void native_bag_set_blockpos(void *bag,const char *name,const int value[3]);
void native_bag_set_result_list(void *bag,const char *name,const char *const *values,unsigned count);

#endif
