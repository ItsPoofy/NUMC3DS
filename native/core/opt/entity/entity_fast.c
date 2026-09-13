#include "entity_fast.h"
#include "../../state.h"

static NuMC3DS_Hook entity_hooks[5];

static inline float fast_sqrt(float val)
{
    float res;
    __asm__("vsqrt.f32 %0, %1" : "=t"(res) : "t"(val));
    return res;
}

int entity_intersects_fast(const void *self, const float *min, const float *max)
{
    const float *bb = (const float *)((const char *)self + 0x288);
    if (max[0] <= bb[0] || min[0] >= bb[3]) return 0;
    if (max[1] <= bb[1] || min[1] >= bb[4]) return 0;
    if (max[2] <= bb[2] || min[2] >= bb[5]) return 0;
    return 1;
}

float entity_distance_to_vec3_sq_fast(const void *self, const float *pos)
{
    const float *spos = (const float *)((const char *)self + 0x1C0);
    float dx = spos[0] - pos[0];
    float dy = spos[1] - pos[1];
    float dz = spos[2] - pos[2];
    return dx * dx + dy * dy + dz * dz;
}

float entity_distance_to_entity_sq_fast(const void *self, const void *other)
{
    const float *spos = (const float *)((const char *)self + 0x1C0);
    const float *opos = (const float *)((const char *)other + 0x1C0);
    float dx = spos[0] - opos[0];
    float dy = spos[1] - opos[1];
    float dz = spos[2] - opos[2];
    return dx * dx + dy * dy + dz * dz;
}

float entity_distance_to_vec3_fast(const void *self, const float *pos)
{
    const float *spos = (const float *)((const char *)self + 0x1C0);
    float dx = spos[0] - pos[0];
    float dy = spos[1] - pos[1];
    float dz = spos[2] - pos[2];
    return fast_sqrt(dx * dx + dy * dy + dz * dz);
}

float entity_distance_to_entity_fast(const void *self, const void *other)
{
    const float *spos = (const float *)((const char *)self + 0x1C0);
    const float *opos = (const float *)((const char *)other + 0x1C0);
    float dx = spos[0] - opos[0];
    float dy = spos[1] - opos[1];
    float dz = spos[2] - opos[2];
    return fast_sqrt(dx * dx + dy * dy + dz * dz);
}

void entity_turn_fast(void *self, const float *rot, int slow)
{
    float *fself = (float *)((char *)self + 0x1F0);
    float scale = slow ? 0.15f : 1.0f;
    float prev_pitch = fself[0]; /* 0x1F0 */
    float prev_yaw   = fself[1]; /* 0x1F4 */
    float new_yaw    = prev_yaw + rot[1] * scale;
    float new_pitch  = prev_pitch - rot[0] * scale;

    if (new_pitch < -90.0f) new_pitch = -90.0f;
    if (new_pitch > 90.0f) new_pitch = 90.0f;

    fself[0] = new_pitch;
    fself[1] = new_yaw;
    fself[2] += (new_pitch - prev_pitch); /* 0x1F8: mRotPrev.x */
    fself[3] += (new_yaw - prev_yaw);     /* 0x1FC: mRotPrev.y */
}

void entity_set_rot_fast(void *self, const float *rot)
{
    float *fself = (float *)((char *)self + 0x1F0);
    float pitch = rot[0];
    float yaw = rot[1];
    float prev_pitch = fself[2]; /* 0x1F8 */
    float prev_yaw = fself[3];   /* 0x1FC */

    while (yaw > 360.0f) {
        yaw -= 360.0f;
        prev_yaw -= 360.0f;
    }
    while (yaw < -360.0f) {
        yaw += 360.0f;
        prev_yaw += 360.0f;
    }
    while (pitch > 360.0f) {
        pitch -= 360.0f;
        prev_pitch -= 360.0f;
    }
    while (pitch < -360.0f) {
        pitch += 360.0f;
        prev_pitch += 360.0f;
    }

    fself[0] = pitch;
    fself[1] = yaw;
    fself[2] = prev_pitch;
    fself[3] = prev_yaw;
}

void entity_sub_hit_fast(void *self, const float *point, const float *inflation)
{
    const float *begin = *(const float **)((const char *)self + 0x2A4);
    const float *end   = *(const float **)((const char *)self + 0x2A8);
    float px = point[0], py = point[1], pz = point[2];
    float ix = inflation[0], iy = inflation[1], iz = inflation[2];

    while (begin < end) {
        float min_x = begin[0] - ix;
        float max_x = begin[3] + ix;
        if (px > min_x && px < max_x) {
            float min_y = begin[1] - iy;
            float max_y = begin[4] + iy;
            if (py > min_y && py < max_y) {
                float min_z = begin[2] - iz;
                float max_z = begin[5] + iz;
                if (pz > min_z && pz < max_z) {
                    *(const void **)((char *)self + 0x284) = (const void *)begin;
                }
            }
        }
        begin = (const float *)((const char *)begin + 0x1C);
    }
}

typedef void *(*NavPathFn)(void *result_ptr, void *nav, void *target);

static void *path_navigation_can_update_path_fast(void *result_ptr, void *nav, void *target_entity)
{
    if (!result_ptr) return 0;
    if (!nav || !target_entity) {
        *(void **)result_ptr = 0;
        return result_ptr;
    }
    void *mob = *(void **)((char *)nav + 4);
    if (!mob) {
        *(void **)result_ptr = 0;
        return result_ptr;
    }

    void **nav_vtable = *(void ***)nav;
    float follow_range = ((float (*)(void *))nav_vtable[8])(nav);
    float max_dist_sq = follow_range * follow_range;
    float dist_sq = entity_distance_to_entity_sq_fast(mob, target_entity);

    if (dist_sq > max_dist_sq) {
        *(void **)result_ptr = 0;
        return result_ptr;
    }

    return ((NavPathFn)entity_hooks[3].trampoline)(result_ptr, nav, target_entity);
}

static void *path_navigation_create_path_fast(void *result_ptr, void *nav, const float *target_pos)
{
    if (!result_ptr) return 0;
    if (!nav || !target_pos) {
        *(void **)result_ptr = 0;
        return result_ptr;
    }
    void *mob = *(void **)((char *)nav + 4);
    if (!mob) {
        *(void **)result_ptr = 0;
        return result_ptr;
    }

    void **nav_vtable = *(void ***)nav;
    float follow_range = ((float (*)(void *))nav_vtable[8])(nav);
    float max_dist_sq = follow_range * follow_range;
    float dist_sq = entity_distance_to_vec3_sq_fast(mob, target_pos);

    if (dist_sq > max_dist_sq) {
        *(void **)result_ptr = 0;
        return result_ptr;
    }

    return ((NavPathFn)entity_hooks[4].trampoline)(result_ptr, nav, (void *)target_pos);
}

int entity_fast_install_hooks(void)
{
    static const u32 targets[5] = {
        0x005E87F8u, /* Entity::intersects */
        0x005E9E94u, /* Entity::subHit */
        0x005F62CCu, /* Entity::turn */
        0x00281DA0u, /* PathNavigation::canUpdatePath */
        0x00281E84u  /* PathNavigation::createPath */
    };

    static const u32 expected[5][2] = {
        { 0xE92D4010u, 0xE24DD020u }, /* Entity::intersects */
        { 0xE92D41F0u, 0xE1A05000u }, /* Entity::subHit */
        { 0xED900A7Du, 0xE3520000u }, /* Entity::turn */
        { 0xE92D40F0u, 0xE1A05000u }, /* PathNavigation::canUpdatePath */
        { 0xE92D41F0u, 0xE1A04001u }  /* PathNavigation::createPath */
    };

    u32 replacements[5];
    u32 i;

    replacements[0] = (u32)entity_intersects_fast;
    replacements[1] = (u32)entity_sub_hit_fast;
    replacements[2] = (u32)entity_turn_fast;
    replacements[3] = (u32)path_navigation_can_update_path_fast;
    replacements[4] = (u32)path_navigation_create_path_fast;

    for (i = 0; i < 5; i++) {
        entity_hooks[i].target = targets[i];
        entity_hooks[i].replacement = replacements[i];
        entity_hooks[i].expected[0] = expected[i][0];
        entity_hooks[i].expected[1] = expected[i][1];
        if (s->host.install_hook(&entity_hooks[i])) return -80 - (int)i;
    }
    return 0;
}
