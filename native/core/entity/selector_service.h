#ifndef NUMC3DS_SELECTOR_SERVICE_H
#define NUMC3DS_SELECTOR_SERVICE_H

#include "../../include/numc3ds_abi.h"
#include "../rt.h"

#ifndef SELECTOR_TARGET_CAPACITY
#define SELECTOR_TARGET_CAPACITY 128u
#endif

int self_target(const char* p);
float entity_pos(void* e, int axis);
float pos_base(void* player, int axis);
void entity_name(void* e, char* out, unsigned cap);
int name_ieq(const char* a, const char* b);
float entity_dist2(void* e, void* ref);
unsigned collect_targets(void* executor, void* level, const char* token, void** out, unsigned max);
unsigned collect_world_entities(void* level, void** out, unsigned max);
unsigned collect_entities(void* player, void* level, void* game, const char* token, void** out, unsigned max);
unsigned resolve_selector(void* executor, void* level, void* game, const char* token, void** out, unsigned max);
unsigned command_collect_targets(void* executor, void* level, const char* token, void** out, unsigned max);
unsigned command_resolve_targets(void* executor, void* level, void* game, const char* token, void** out, unsigned max);
int table_lookup(const NameId* table, unsigned count, const char* name);

extern const NameId effect_table[];
extern const NameId enchant_table[];
extern const NameId entity_table[];

#endif /* NUMC3DS_SELECTOR_SERVICE_H */
