#include "../native_output.h"
#include "../native_property_bag.h"

typedef void *(*JsonValueIntCtorFn)(void *,int);
typedef void *(*JsonValueStringCtorFn)(void *,void *);
typedef void *(*JsonValueMemberFn)(void *,void *);
typedef void *(*JsonValueAssignFn)(void *,void *);
typedef void (*JsonValueDtorFn)(void *);

static u32 result_from_address(u32 address){
    volatile u8 *bytes;
    if(address<0x00100000u||address>=0x01000000u)return 1u;
    bytes=(volatile u8 *)address;
    return (u32)bytes[0]|((u32)bytes[1]<<8)|((u32)bytes[2]<<16)|((u32)bytes[3]<<24);
}

u32 native_command_result_success(void){return result_from_address(SEAM_CommandResult_success);}
u32 native_command_result_error(void){return result_from_address(SEAM_CommandResult_syntax);}
u32 native_command_result_not_found(void){return result_from_address(SEAM_CommandResult_notFound);}
u32 native_command_result_permission(void){return result_from_address(SEAM_CommandResult_permission);}
u32 native_command_status_code(u32 result){
    u32 status=(result>>16)&0xffffu;
    status|=((result>>8)&0xffu)<<16;
    if((result&0xffu)==0)status|=0x80000000u;
    return status;
}

static void set_json_value(void*bag,const char*name,void*value){
    u32 key=0,key_scratch=0;void*slot;
    if(!bag||!name||!value)return;
    ((StrCtor)SEAM_StrCtor)(&key,name,&key_scratch);
    slot=((JsonValueMemberFn)SEAM_Json_Value_getOrCreate)((u8*)bag+SEAM_CommandPropertyBag_jsonOffset,&key);
    if(slot)((JsonValueAssignFn)SEAM_Json_Value_assign)(slot,value);
    ((StrDtor)SEAM_StrDtor)(&key);
}

void native_output_set_status(void*bag,u32 status_code,const char*message){
    NativeJsonValue value;u32 string=0,string_scratch=0;
    if(!bag)return;
    zero(&value,sizeof(value));
    ((JsonValueIntCtorFn)SEAM_Json_Value_intCtor)(&value,(int)status_code);
    set_json_value(bag,"statusCode",&value);
    ((JsonValueDtorFn)SEAM_Tag_destructByType)(&value);
    if(message){
        zero(&value,sizeof(value));
        ((StrCtor)SEAM_StrCtor)(&string,message,&string_scratch);
        ((JsonValueStringCtorFn)SEAM_Json_Value_stdStringCtor)(&value,&string);
        set_json_value(bag,"statusMessage",&value);
        ((JsonValueDtorFn)SEAM_Tag_destructByType)(&value);
        ((StrDtor)SEAM_StrDtor)(&string);
    }
}

const char *native_output_message(void*bag){return native_bag_get_string(bag,"statusMessage");}
