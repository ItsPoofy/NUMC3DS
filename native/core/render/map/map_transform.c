#include "map_transform.h"
#include "../../util/string_util.h"

static float *matrix(MapTransform *transform){
    return ((float *(*)(void*))0x0019B7A8u)(transform);
}

void map_transform_push(MapTransform *transform){
    ((void(*)(void*,void*))0x0019B9DCu)(transform,*(void**)0x00A358B8u);
}

void map_transform_pop(MapTransform *transform){
    ((void(*)(void*))0x0019B5C4u)(transform);
}

void map_transform_translate(MapTransform *transform,float x,float y,float z){
    float *m=matrix(transform);
    unsigned i;
    for(i=0;i<4;i++)m[12+i]+=m[i]*x+m[4+i]*y+m[8+i]*z;
}

void map_transform_scale(MapTransform *transform,float scale){
    float *m=matrix(transform);
    unsigned i;
    for(i=0;i<12;i++)m[i]*=scale;
}

void map_transform_rotate(MapTransform *transform,float degrees,float x,float y,float z){
    float result[16],axis[3]={x,y,z};
    float *m=matrix(transform);
    ((void(*)(void*,const void*,const float*,const float*))0x0011E650u)(result,m,&degrees,axis);
    cp(m,result,sizeof(result));
}
