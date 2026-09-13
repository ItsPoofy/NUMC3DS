#include "command_enum_registry.h"
#include "../../entity_type_enum.h"
#include "../../internal.h"

typedef void (*NativeEnumProvider)(void *);
typedef void *(*VectorAllocateFn)(u32,u32);
typedef void (*VectorDeallocateFn)(void*,u32,int);

static const char *const gamerules=
    "commandBlockOutput "
    "doDaylightCycle "
    "doEntityDrops "
    "doFireTick "
    "doMobLoot "
    "doMobSpawning "
    "doTileDrops "
    "doWeatherCycle "
    "drowningDamage "
    "fallDamage "
    "fireDamage "
    "keepInventory "
    "mobGriefing "
    "pvp "
    "sendcommandfeedback sendCommandFeedback "
    "globalmute globalMute "
    "allowdestructiveobjects allowDestructiveObjects "
    "allowmobs allowMobs";

static NativeEnumProvider provider_for(const char *name){
    if(streq(name,"itemType"))return (NativeEnumProvider)SEAM_CommandParser_getItemTypeNames;
    if(streq(name,"blockType"))return (NativeEnumProvider)SEAM_CommandParser_getBlockTypeNames;
    if(streq(name,"featureType"))return (NativeEnumProvider)SEAM_CommandParser_getFeatureTypes;
    if(streq(name,"blockSlotType"))return (NativeEnumProvider)SEAM_CommandParser_getBlockSlotTypes;
    if(streq(name,"entitySlotType"))return (NativeEnumProvider)SEAM_CommandParser_getEntitySlotTypes;
    if(streq(name,"enchantmentType"))return (NativeEnumProvider)SEAM_CommandParser_getEnchantmentTypes;
    if(streq(name,"effectType"))return (NativeEnumProvider)SEAM_CommandParser_getEffectTypes;
    return 0;
}

static unsigned value_count(const char *values){
    unsigned count=0;const char *at=values?values:"";
    while(*at){if(*at!=' '&&(at==values||at[-1]==' '))count++;at++;}
    return count;
}

static int vector_has(NativeVector *vector,const char *target){
    NativeGstdString *at=(NativeGstdString*)vector->begin,*end=(NativeGstdString*)vector->end;
    while(at&&at!=end){if(at->handle&&streq((const char*)at->handle,target))return 1;at++;}
    return 0;
}

static int vector_append(NativeVector *vector,const char *text){
    unsigned count=(vector->end-vector->begin)/sizeof(NativeGstdString),capacity=count+1;
    NativeGstdString *values=(NativeGstdString*)((VectorAllocateFn)SEAM_gstd_allocator_allocate)(capacity*sizeof(*values),0);
    u32 scratch=0;
    if(!values)return 0;
    zero(values,capacity*sizeof(*values));
    ((StrCtor)SEAM_StrCtor)(&values[count],text,&scratch);
    if(!values[count].handle){((VectorDeallocateFn)SEAM_gstd_allocator_deallocate)(values,capacity,0);return 0;}
    if(count&&vector->begin){cp(values,(void*)vector->begin,count*sizeof(*values));((VectorDeallocateFn)SEAM_gstd_allocator_deallocate)((void*)vector->begin,(vector->capacity-vector->begin)/sizeof(*values),0);}
    vector->begin=(u32)values;vector->end=(u32)(values+capacity);vector->capacity=vector->end;
    return 1;
}

int command_enum_available(const NativeSchemaParameter *schema){
    const char *name;
    if(!schema)return 0;
    if(native_schema_text(schema->enum_values)[0])return 1;
    name=native_schema_text(schema->enum_name);
    return provider_for(name)!=0||streq(name,"entityType")||streq(name,"gameRuleTypes");
}

int command_enum_build_vector(const NativeSchemaParameter *schema,NativeVector *output){
    const char *name,*at;NativeEnumProvider provider;unsigned count,index=0,length;NativeGstdString *values;
    if(!output)return 0;
    zero(output,sizeof(*output));if(!schema)return 0;
    name=native_schema_text(schema->enum_name);provider=provider_for(name);
    if(provider){provider(output);if(streq(name,"itemType")&&!vector_has(output,"empty_map"))return vector_append(output,"empty_map");return 1;}
    if(streq(name,"entityType")){entity_type_build_enum(output);return output->begin!=0;}
    at=native_schema_text(schema->enum_values);if(!at[0]&&streq(name,"gameRuleTypes"))at=gamerules;
    count=value_count(at);if(!count)return 0;
    values=(NativeGstdString*)((VectorAllocateFn)SEAM_gstd_allocator_allocate)(count*sizeof(*values),0);if(!values)return 0;zero(values,count*sizeof(*values));
    while(*at&&index<count){char token[128];u32 scratch=0;length=0;while(*at==' ')at++;while(*at&&*at!=' '){if(length+1>=sizeof(token))goto failed;token[length++]=*at++;}token[length]=0;if(length){((StrCtor)SEAM_StrCtor)(&values[index],token,&scratch);if(!values[index].handle)goto failed;index++;}}
    output->begin=(u32)values;output->end=(u32)(values+index);output->capacity=(u32)(values+count);return 1;
failed:
    while(index)if(values[--index].handle)((StrDtor)SEAM_StrDtor)(&values[index]);
    ((VectorDeallocateFn)SEAM_gstd_allocator_deallocate)(values,count,0);return 0;
}

void command_enum_visit(const NativeSchemaParameter *schema,CommandEnumVisitor visitor,void *context){
    NativeVector vector;NativeGstdString *at,*end;
    if(!visitor||!command_enum_build_vector(schema,&vector))return;
    at=(NativeGstdString*)vector.begin;end=(NativeGstdString*)vector.end;
    while(at&&at!=end){if(at->handle)visitor((const char*)at->handle,context);((StrDtor)SEAM_StrDtor)(at++);}
    if(vector.begin)((VectorDeallocateFn)SEAM_gstd_allocator_deallocate)((void*)vector.begin,(vector.capacity-vector.begin)/sizeof(NativeGstdString),0);
}
