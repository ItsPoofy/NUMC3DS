#include "selector_service.h"
#include "../util/string_util.h"
#include "../util/math_util.h"
#include "../world/world_block_service.h"
#include "../rt.h"
#include "../state.h"

int self_target(const char* p) {
    char target[16];
    if (!*p) return 1;
    p = word(p, target, sizeof(target));
    return !*p && streq(target, "@p");
}

float pos_base(void* player, int axis) {
    return entity_pos(player, axis);
}

static void *level_players(void* level, void*** endOut) {
    void** begin = *(void***)((unsigned char*)level + 0x18);
    *endOut = *(void***)((unsigned char*)level + 0x1c);
    return begin;
}

static const NameId entity_type_names[] = {
    {"chicken", 0x130A}, {"cow", 0x130B}, {"pig", 0x130C}, {"sheep", 0x130D},
    {"wolf", 0x530E}, {"villager", 0x030F}, {"mooshroom", 0x1310}, {"squid", 0x2311},
    {"rabbit", 0x1312}, {"bat", 0x8113}, {"iron_golem", 0x0314}, {"snow_golem", 0x0315},
    {"ocelot", 0x5316}, {"horse", 0x205317}, {"llama", 0x131D}, {"polar_bear", 0x131C},
    {"donkey", 0x205318}, {"mule", 0x205319}, {"skeleton_horse", 0x215B1A},
    {"zombie_horse", 0x215B1B}, {"zombie", 0x030B20}, {"creeper", 0x0B21},
    {"skeleton", 0x110B22}, {"spider", 0x040B23}, {"zombie_pigman", 0x010B24},
    {"slime", 0x0B25}, {"enderman", 0x0B26}, {"silverfish", 0x040B27},
    {"cave_spider", 0x040B28}, {"ghast", 0x0B29}, {"magma_cube", 0x0B2A},
    {"blaze", 0x0B2B}, {"zombie_villager", 0x030B2C}, {"witch", 0x0B2D},
    {"stray", 0x110B2E}, {"husk", 0x030B2F}, {"wither_skeleton", 0x110B30},
    {"guardian", 0x0B31}, {"elder_guardian", 0x0B32}, {"wither", 0x010B34},
    {"ender_dragon", 0x0B35}, {"shulker", 0x0B36}, {"endermite", 0x040B37},
    {"vindicator", 0x0B39}, {"player", 0x013F}, {"item", 0x0040}, {"tnt", 0x0041},
    {"falling_block", 0x0042}, {"moving_block", 0x0043}, {"xp_bottle", 0x400044},
    {"xp_orb", 0x0045}, {"eye_of_ender_signal", 0x0046}, {"ender_crystal", 0x0047},
    {"shulker_bullet", 0x40004C}, {"fishing_hook", 0x004D},
    {"dragon_fireball", 0x40004F}, {"arrow", 0x400050}, {"snowball", 0x400051},
    {"egg", 0x400052}, {"painting", 0x400053}, {"minecart", 0x080054},
    {"fireball", 0x400055}, {"splash_potion", 0x400056}, {"ender_pearl", 0x400057},
    {"leash_knot", 0x400058}, {"wither_skull", 0x400059}, {"boat", 0x005A},
    {"wither_skull_dangerous", 0x005B}, {"lightning_bolt", 0x005D},
    {"small_fireball", 0x40005E}, {"area_effect_cloud", 0x005F},
    {"hopper_minecart", 0x080060}, {"tnt_minecart", 0x080061},
    {"chest_minecart", 0x080062}, {"command_block_minecart", 0x080064},
    {"lingering_potion", 0x400065}, {"llama_spit", 0x400066},
    {"evocation_fang", 0x400067}, {"evocation_illager", 0x0B68}, {"vex", 0x0B69}
};

static void entity_type_name(void* e, char* out, unsigned cap) {
    typedef u32 (*GetEntityTypeFn)(void*);
    u32 type = ((GetEntityTypeFn)(*(void***)e)[0x1d4 / 4])(e);
    unsigned i;
    for (i = 0; i < sizeof(entity_type_names) / sizeof(entity_type_names[0]); i++) {
        if ((u32)entity_type_names[i].id == type) {
            copy_text(out, entity_type_names[i].name, cap - 1);
            return;
        }
    }
    copy_text(out, "Entity", cap - 1);
}

void entity_name(void* e, char* out, unsigned cap) {
    CopyEntityName copy_name = (CopyEntityName)0x00724730u;
    numc3ds_u32 h = 0;
    char* raw;
    copy_name(&h, e);
    raw = *(char**)&h;
    if (raw && raw[0]) copy_text(out, raw, cap - 1);
    else entity_type_name(e, out, cap);
    drop_str(&h);
}

int name_ieq(const char* a, const char* b) {
    while (*a && *b) {
        char ca = *a, cb = *b;
        if (ca >= 'A' && ca <= 'Z') ca += 32;
        if (cb >= 'A' && cb <= 'Z') cb += 32;
        if (ca != cb) return 0;
        a++;
        b++;
    }
    return *a == *b;
}

float entity_pos(void* e, int axis) {
    if (s && s->exec_active && s->exec_entity == e) return s->exec_pos[axis];
    return *(float*)((unsigned char*)e + SEAM_Entity_posOffset + axis * 4);
}

unsigned collect_targets(void* executor, void* level, const char* token, void** out, unsigned max) {
    void **begin, **end, **p;
    unsigned count = 0, i, best;
    float bd, dx, dy, dz, dd;
    if (!token[0]) return 0;
    begin = level_players(level, &end);
    if (!streq(token, "@a") && !streq(token, "@p") && !streq(token, "@r") && !streq(token, "@e")) {
        for (p = begin; p && p < end; p++) {
            char nm[32];
            entity_name(*p, nm, sizeof(nm));
            if (name_ieq(nm, token) && count < max) out[count++] = *p;
        }
        return count;
    }
    if (streq(token, "@p")) {
        best = 0xffffffffu;
        bd = 1e30f;
        for (i = 0, p = begin; p && p < end; p++, i++) {
            dx = entity_pos(*p, 0) - entity_pos(executor, 0);
            dy = entity_pos(*p, 1) - entity_pos(executor, 1);
            dz = entity_pos(*p, 2) - entity_pos(executor, 2);
            dd = dx * dx + dy * dy + dz * dz;
            if (dd < bd) {
                bd = dd;
                best = i;
            }
        }
        if (best != 0xffffffffu && best < max) {
            out[count] = ((void**)begin)[best];
            count++;
        }
        return count;
    }
    if (streq(token, "@r")) {
        unsigned total = 0;
        for (p = begin; p && p < end; p++) total++;
        if (total) {
            unsigned pick = rng_next() % total;
            out[count++] = ((void**)begin)[pick];
        }
        return count;
    }
    for (p = begin; p && p < end; p++) if (count < max) out[count++] = *p;
    return count;
}

static unsigned collect_entity_tree(void* header, void** out, unsigned max) {
    void* stack[64];
    void* node;
    unsigned depth = 0, count = 0, visited = 0;
    if (!header || !out || !max) return 0;
    node = *(void**)((unsigned char*)header + 4);
    while ((node || depth) && count < max && visited < 1024u) {
        while (node && node != header) {
            if (depth >= sizeof(stack) / sizeof(stack[0])) return count;
            stack[depth++] = node;
            node = *(void**)((unsigned char*)node + 8);
        }
        if (!depth) break;
        node = stack[--depth];
        {
            void* entity = *(void**)((unsigned char*)node + 0x18);
            if (entity) out[count++] = entity;
        }
        node = *(void**)((unsigned char*)node + 0xc);
        visited++;
    }
    return count;
}

unsigned collect_world_entities(void* level, void** out, unsigned max) {
    void *source, *region_owner, *header;
    if (!level || !out || !max) return 0;
    source = world_source(level);
    if (!source) return 0;
    region_owner = *(void**)((unsigned char*)source + 0x14);
    if (!region_owner) return 0;
    header = *(void**)((unsigned char*)region_owner + 0xf4);
    return collect_entity_tree(header, out, max);
}

unsigned collect_entities(void* player, void* level, void* game, const char* token, void** out, unsigned max) {
    unsigned n = 0, i, j;
    void* tv[48];
    (void)game;
    if (!streq(token, "@e")) return 0;
    j = collect_targets(player, level, "@a", tv, (unsigned)(sizeof(tv) / sizeof(tv[0])));
    if (j > max) j = max;
    n = collect_world_entities(level, out, max - j);
    for (i = 0; i < j && n < max; i++) {
        unsigned k;
        int dup = 0;
        for (k = 0; k < n; k++) if (out[k] == tv[i]) { dup = 1; break; }
        if (!dup) out[n++] = tv[i];
    }
    return n;
}

float entity_dist2(void* e, void* ref) {
    float dx = entity_pos(e, 0) - entity_pos(ref, 0);
    float dy = entity_pos(e, 1) - entity_pos(ref, 1);
    float dz = entity_pos(e, 2) - entity_pos(ref, 2);
    return dx * dx + dy * dy + dz * dz;
}

unsigned resolve_selector(void* executor, void* level, void* game, const char* token, void** out, unsigned max) {
    char base[24];
    unsigned blen = 0;
    const char* a = token;
    void* ents[SELECTOR_TARGET_CAPACITY];
    float dists[SELECTOR_TARGET_CAPACITY];
    unsigned npool = 0, i, j, k;
    char pname[40];
    int has_type = 0, type_len = 0, has_name = 0, name_len = 0;
    char typev[40], namev[32];
    float rmax = -1.0f, rmin = -1.0f;
    int cnt = 0, has_c = 0;
    (void)game;
    while (a[blen] && a[blen] != '[' && blen < sizeof(base) - 1) {
        base[blen] = a[blen];
        blen++;
    }
    base[blen] = 0;
    if (a[blen] == '[') {
        const char* q = a + blen + 1;
        while (*q && *q != ']') {
            char key[12];
            int kn = 0, val = 0;
            while (*q && *q != '=' && *q != ',' && *q != ']') {
                if (kn < (int)sizeof(key) - 1) key[kn++] = *q;
                q++;
            }
            key[kn] = 0;
            if (*q != '=') {
                while (*q && *q != ']') q++;
                break;
            }
            q++;
            if (streq(key, "type")) {
                has_type = 1;
                type_len = 0;
                while (*q && *q != ',' && *q != ']') {
                    if (type_len < (int)sizeof(typev) - 1) typev[type_len++] = *q;
                    q++;
                }
                typev[type_len] = 0;
            } else if (streq(key, "name")) {
                has_name = 1;
                name_len = 0;
                while (*q && *q != ',' && *q != ']') {
                    if (name_len < (int)sizeof(namev) - 1) namev[name_len++] = *q;
                    q++;
                }
                namev[name_len] = 0;
            } else if (streq(key, "r") || streq(key, "rm")) {
                if (integer(&q, &val)) {
                    if (streq(key, "r")) rmax = (float)val;
                    else rmin = (float)val;
                }
            } else if (streq(key, "c")) {
                if (integer(&q, &cnt)) has_c = 1;
            } else {
                while (*q && *q != ',' && *q != ']') q++;
            }
            if (*q == ',') q++;
        }
    }
    if (!base[0]) return 0;
    if (streq(base, "@e")) {
        unsigned nt2, i2, nworld;
        int dup;
        void* tv2[SELECTOR_TARGET_CAPACITY];
        void* world_entities[SELECTOR_TARGET_CAPACITY];
        nt2 = collect_targets(executor, level, "@a", tv2, SELECTOR_TARGET_CAPACITY);
        if (nt2 > SELECTOR_TARGET_CAPACITY) nt2 = SELECTOR_TARGET_CAPACITY;
        nworld = collect_world_entities(level, world_entities, SELECTOR_TARGET_CAPACITY - nt2);
        for (i = 0; i < nworld; i++) ents[npool++] = world_entities[i];
        for (i2 = 0; i2 < nt2 && npool < SELECTOR_TARGET_CAPACITY; i2++) {
            dup = 0;
            for (j = 0; j < npool; j++) if (ents[j] == tv2[i2]) { dup = 1; break; }
            if (!dup) ents[npool++] = tv2[i2];
        }
        for (i = 0; i < npool; i++) dists[i] = executor ? entity_dist2(ents[i], executor) : 0.0f;
    } else {
        npool = collect_targets(executor, level, base, ents, SELECTOR_TARGET_CAPACITY);
        for (i = 0; i < npool; i++) dists[i] = executor ? entity_dist2(ents[i], executor) : 0.0f;
    }
    k = 0;
    for (i = 0; i < npool; i++) {
        if (has_type) {
            int ok = 0;
            entity_type_name(ents[i], pname, sizeof(pname));
            if (name_ieq(strip_ns(pname), strip_ns(typev)) || name_ieq(pname, typev)) ok = 1;
            if (!ok) continue;
        }
        if (has_name) {
            entity_name(ents[i], pname, sizeof(pname));
            if (!name_ieq(pname, namev)) continue;
        }
        if (rmin >= 0.0f && dists[i] < rmin * rmin) continue;
        if (rmax >= 0.0f && dists[i] > rmax * rmax) continue;
        ents[k] = ents[i];
        dists[k] = dists[i];
        k++;
    }
    npool = k;
    if (has_c && cnt != 0) {
        for (i = 0; i < npool; i++) {
            for (j = i + 1; j < npool; j++) {
                if ((cnt > 0 && dists[j] < dists[i]) || (cnt < 0 && dists[j] > dists[i])) {
                    void* te = ents[i];
                    float td = dists[i];
                    ents[i] = ents[j];
                    dists[i] = dists[j];
                    ents[j] = te;
                    dists[j] = td;
                }
            }
        }
        npool = (unsigned)(cnt > 0 ? (cnt < (int)npool ? cnt : npool) : (-cnt < (int)npool ? -cnt : (int)npool));
    }
    if (npool > (unsigned)max) npool = (unsigned)max;
    for (i = 0; i < npool; i++) out[i] = ents[i];
    return npool;
}

static unsigned native_callback_targets(void** out, unsigned max) {
    unsigned count;
    if (!s || !s->exec_active || !s->exec_target_explicit || !s->exec_target_count || !out || !max) return 0;
    count = s->exec_target_count;
    if (count > max) count = max;
    cp(out, s->exec_targets, count * sizeof(void*));
    return count;
}

unsigned command_collect_targets(void* executor, void* level, const char* token, void** out, unsigned max) {
    unsigned count = native_callback_targets(out, max);
    return count ? count : collect_targets(executor, level, token, out, max);
}

unsigned command_resolve_targets(void* executor, void* level, void* game, const char* token, void** out, unsigned max) {
    unsigned count = native_callback_targets(out, max);
    return count ? count : resolve_selector(executor, level, game, token, out, max);
}

const NameId effect_table[] = {
    {"speed", 1}, {"slowness", 2}, {"haste", 3}, {"mining_fatigue", 4}, {"strength", 5},
    {"instant_health", 6}, {"instant_damage", 7}, {"jump_boost", 8}, {"nausea", 9},
    {"regeneration", 10}, {"resistance", 11}, {"fire_resistance", 12}, {"water_breathing", 13},
    {"invisibility", 14}, {"blindness", 15}, {"night_vision", 16}, {"hunger", 17},
    {"weakness", 18}, {"poison", 19}, {"wither", 20}, {"health_boost", 21},
    {"absorption", 22}, {"saturation", 23}
};

const NameId enchant_table[] = {
    {"protection", 0}, {"fire_protection", 1}, {"feather_falling", 2}, {"blast_protection", 3},
    {"projectile_protection", 4}, {"thorns", 5}, {"respiration", 6}, {"depth_strider", 7},
    {"aqua_affinity", 8}, {"sharpness", 9}, {"smite", 10}, {"bane_of_arthropods", 11},
    {"knockback", 12}, {"fire_aspect", 13}, {"looting", 14}, {"efficiency", 15},
    {"silk_touch", 16}, {"unbreaking", 17}, {"fortune", 18}, {"power", 19}, {"punch", 20},
    {"flame", 21}, {"infinity", 22}, {"luck_of_the_sea", 23}, {"lure", 24}
};

const NameId entity_table[] = {
    {"chicken", 0x130A}, {"cow", 0x130B}, {"pig", 0x130C}, {"sheep", 0x130D},
    {"wolf", 0x530E}, {"villager", 0x030F}, {"mooshroom", 0x1310}, {"squid", 0x2311},
    {"rabbit", 0x1312}, {"bat", 0x8113}, {"iron_golem", 0x0314}, {"snow_golem", 0x0315},
    {"ocelot", 0x5316}, {"horse", 0x205317}, {"donkey", 0x205318}, {"mule", 0x205319},
    {"skeleton_horse", 0x215B1A}, {"zombie_horse", 0x215B1B}, {"polar_bear", 0x131C},
    {"llama", 0x131D}, {"zombie", 0x030B20}, {"creeper", 0x0B21},
    {"skeleton", 0x110B22}, {"spider", 0x040B23}, {"zombie_pigman", 0x010B24},
    {"slime", 0x0B25}, {"enderman", 0x0B26}, {"silverfish", 0x040B27},
    {"cave_spider", 0x040B28}, {"ghast", 0x0B29}, {"magma_cube", 0x0B2A},
    {"blaze", 0x0B2B}, {"zombie_villager", 0x030B2C}, {"witch", 0x0B2D},
    {"stray", 0x110B2E}, {"husk", 0x030B2F}, {"wither_skeleton", 0x110B30},
    {"guardian", 0x0B31}, {"shulker", 0x0B36}, {"elder_guardian", 0x0B32},
    {"irongolem", 0x0314}, {"snowgolem", 0x0315}, {"skeletonhorse", 0x215B1A},
    {"zombiehorse", 0x215B1B}, {"polarbear", 0x131C}, {"zombiepigman", 0x010B24},
    {"cavespider", 0x040B28}, {"magmacube", 0x0B2A}, {"zombievillager", 0x030B2C},
    {"witherskeleton", 0x110B30}, {"elderguardian", 0x0B32}
};

int table_lookup(const NameId* table, unsigned count, const char* name) {
    unsigned i;
    for (i = 0; i < count; i++) {
        if (streq(table[i].name, name)) return table[i].id;
    }
    return -1;
}
