#include "entity_type_enum.h"
#include "internal.h"
#include "commands/native_types.h"

typedef struct {
    u32 type;
    u32 enabled;
    u32 factory;
} EntityFactoryDescriptor;

typedef void *(*VectorAllocateFn)(u32,u32);
typedef int (*EntityTypeFromStringFn)(NativeGstdString *);

typedef struct {
    int id;
    const char *name;
} EntityIdNamePair;

static const char * const canonical_entity_names[] = {
    "armor_stand",
    "arrow",
    "bat",
    "blaze",
    "boat",
    "camera",
    "cave_spider",
    "chalkboard",
    "chicken",
    "cow",
    "creeper",
    "donkey",
    "dragon_fireball",
    "egg",
    "elder_guardian",
    "ender_dragon",
    "ender_pearl",
    "enderman",
    "endermite",
    "evoker",
    "eye_of_ender_signal",
    "fireball",
    "fishing_hook",
    "ghast",
    "guardian",
    "horse",
    "husk",
    "iron_golem",
    "item",
    "lightning_bolt",
    "llama",
    "magma_cube",
    "minecart",
    "mooshroom",
    "mule",
    "npc",
    "ocelot",
    "pig",
    "polar_bear",
    "rabbit",
    "sheep",
    "shulker",
    "silverfish",
    "skeleton",
    "skeleton_horse",
    "slime",
    "small_fireball",
    "snow_golem",
    "snowball",
    "spider",
    "splash_potion",
    "squid",
    "stray",
    "tnt",
    "vex",
    "villager",
    "vindicator",
    "witch",
    "wither",
    "wither_skeleton",
    "wither_skull",
    "wither_skull_dangerous",
    "wolf",
    "xp_bottle",
    "xp_orb",
    "zombie",
    "zombie_horse",
    "zombie_pigman"
};

enum { CANONICAL_ENTITY_COUNT = sizeof(canonical_entity_names)/sizeof(canonical_entity_names[0]) };

static const EntityIdNamePair id_name_pairs[] = {
    { 0x00130a, "chicken" },
    { 0x00130b, "cow" },
    { 0x00130c, "pig" },
    { 0x00130d, "sheep" },
    { 0x00530e, "wolf" },
    { 0x00030f, "villager" },
    { 0x001310, "mooshroom" },
    { 0x002311, "squid" },
    { 0x001312, "rabbit" },
    { 0x008113, "bat" },
    { 0x000314, "iron_golem" },
    { 0x000315, "snow_golem" },
    { 0x005316, "ocelot" },
    { 0x205317, "horse" },
    { 0x00131d, "polar_bear" },
    { 0x00131c, "llama" },
    { 0x205318, "donkey" },
    { 0x205319, "mule" },
    { 0x215b1a, "skeleton_horse" },
    { 0x215b1b, "zombie_horse" },
    { 0x030b20, "zombie" },
    { 0x000b21, "creeper" },
    { 0x110b22, "skeleton" },
    { 0x040b23, "spider" },
    { 0x010b24, "zombie_pigman" },
    { 0x000b25, "slime" },
    { 0x000b26, "enderman" },
    { 0x040b27, "silverfish" },
    { 0x040b28, "cave_spider" },
    { 0x000b29, "ghast" },
    { 0x000b2a, "magma_cube" },
    { 0x000b2b, "blaze" },
    { 0x030b2c, "zombie_villager" },
    { 0x000b2d, "witch" },
    { 0x110b2e, "stray" },
    { 0x030b2f, "husk" },
    { 0x110b30, "wither_skeleton" },
    { 0x000b31, "guardian" },
    { 0x000b32, "elder_guardian" },
    { 0x000b39, "wither" },
    { 0x010b34, "ender_dragon" },
    { 0x000b35, "shulker" },
    { 0x000b36, "endermite" },
    { 0x040b37, "vindicator" },
    { 0x000b68, "evoker" },
    { 0x000b69, "vex" },
    { 0x080054, "minecart" },
    { 0x080060, "hopper_minecart" },
    { 0x080061, "tnt_minecart" },
    { 0x080062, "chest_minecart" },
    { 0x080064, "command_block_minecart" },
    { 0x000040, "item" },
    { 0x000041, "tnt" },
    { 0x000042, "falling_block" },
    { 0x400044, "xp_bottle" },
    { 0x000045, "xp_orb" },
    { 0x000046, "eye_of_ender_signal" },
    { 0x000047, "ender_pearl" },
    { 0x00005a, "boat" },
    { 0x00005d, "lightning_bolt" },
    { 0x400050, "arrow" },
    { 0x400051, "snowball" },
    { 0x400052, "egg" },
    { 0x400056, "fireball" },
    { 0x400058, "splash_potion" },
    { 0x400067, "dragon_fireball" },
};

enum { ID_NAME_PAIR_COUNT = sizeof(id_name_pairs)/sizeof(id_name_pairs[0]) };

static char entity_name_fold(char value){return value>='A'&&value<='Z'?(char)(value-'A'+'a'):value;}

static int entity_name_equal(const char *left,const char *right){
    while(*left&&*right){if(entity_name_fold(*left++)!=entity_name_fold(*right++))return 0;}
    return *left==*right;
}

static unsigned collect_names(const char **names){
    unsigned i;
    for(i=0;i<CANONICAL_ENTITY_COUNT;i++){
        names[i]=canonical_entity_names[i];
    }
    return CANONICAL_ENTITY_COUNT;
}

u32 entity_type_build_enum(void *output){
    NativeGstdString *values;u32 *memory;unsigned count,index;
    if(!output)return 0;zero(output,sizeof(NativeVector));
    count=CANONICAL_ENTITY_COUNT;
    memory=(u32*)((VectorAllocateFn)SEAM_gstd_allocator_allocate)(count*sizeof(NativeGstdString),0);if(!memory)return 0;
    zero(memory,count*sizeof(NativeGstdString));values=(NativeGstdString*)memory;
    for(index=0;index<count;index++){u32 scratch=0;((StrCtor)SEAM_StrCtor)(&values[index],canonical_entity_names[index],&scratch);}
    ((NativeVector*)output)->begin=(u32)values;((NativeVector*)output)->end=(u32)(values+count);((NativeVector*)output)->capacity=(u32)(values+count);
    return 0;
}

int entity_type_enum_contains(const char *name){
    unsigned i;if(!name)return 0;
    for(i=0;i<CANONICAL_ENTITY_COUNT;i++)if(entity_name_equal(canonical_entity_names[i],name))return 1;
    return 0;
}

int entity_type_resolve_name(const char *name){
    NativeGstdString value;u32 scratch=0;int result;unsigned i;
    if(!name||!name[0])return -1;
    /* Try native string lookup (Jenkins hash against code.bin map) */
    ((StrCtor)SEAM_StrCtor)(&value,name,&scratch);
    result=((EntityTypeFromStringFn)SEAM_EntityTypeFromString)(&value);
    ((StrDtor)SEAM_StrDtor)(&value);
    if(result>1)return result;

    /* Fallback alias resolution */
    for(i=0;i<ID_NAME_PAIR_COUNT;i++){
        if(entity_name_equal(id_name_pairs[i].name,name))return id_name_pairs[i].id;
    }
    if(entity_name_equal(name,"pig_zombie")||entity_name_equal(name,"zombiepigman"))return 0x010b24;
    if(entity_name_equal(name,"magmacube"))return 0x000b2a;
    if(entity_name_equal(name,"irongolem"))return 0x000314;
    if(entity_name_equal(name,"snowgolem"))return 0x000315;
    if(entity_name_equal(name,"cavespider"))return 0x040b28;
    if(entity_name_equal(name,"skeletonhorse"))return 0x215b1a;
    if(entity_name_equal(name,"zombiehorse"))return 0x215b1b;
    if(entity_name_equal(name,"witherskeleton"))return 0x110b30;
    if(entity_name_equal(name,"elderguardian"))return 0x000b32;
    if(entity_name_equal(name,"polarbear"))return 0x00131d;
    return -1;
}

const char *entity_type_name_from_id(int entity_type){
    unsigned i;
    for(i=0;i<ID_NAME_PAIR_COUNT;i++){
        if(id_name_pairs[i].id==entity_type)return id_name_pairs[i].name;
    }
    return 0;
}

void entity_type_visit_names(EntityTypeNameVisitor visitor,void *context){
    const char *names[CANONICAL_ENTITY_COUNT];unsigned count,i;if(!visitor)return;count=collect_names(names);
    for(i=0;i<count;i++)visitor(names[i],context);
}
