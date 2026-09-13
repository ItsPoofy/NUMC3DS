#include "../entity_rotation.h"
#include "../command_math.h"
#include "../../internal.h"

int command_entity_get_rotation(const void *entity,float rotation[2]){
    if(!entity||!rotation)return 0;
    rotation[0]=*(const float *)((const unsigned char *)entity+SEAM_Entity_xRotOffset);
    rotation[1]=*(const float *)((const unsigned char *)entity+SEAM_Entity_yRotOffset);
    return 1;
}

int command_entity_resolve_rotation(void *entity,const float requested[2],const int relative[2],const int provided[2],float resolved[2]){
    float x_rot,y_rot;
    if(!entity||!requested||!relative||!provided||!resolved)return 0;
    if(!command_entity_get_rotation(entity,resolved))return 0;
    x_rot=resolved[0];
    y_rot=resolved[1];
    resolved[0]=provided[0]?(relative[0]?x_rot+requested[0]:requested[0]):x_rot;
    resolved[1]=provided[1]?(relative[1]?y_rot+requested[1]:requested[1]):y_rot;
    resolved[0]=command_rotation_clamp_pitch(resolved[0]);
    resolved[1]=command_rotation_wrap_yaw(resolved[1]);
    return 1;
}
