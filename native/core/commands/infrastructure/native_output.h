#ifndef NUMC3DS_NATIVE_OUTPUT_H
#define NUMC3DS_NATIVE_OUTPUT_H

#include "../rt.h"

void native_output_set_status(void *bag,u32 status_code,const char *message);
const char *native_output_message(void *bag);
u32 native_command_result_success(void);
u32 native_command_result_error(void);
u32 native_command_result_not_found(void);
u32 native_command_result_permission(void);
u32 native_command_status_code(u32 result);

#endif
