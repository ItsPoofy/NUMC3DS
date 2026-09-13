#ifndef NUMC3DS_GAMERULE_SERVICE_H
#define NUMC3DS_GAMERULE_SERVICE_H

#include "../../include/numc3ds_abi.h"

void* rule_dfs(void* node, const char* name);
void rule_print(char* out, unsigned* len, void* rule);
void rule_walk(void* node, char* out, unsigned* len);
void* rules_root(void* rules);
int gamerule_set(void* rules, const char* name, const char* value);
int gamerule_query_one(void* rules, const char* name);
int rule_value_bool(void* rule);

#endif /* NUMC3DS_GAMERULE_SERVICE_H */
