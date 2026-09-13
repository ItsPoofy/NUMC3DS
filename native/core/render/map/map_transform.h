#ifndef NUMC3DS_MAP_TRANSFORM_H
#define NUMC3DS_MAP_TRANSFORM_H
#include "../../rt.h"

typedef struct {u32 words[2];} MapTransform;
void map_transform_push(MapTransform *transform);
void map_transform_pop(MapTransform *transform);
void map_transform_translate(MapTransform *transform,float x,float y,float z);
void map_transform_scale(MapTransform *transform,float scale);
void map_transform_rotate(MapTransform *transform,float degrees,float x,float y,float z);
#endif
