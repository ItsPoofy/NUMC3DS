#include "achievement_restore.h"

enum {
    ACHIEVEMENT_LEADER_OF_THE_PACK = 0x06,
    ACHIEVEMENT_MANAGER_GET_INSTANCE = 0x00228EB8,
    ACHIEVEMENT_MANAGER_UNLOCK = 0x0042BFD0,
    ACHIEVEMENT_VECTOR_PUSH = 0x008F080C,
};

typedef void (*AchievementVectorPushFn)(
    void *achievement_vector,
    numc3ds_achievement_u32 *achievement_id,
    const char *localization_key);
typedef void *(*AchievementManagerGetInstanceFn)(void);
typedef void (*AchievementManagerUnlockFn)(
    void *achievement_manager,
    numc3ds_achievement_u32 achievement_id,
    float progress);

static const char leader_of_the_pack_key[] = "tameWolves";

__attribute__((section(".patch_targets"), used, noinline))
void numc3ds_register_iron_golem_and_leader(
    void *achievement_vector,
    numc3ds_achievement_u32 *achievement_id,
    const char *localization_key)
{
    AchievementVectorPushFn push =
        (AchievementVectorPushFn)ACHIEVEMENT_VECTOR_PUSH;

    push(achievement_vector, achievement_id, localization_key);
    *achievement_id = ACHIEVEMENT_LEADER_OF_THE_PACK;
    push(achievement_vector, achievement_id, leader_of_the_pack_key);
}

__attribute__((section(".patch_targets"), used, noinline))
void numc3ds_wolf_on_tame(void *wolf)
{
    AchievementManagerGetInstanceFn get_instance =
        (AchievementManagerGetInstanceFn)ACHIEVEMENT_MANAGER_GET_INSTANCE;
    AchievementManagerUnlockFn unlock =
        (AchievementManagerUnlockFn)ACHIEVEMENT_MANAGER_UNLOCK;
    void *achievement_manager;

    (void)wolf;
    achievement_manager = get_instance();
    if (achievement_manager) {
        unlock(achievement_manager, ACHIEVEMENT_LEADER_OF_THE_PACK, 0.2f);
    }
}
