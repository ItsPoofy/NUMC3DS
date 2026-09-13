#ifndef NUMC3DS_ACHIEVEMENT_RESTORE_H
#define NUMC3DS_ACHIEVEMENT_RESTORE_H

typedef unsigned int numc3ds_achievement_u32;

void numc3ds_register_iron_golem_and_leader(
    void *achievement_vector,
    numc3ds_achievement_u32 *achievement_id,
    const char *localization_key);
void numc3ds_wolf_on_tame(void *wolf);

#endif
