#include "gamerule_service.h"
#include "../util/string_util.h"
#include "../util/math_util.h"
#include "../rt.h"
#include "../commands/command_error_service.h"
#include "../entity/selector_service.h"

void* rule_dfs(void* node, const char* name) {
    char* key;
    void* r;
    if (!node) return 0;
    key = *(char**)((unsigned char*)node + 0x10);
    if (key && name_ieq(key, name)) return (unsigned char*)node + 0x14;
    r = rule_dfs(*(void**)((unsigned char*)node + 8), name);
    return r ? r : rule_dfs(*(void**)((unsigned char*)node + 0xc), name);
}

void rule_print(char* out, unsigned* len, void* rule) {
    unsigned char type = *((unsigned char*)rule + 2);
    if (type == 1) {
        append(out, len, *(unsigned char*)((unsigned char*)rule + 4) ? "true" : "false");
    } else if (type == 2) {
        float f = *(float*)((unsigned char*)rule + 4);
        append_int(out, len, (int)f);
    } else {
        append_int(out, len, *(int*)((unsigned char*)rule + 4));
    }
}

void rule_walk(void* node, char* out, unsigned* len) {
    char* key;
    if (!node) return;
    rule_walk(*(void**)((unsigned char*)node + 8), out, len);
    key = *(char**)((unsigned char*)node + 0x10);
    if (key) {
        append(out, len, key);
        append(out, len, "=");
        rule_print(out, len, (unsigned char*)node + 0x14);
        append(out, len, " ");
    }
    rule_walk(*(void**)((unsigned char*)node + 0xc), out, len);
}

void* rules_root(void* rules) {
    void* header = *(void**)((unsigned char*)rules + 0x10);
    return header ? *(void**)((unsigned char*)header + 4) : 0;
}

int gamerule_set(void* rules, const char* name, const char* value) {
    void* rule = rule_dfs(rules_root(rules), name);
    RuleSetBoolFn sb = (RuleSetBoolFn)0x006511E8u;
    RuleSetIntFn si = (RuleSetIntFn)0x00651184u;
    RuleSetFloatFn sf = (RuleSetFloatFn)0x0065124Cu;
    unsigned char type;
    if (!rule) return 0;
    type = *((unsigned char*)rule + 2);
    if (type == 1) {
        sb(rule, streq(value, "true") || streq(value, "1"));
    } else if (type == 2) {
        const char* q = value;
        float f = 0;
        decimal(&q, 0, &f);
        sf(rule, f);
    } else {
        const char* q = value;
        int v = 0;
        if (!integer(&q, &v) || *q) return 0;
        si(rule, v);
    }
    return 1;
}

int gamerule_query_one(void* rules, const char* name) {
    void* rule = rule_dfs(rules_root(rules), name);
    char out[MAX_TEXT + 1];
    unsigned n = 0;
    if (!rule) return 0;
    out[0] = 0;
    append(out, &n, name);
    append(out, &n, " = ");
    rule_print(out, &n, rule);
    result_text(out);
    return 1;
}

int rule_value_bool(void* rule) {
    if (*((unsigned char*)rule + 2) == 1) return *(unsigned char*)((unsigned char*)rule + 4) != 0;
    return *(int*)((unsigned char*)rule + 4) != 0;
}
