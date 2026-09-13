#ifndef NUMC3DS_INTERNAL_H
#define NUMC3DS_INTERNAL_H
#include "rt.h"
#include "util/string_util.h"
#include "util/math_util.h"
#include "commands/command_error_service.h"
#include "world/world_block_service.h"
#include "world/gamerule_service.h"
#include "world/weather_service.h"
#include "item/item_registry_service.h"
#include "entity/selector_service.h"
#include "ui/title_service.h"
#include "commands/block_mutation_service.h"

#define SELECTOR_TARGET_CAPACITY 128u

typedef int (*CmdFn)(void *player,void *level,void *game,const char *args);
/* Shared types used across modules */

/* Command handler declarations (all defined under commands). */
extern int cmd_help(void*,void*,void*,const char*);
extern int cmd_time(void*,void*,void*,const char*);
extern int cmd_gamemode(void*,void*,void*,const char*);
extern int cmd_tp(void*,void*,void*,const char*);
extern int cmd_kill(void*,void*,void*,const char*);
extern int cmd_difficulty(void*,void*,void*,const char*);
extern int cmd_setworldspawn(void*,void*,void*,const char*);
extern int cmd_say(void*,void*,void*,const char*);
extern int cmd_me(void*,void*,void*,const char*);
extern int cmd_list(void*,void*,void*,const char*);
extern int cmd_give(void*,void*,void*,const char*);
extern int cmd_clear(void*,void*,void*,const char*);
extern int cmd_effect(void*,void*,void*,const char*);
extern int cmd_enchant(void*,void*,void*,const char*);
extern int cmd_xp(void*,void*,void*,const char*);
extern int cmd_gamerule(void*,void*,void*,const char*);
extern int cmd_daylock(void*,void*,void*,const char*);
extern int cmd_tell(void*,void*,void*,const char*);
extern int cmd_title(void*,void*,void*,const char*);
extern int cmd_playsound(void*,void*,void*,const char*);
extern int cmd_testfor(void*,void*,void*,const char*);
extern int cmd_summon(void*,void*,void*,const char*);
extern int cmd_spawnpoint(void*,void*,void*,const char*);
extern int cmd_op(void*,void*,void*,const char*);
extern int cmd_deop(void*,void*,void*,const char*);
extern int cmd_weather(void*,void*,void*,const char*);
extern int cmd_toggledownfall(void*,void*,void*,const char*);
extern int cmd_setmaxplayers(void*,void*,void*,const char*);
extern int cmd_setblock(void*,void*,void*,const char*);
extern int cmd_fill(void*,void*,void*,const char*);
extern int cmd_clone(void*,void*,void*,const char*);
extern int cmd_testforblock(void*,void*,void*,const char*);
extern int cmd_testforblocks(void*,void*,void*,const char*);
extern int cmd_spreadplayers(void*,void*,void*,const char*);
extern int cmd_execute(void*,void*,void*,const char*);
extern int cmd_locate(void*,void*,void*,const char*);
extern int cmd_stopsound(void*,void*,void*,const char*);
extern int cmd_replaceitem(void*,void*,void*,const char*);
extern int cmd_testall(void*,void*,void*,const char*);

/* Helper functions (defined in runtime.c / sel.c) */
extern float entity_dist2(void*e,void*ref);
extern float entity_pos(void*e,int axis);
extern float pos_base(void*player,int axis);
enum {
    ENCHANT_APPLY_INTERNAL=-1,
    ENCHANT_APPLY_NO_ITEM=0,
    ENCHANT_APPLY_SUCCESS=1,
    ENCHANT_APPLY_CANT_ENCHANT=2,
    ENCHANT_APPLY_CANT_COMBINE=3
};
extern int enchant_max_level(int eid);
extern int apply_enchant(void*level,void*target,int eid,int lvl);
extern int boxes_overlap(const int lo1[3],const int hi1[3],const int lo2[3],const int hi2[3]);
extern int fail_syntax(void);
extern int gamerule_query_one(void*rules,const char*name);
extern int gamerule_set(void*rules,const char*name,const char*value);
extern int boxes_overlap(const int lo1[3],const int hi1[3],const int lo2[3],const int hi2[3]);
extern int fail_syntax(void);
extern int gamerule_query_one(void*rules,const char*name);
extern int gamerule_set(void*rules,const char*name,const char*value);
extern int integer(const char**at,int*out);
extern int name_ieq(const char*a,const char*b);
extern int resolve_block_arg(char*name,unsigned char*idOut);
extern int rule_value_bool(void*rule);
extern int self_target(const char*p);
extern int send_command_chat(void*player,const char*source_text,const char*message);
extern int streq(const char*a,const char*b);
extern int topsolid(void*src,const int*pos);
extern unsigned collect_targets(void*executor,void*level,const char*token,void**out,unsigned max);
extern unsigned collect_world_entities(void*game,void**out,unsigned max);
extern unsigned resolve_selector(void*executor,void*level,void*game,const char*token,void**out,unsigned max);
/* Native command callbacks already resolved target selectors through the
 * stock CommandTarget converter.  Handlers must consume that exact list
 * instead of reparsing a synthetic target and losing selector fan-out.
 * Outside the native callback these helpers are transparent wrappers around
 * the retained selector implementation. */
extern unsigned command_collect_targets(void*executor,void*level,const char*token,void**out,unsigned max);
extern unsigned command_resolve_targets(void*executor,void*level,void*game,const char*token,void**out,unsigned max);
extern unsigned text_len(const char*t);
extern u32 make_str(u32*out,const char*t);
extern u32 rng_next(void);
extern void *block_by_name(const char*name);
extern void *inventory_of(void*player);
extern void *item_by_id(int id);
extern void *item_by_name(const char*name);
extern void *item_from_arg(const char*name);
extern void *map_find_int(void*map,u32 key);
extern void *rules_root(void*rules);
extern void *supplies(void*player);
extern void *world_source(void*level);
extern void action_message(void*player,const char*message);
extern void append_int(char*out,unsigned*len,int value);
extern void append(char*out,unsigned*len,const char*text);
extern const char*strip_ns(const char*n);
extern void drop_str(u32*h);
extern void entity_name(void*e,char*out,unsigned cap);
extern void item_not_found(const char*name);
extern void release_obj(void*o);
extern void result_number(const char*prefix,int value);
extern void result_text(const char*t);
extern int divide(int value,int divisor);
extern void title_apply_local(const char*op,const char*text_arg);
extern int decimal(const char**at,float base,float*out);
extern const char *word(const char*p,char*out,unsigned cap);

extern void *rule_dfs(void*node,const char*name);
extern void rule_print(char*out,unsigned*len,void*rule);
extern void rule_walk(void*node,char*out,unsigned*len);
extern int gamerule_query_one(void*rules,const char*name);
extern const char *spaces(const char*p);
extern unsigned collect_world_entities(void*game,void**out,unsigned max);
extern u32 rng_state;
extern int cmd_debug(void*,void*,void*,const char*);
extern int cmd_debug2(void*,void*,void*,const char*);
extern int cmd_testspawn(void*,void*,void*,const char*);
extern int cmd_testall(void*,void*,void*,const char*);
#endif

