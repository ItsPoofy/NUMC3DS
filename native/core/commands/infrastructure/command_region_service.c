#include "../command_region.h"

int command_region_from_bounds(const int first[3],const int second[3],CommandRegion *out){
    unsigned width,height,depth;
    int axis;
    if(!first||!second||!out)return 0;
    for(axis=0;axis<3;axis++){
        out->minimum[axis]=first[axis]<second[axis]?first[axis]:second[axis];
        out->maximum[axis]=first[axis]>second[axis]?first[axis]:second[axis];
    }
    width=(unsigned)(out->maximum[0]-out->minimum[0]+1);
    height=(unsigned)(out->maximum[1]-out->minimum[1]+1);
    depth=(unsigned)(out->maximum[2]-out->minimum[2]+1);
    out->volume=(int)(width*height*depth);
    return out->volume<0x8001;
}
