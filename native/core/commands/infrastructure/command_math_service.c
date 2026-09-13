#include "../command_math.h"

extern float fmodf(float,float);

static int command_math_errno;

int *__errno(void){return &command_math_errno;}

float command_rotation_clamp_pitch(float value){
    if(value<-90.0f)return -90.0f;
    if(value>90.0f)return 90.0f;
    return value;
}

float command_rotation_wrap_yaw(float value){
    value=fmodf(value,360.0f);
    if(value>=180.0f)value-=360.0f;
    if(value<-180.0f)value+=360.0f;
    return value;
}
