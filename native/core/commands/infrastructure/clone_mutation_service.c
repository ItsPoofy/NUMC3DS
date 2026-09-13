#include "../clone_mutation_service.h"
#include "../block_entity_transfer.h"
#include "../../internal.h"

typedef struct { int position[3]; } ClonePosition;
typedef struct { int destination[3]; unsigned char id,data; } CloneBlock;
typedef struct { int destination[3]; unsigned char id,data; void *tag; } CloneBlockEntity;
typedef struct { void *values; int count,capacity; unsigned stride; } CloneVector;

static void clone_vector_destroy(CloneVector *vector){
    if(vector->values)s->host.heap_free(vector->values);
    vector->values=0;vector->count=vector->capacity=0;
}

static int clone_vector_append(CloneVector *vector,const void *value){
    void *replacement;int capacity;
    if(vector->count==vector->capacity){
        capacity=vector->capacity?vector->capacity*2:16;
        if(capacity<vector->count+1)capacity=vector->count+1;
        replacement=s->host.heap_alloc((u32)capacity*vector->stride);
        if(!replacement)return 0;
        if(vector->values){cp(replacement,vector->values,(u32)vector->count*vector->stride);s->host.heap_free(vector->values);}
        vector->values=replacement;vector->capacity=capacity;
    }
    cp((unsigned char *)vector->values+(u32)vector->count*vector->stride,value,vector->stride);vector->count++;
    return 1;
}

static int clone_is_selected(const CloneMutationOptions *options,unsigned char id,unsigned char data){
    if(options->mask==CLONE_MUTATION_REPLACE)return 1;
    if(options->mask==CLONE_MUTATION_MASKED)return id!=0;
    return id==options->filter_id&&(!options->has_filter_data||data==options->filter_data);
}

static int clone_block_is_solid(unsigned char id){
    return ((const unsigned char *)SEAM_Block_solidById)[id]!=0;
}

static int clone_block_has_property(unsigned char id,unsigned property){
    void *block=((void **)SEAM_BlockRegistry_byNumericId)[id];
    return block&&((int (*)(void *,unsigned))SEAM_Block_hasProperty)(block,property);
}

static void clone_destroy_block_entity_tags(CloneVector *entities){
    CloneBlockEntity *records=(CloneBlockEntity *)entities->values;int index;
    for(index=0;index<entities->count;index++)block_entity_snapshot_destroy(records[index].tag);
}

static int clone_append_block(CloneVector *solid,CloneVector *non_solid,const CloneBlock *block){
    if(!clone_block_is_solid(block->id)&&!clone_block_has_property(block->id,0x200000u))return clone_vector_append(non_solid,block);
    return clone_vector_append(solid,block);
}

static int clone_apply_blocks(void *source,const CloneVector *blocks){
    const CloneBlock *records=(const CloneBlock *)blocks->values;int index,copied=0;
    for(index=0;index<blocks->count;index++)if(command_block_set(source,records[index].destination,records[index].id,records[index].data))copied++;
    return copied;
}

int command_clone_apply(void *source,const CommandRegion *region,const int destination[3],const CloneMutationOptions *options,int *copied_out){
    CloneVector solid={0,0,0,sizeof(CloneBlock)},entities={0,0,0,sizeof(CloneBlockEntity)},non_solid={0,0,0,sizeof(CloneBlock)},sources={0,0,0,sizeof(ClonePosition)};
    int x,y,z,copied=0,result=-1;
    if(copied_out)*copied_out=0;
    if(!source||!region||!destination||!options)goto done;
    for(z=region->minimum[2];z<=region->maximum[2];z++)for(y=region->minimum[1];y<=region->maximum[1];y++)for(x=region->minimum[0];x<=region->maximum[0];x++){
        ClonePosition source_position={{x,y,z}};CloneBlock block;void *tag=0;int snapshot;
        block.destination[0]=destination[0]+x-region->minimum[0];block.destination[1]=destination[1]+y-region->minimum[1];block.destination[2]=destination[2]+z-region->minimum[2];
        block.id=command_block_read_id(source,source_position.position);block.data=command_block_read_data(source,source_position.position);
        if(!clone_is_selected(options,block.id,block.data))continue;
        snapshot=block_entity_snapshot_save(source,source_position.position,&tag);
        if(snapshot<0)goto done;
        if(snapshot){CloneBlockEntity entity;cp(entity.destination,block.destination,sizeof(entity.destination));entity.id=block.id;entity.data=block.data;entity.tag=tag;if(!clone_vector_append(&entities,&entity)){block_entity_snapshot_destroy(tag);goto done;}}
        else if(!clone_append_block(&solid,&non_solid,&block))goto done;
        if(!clone_vector_append(&sources,&source_position))goto done;
    }
    if(options->move){
        ClonePosition *positions=(ClonePosition *)sources.values;int index;
        for(index=0;index<sources.count;index++)command_block_set(source,positions[index].position,0,0);
    }
    copied+=clone_apply_blocks(source,&solid);
    if(entities.count){
        CloneBlockEntity *records=(CloneBlockEntity *)entities.values;int index;
        for(index=0;index<entities.count;index++)if(command_block_set(source,records[index].destination,records[index].id,records[index].data))copied++;
        for(index=0;index<entities.count;index++){block_entity_snapshot_load(source,records[index].destination,records[index].tag);command_block_set(source,records[index].destination,records[index].id,records[index].data);}
    }
    copied+=clone_apply_blocks(source,&non_solid);
    result=copied;
done:
    clone_destroy_block_entity_tags(&entities);clone_vector_destroy(&sources);clone_vector_destroy(&non_solid);clone_vector_destroy(&entities);clone_vector_destroy(&solid);
    if(copied_out&&result>=0)*copied_out=result;
    return result;
}
