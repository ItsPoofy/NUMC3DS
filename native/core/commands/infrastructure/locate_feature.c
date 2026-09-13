#include "../locate_feature.h"
#include "../../internal.h"

typedef unsigned char (*FeatureIdFn)(void *);
typedef int (*FindNearestFeatureFn)(void *,unsigned char,void *,const int *,int *);

int locate_feature_find(void *source,void *level,const int origin[3],const char *feature,int destination[3]){
    void *dimension,*chunk_source;u32 feature_string=0;unsigned char feature_id;
    if(!source||!level||!origin||!destination||!feature||!*feature)return 0;
    dimension=((GetDimensionFn)SEAM_Level_getDimension)(level,0);if(!dimension)return 0;
    chunk_source=*(void **)((u8 *)dimension+SEAM_Dimension_chunkSourceOffset);if(!chunk_source)return 0;
    make_str(&feature_string,feature);feature_id=((FeatureIdFn)SEAM_ChunkSource_getFeatureId)(&feature_string);drop_str(&feature_string);if(feature_id==0xffu)return 0;
    zero(destination,sizeof(int)*3);return ((FindNearestFeatureFn)SEAM_ChunkSource_findNearestFeature)(chunk_source,feature_id,source,origin,destination)!=0;
}
