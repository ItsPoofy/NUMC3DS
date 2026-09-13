#include "../entity_spawn_service.h"
#include "../native_types.h"
#include "../../internal.h"

typedef struct {
    float yaw;
    float pitch;
} NativeVec2;

typedef void (*EntityFactoryCreateFn)(void **out, const void *def, void *source, const float *pos, const float *rot, int unk);
typedef void *(*LevelAddEntityFn)(void *level, void *source, void **entity_ptr);

int command_spawn_entity_by_id(void *level, void *source, int entity_id, const float position[3]) {
    u32 definition[3];
    void *entity = 0;
    const float rot[2] = {0.0f, 0.0f};
    u8 prev_bypass;

    if (!level || !source || !position || entity_id <= 0) {
        return 0;
    }

    ((EntityDefinitionInitByIdFn)SEAM_EntityDefinition_InitById)(definition, (unsigned int)entity_id);

    prev_bypass = *(u8*)SEAM_EntityFactory_CapacityBypass;
    *(u8*)SEAM_EntityFactory_CapacityBypass = 1;

    ((EntityFactoryCreateFn)SEAM_EntityFactory_Create)(&entity, definition, source, position, rot, 0);

    *(u8*)SEAM_EntityFactory_CapacityBypass = prev_bypass;

    if (!entity) {
        return 0;
    }

    ((LevelAddEntityFn)SEAM_Level_addEntity)(level, source, &entity);
    return 1;
}

int command_spawn_source_has_block(void *source, const int position[3]) {
    return source && position && ((void *(*)(void *, const int *))SEAM_BlockSource_getLevelChunkFromBlockPos)(source, position) != 0;
}
