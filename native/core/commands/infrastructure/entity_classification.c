#include "../entity_classification.h"
#include "../../internal.h"

typedef int (*EntityClassTreeIsInstanceOfFn)(void *,unsigned);

int command_entity_is_type(void *entity,unsigned type){return entity&&((EntityClassTreeIsInstanceOfFn)SEAM_EntityClassTree_isInstanceOf)(entity,type)!=0;}
int command_entity_is_player(void *entity){return command_entity_is_type(entity,0x013f);}
int command_entity_is_mob(void *entity){return command_entity_is_type(entity,0x0100);}
