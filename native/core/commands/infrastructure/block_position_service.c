#include "../block_position_service.h"
#include "../../rt.h"

void command_blockpos_to_string(const int position[3],char *out,unsigned capacity){
    unsigned length=0;
    if(!out||!capacity)return;
    out[0]=0;
    if(!position)return;
    append(out,&length,"Pos(");
    append_int(out,&length,position[0]);
    append(out,&length,",");
    append_int(out,&length,position[1]);
    append(out,&length,",");
    append_int(out,&length,position[2]);
    append(out,&length,")");
}
