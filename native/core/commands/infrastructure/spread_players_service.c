#include "../spread_players_service.h"
#include "../command_random.h"
#include "../entity_teleport.h"
#include "../block_mutation_service.h"
#include "../../internal.h"

typedef struct { float x,z; } SpreadPosition;
typedef void (*BlockSourceTopRainPosFn)(int *,void *,const int *);
typedef void *(*BlockSourceMaterialFn)(void *,const int *);
typedef int (*MaterialIsTypeFn)(void *,int);

static int floor_coordinate(float value){int whole=(int)value;return value<(float)whole?whole-1:whole;}

static float vector_length(float x,float z){
    float value=x*x+z*z,guess;
    unsigned index;
    if(value<=0.0f)return 0.0f;
    guess=value>1.0f?value:1.0f;
    for(index=0;index<8;index++)guess=0.5f*(guess+value/guess);
    return guess;
}

static void randomize_position(CommandRandom *random,SpreadPosition *position,float low_x,float low_z,float high_x,float high_z){
    position->x=command_random_local_next_float(random,low_x,high_x);
    position->z=command_random_local_next_float(random,low_z,high_z);
}

static int clamp_position(SpreadPosition *position,float low_x,float low_z,float high_x,float high_z){
    int changed=0;
    if(position->x<low_x){position->x=low_x;changed=1;}else if(position->x>high_x){position->x=high_x;changed=1;}
    if(position->z<low_z){position->z=low_z;changed=1;}else if(position->z>high_z){position->z=high_z;changed=1;}
    return changed;
}

static int position_is_safe(void *source,const SpreadPosition *position){
    int block_position[3],rain_position[3],ground_position[3];void *material;
    block_position[0]=floor_coordinate(position->x);block_position[1]=0;block_position[2]=floor_coordinate(position->z);
    ((BlockSourceTopRainPosFn)SEAM_BlockSource_getTopRainBlockPos)(rain_position,source,block_position);
    ground_position[0]=rain_position[0];ground_position[1]=rain_position[1]-1;ground_position[2]=rain_position[2];
    material=((BlockSourceMaterialFn)SEAM_BlockSource_getMaterialXYZ)(source,ground_position);
    return command_block_read_id(source,rain_position)!=0&&!((MaterialIsTypeFn)SEAM_Material_isType)(material,5);
}

static int spread_positions(CommandRandom *random,void *source,SpreadPosition *positions,unsigned count,float spread_distance,float low_x,float low_z,float high_x,float high_z,SpreadPlayersResult *result){
    unsigned iteration,index,other;
    if(result){result->minimum_distance=0.0f;result->iterations=0;}
    for(iteration=0;iteration<10000;iteration++){
        int changed=0;
        for(index=0;index<count;index++){
            float sum_x=0.0f,sum_z=0.0f;unsigned nearby=0;
            for(other=0;other<count;other++)if(other!=index){
                float dx=positions[other].x-positions[index].x,dz=positions[other].z-positions[index].z,distance=vector_length(dx,dz);
                if(distance<spread_distance){sum_x+=dx;sum_z+=dz;nearby++;}
                if(result&&(result->minimum_distance==0.0f||distance<result->minimum_distance))result->minimum_distance=distance;
            }
            if(nearby){
                float length;
                sum_x/=(float)nearby;sum_z/=(float)nearby;length=vector_length(sum_x,sum_z);
                if(length>0.0f){positions[index].x-=sum_x/length;positions[index].z-=sum_z/length;}
                else randomize_position(random,&positions[index],low_x,low_z,high_x,high_z);
                changed=1;
            }
            if(clamp_position(&positions[index],low_x,low_z,high_x,high_z))changed=1;
        }
        if(!changed){
            for(index=0;index<count;index++)if(!position_is_safe(source,&positions[index])){randomize_position(random,&positions[index],low_x,low_z,high_x,high_z);changed=1;}
            if(!changed){if(result)result->iterations=iteration;return 1;}
        }
    }
    if(result)result->iterations=10000;
    return 0;
}

int command_spread_players(void *level,void *source,void *const *targets,unsigned target_count,float center_x,float center_z,float spread_distance,float max_range,SpreadPlayersResult *result){
    CommandRandom random;SpreadPosition positions[SELECTOR_TARGET_CAPACITY];float low_x=center_x-max_range,low_z=center_z-max_range,high_x=center_x+max_range,high_z=center_z+max_range;unsigned index;
    if(!level||!source||!targets||!target_count||target_count>SELECTOR_TARGET_CAPACITY)return 0;
    if(!command_random_construct(&random))return 0;
    for(index=0;index<target_count;index++)randomize_position(&random,&positions[index],low_x,low_z,high_x,high_z);
    if(!spread_positions(&random,source,positions,target_count,spread_distance,low_x,low_z,high_x,high_z,result))return 0;
    for(index=0;index<target_count;index++){
        int block_position[3]={floor_coordinate(positions[index].x),0,floor_coordinate(positions[index].z)};
        float position[3];
        position[0]=(float)block_position[0]+0.5f;
        position[1]=(float)((int(*)(void*,const int*,int,int))SEAM_BlockSource_getAboveTopSolidBlock)(source,block_position,0,0)+1.0f;
        position[2]=(float)block_position[2]+0.5f;
        if(!native_entity_teleport(targets[index],position))return 0;
    }
    return 1;
}
