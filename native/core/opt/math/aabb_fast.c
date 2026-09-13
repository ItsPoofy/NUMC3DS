#include "aabb_fast.h"
#include "../../state.h"

static NuMC3DS_Hook aabb_hooks[18];

void *aabb_offset(float *result, const float *src, const float *offset)
{
    float min_x = src[0] + offset[0];
    float min_y = src[1] + offset[1];
    float min_z = src[2] + offset[2];
    float max_x = src[3] + offset[0];
    float max_y = src[4] + offset[1];
    float max_z = src[5] + offset[2];
    result[0] = min_x;
    result[1] = min_y;
    result[2] = min_z;
    result[3] = max_x;
    result[4] = max_y;
    result[5] = max_z;
    ((u8 *)result)[24] = (min_x == 0.0f && min_y == 0.0f && min_z == 0.0f &&
                          max_x == 0.0f && max_y == 0.0f && max_z == 0.0f) ? 1 : 0;
    return result;
}

float aabb_distance_to_sqr_aabb(const float *self, const float *other)
{
    float dist_sq = 0.0f;
    int i;
    for (i = 0; i < 3; i++) {
        float min_a = self[i];
        float max_b = other[i + 3];
        if (min_a > max_b) {
            float diff = min_a - max_b;
            dist_sq += diff * diff;
        } else {
            float min_b = other[i];
            float max_a = self[i + 3];
            if (min_b > max_a) {
                float diff = min_b - max_a;
                dist_sq += diff * diff;
            }
        }
    }
    return dist_sq;
}

float aabb_distance_to_sqr_vec3(const float *self, const float *point)
{
    float dist_sq = 0.0f;
    int i;
    for (i = 0; i < 3; i++) {
        float p = point[i];
        float min_val = self[i];
        float max_val = self[i + 3];
        if (p < min_val) {
            float diff = min_val - p;
            dist_sq += diff * diff;
        } else if (p > max_val) {
            float diff = p - max_val;
            dist_sq += diff * diff;
        }
    }
    return dist_sq;
}

int aabb_intersects(const float *self, const float *other)
{
    if (other[3] <= self[0] || other[0] >= self[3]) return 0;
    if (other[4] <= self[1] || other[1] >= self[4]) return 0;
    if (other[5] <= self[2] || other[2] >= self[5]) return 0;
    return 1;
}

void *aabb_set(float *self, const float *min, const float *max)
{
    float min_x = min[0], min_y = min[1], min_z = min[2];
    float max_x = max[0], max_y = max[1], max_z = max[2];
    self[0] = min_x;
    self[1] = min_y;
    self[2] = min_z;
    self[3] = max_x;
    self[4] = max_y;
    self[5] = max_z;
    ((u8 *)self)[24] = (min_x == 0.0f && min_y == 0.0f && min_z == 0.0f &&
                        max_x == 0.0f && max_y == 0.0f && max_z == 0.0f) ? 1 : 0;
    return self;
}

void *aabb_ctor_vec3(float *self, const float *min, const float *max)
{
    return aabb_set(self, min, max);
}

void *aabb_ctor_floats(float *self, float minX, float minY, float minZ,
                       float maxX, float maxY, float maxZ)
{
    self[0] = minX;
    self[1] = minY;
    self[2] = minZ;
    self[3] = maxX;
    self[4] = maxY;
    self[5] = maxZ;
    ((u8 *)self)[24] = (minX == 0.0f && minY == 0.0f && minZ == 0.0f &&
                        maxX == 0.0f && maxY == 0.0f && maxZ == 0.0f) ? 1 : 0;
    return self;
}

Vec3Float __attribute__((pcs("aapcs-vfp"))) aabb_clip_collide(const float *other, const float *self, const float *motion, int no_clamp, float *hit_distance)
{
    Vec3Float out;
    out.x = motion[0];
    out.y = motion[1];
    out.z = motion[2];

    if (hit_distance) {
        *hit_distance = 0.0f;
    }

    int count = 0;
    int sep_axis = 0;
    float min_dist = 3.4028235e+38f;

    float overlaps[3];
    float diffs[3];
    float signs[3];

    for (int i = 0; i < 3; i++) {
        float s19 = self[i + 3] - other[i];
        float s2 = other[i + 3] - self[i];

        float s0 = s19 > 0.0f ? s19 : 0.0f;
        float s1 = s2 > 0.0f ? s2 : 0.0f;

        if (s0 == 0.0f) {
            overlaps[i] = 0.0f;
            diffs[i] = s19;
            signs[i] = -1.0f;
            sep_axis = i;
            count++;
        } else if (s1 == 0.0f) {
            overlaps[i] = 0.0f;
            diffs[i] = s2;
            signs[i] = 1.0f;
            sep_axis = i;
            count++;
        } else {
            if (s1 <= s0) {
                diffs[i] = s1;
                overlaps[i] = s1;
                signs[i] = 1.0f;
            } else {
                diffs[i] = s0;
                overlaps[i] = s0;
                signs[i] = -1.0f;
            }
        }

        if (count > 1) {
            return out;
        }

        if (overlaps[i] < min_dist) {
            min_dist = overlaps[i];
        }
    }

    if (hit_distance) {
        *hit_distance = min_dist;
    }

    if (count == 0) {
        if (no_clamp) {
            return out;
        }
        int min_idx = 0;
        if (overlaps[1] < overlaps[0]) {
            min_idx = 1;
        }
        if (overlaps[2] < overlaps[min_idx]) {
            min_idx = 2;
        }
        float val = overlaps[min_idx] * signs[min_idx];
        if (min_idx == 0) out.x = val;
        else if (min_idx == 1) out.y = val;
        else out.z = val;
        return out;
    }

    /* count == 1 */
    float diff = diffs[sep_axis];
    float sign = signs[sep_axis];
    float motion_axis = (sep_axis == 0) ? out.x : ((sep_axis == 1) ? out.y : out.z);

    if ((diff - sign * motion_axis) > 0.0f) {
        float val = diff * sign;
        if (sep_axis == 0) out.x = val;
        else if (sep_axis == 1) out.y = val;
        else out.z = val;
    }

    return out;
}

static inline float aabb_clip_axis_collide(const float *self, const float *other, float motion, int axis, int no_clamp)
{
    if (motion == 0.0f) {
        return 0.0f;
    }
    static const int next_axis[3] = { 1, 2, 0 };
    static const int next_next_axis[3] = { 2, 0, 1 };
    int a1 = next_axis[axis];
    int a2 = next_next_axis[axis];

    if (other[a1 + 3] <= self[a1] || other[a1] >= self[a1 + 3]) {
        return motion;
    }
    if (other[a2 + 3] <= self[a2] || other[a2] >= self[a2 + 3]) {
        return motion;
    }

    float diff1 = self[axis + 3] - other[axis];
    if (diff1 <= 0.0f) {
        if (diff1 - motion > 0.0f) {
            return diff1;
        }
        return motion;
    }

    float diff2 = self[axis] - other[axis + 3];
    if (diff2 >= 0.0f) {
        if (diff2 - motion < 0.0f) {
            return diff2;
        }
        return motion;
    }

    if (no_clamp) {
        return motion;
    }

    float s1 = diff1 > 0.0f ? diff1 : 0.0f;
    float s0 = diff2 < 0.0f ? diff2 : 0.0f;
    float s2 = -s0;
    if (s1 < s2) {
        return s1;
    }
    return s0;
}

float aabb_clip_x_collide(const float *self, const float *other, float motion, int no_clamp)
{
    return aabb_clip_axis_collide(self, other, motion, 0, no_clamp);
}

float aabb_clip_y_collide(const float *self, const float *other, float motion, int no_clamp)
{
    return aabb_clip_axis_collide(self, other, motion, 1, no_clamp);
}

float aabb_clip_z_collide(const float *self, const float *other, float motion, int no_clamp)
{
    return aabb_clip_axis_collide(self, other, motion, 2, no_clamp);
}

void *aabb_expanded_clone(float *result, const float *self, const float *offset)
{
    float min_x = self[0] - offset[0];
    float min_y = self[1] - offset[1];
    float min_z = self[2] - offset[2];
    float max_x = self[3] + offset[0];
    float max_y = self[4] + offset[1];
    float max_z = self[5] + offset[2];
    result[0] = min_x;
    result[1] = min_y;
    result[2] = min_z;
    result[3] = max_x;
    result[4] = max_y;
    result[5] = max_z;
    ((u8 *)result)[24] = (min_x == 0.0f && min_y == 0.0f && min_z == 0.0f &&
                          max_x == 0.0f && max_y == 0.0f && max_z == 0.0f) ? 1 : 0;
    return result;
}

void *aabb_merge(float *result, const float *self, const float *other)
{
    float min_x = self[0] < other[0] ? self[0] : other[0];
    float min_y = self[1] < other[1] ? self[1] : other[1];
    float min_z = self[2] < other[2] ? self[2] : other[2];
    float max_x = self[3] > other[3] ? self[3] : other[3];
    float max_y = self[4] > other[4] ? self[4] : other[4];
    float max_z = self[5] > other[5] ? self[5] : other[5];
    result[0] = min_x;
    result[1] = min_y;
    result[2] = min_z;
    result[3] = max_x;
    result[4] = max_y;
    result[5] = max_z;
    ((u8 *)result)[24] = (min_x == 0.0f && min_y == 0.0f && min_z == 0.0f &&
                          max_x == 0.0f && max_y == 0.0f && max_z == 0.0f) ? 1 : 0;
    return result;
}

void *aabb_center_at(float *self, const float *center)
{
    float half_x = (self[3] - self[0]) * 0.5f;
    float half_y = (self[4] - self[1]) * 0.5f;
    float half_z = (self[5] - self[2]) * 0.5f;
    self[0] = center[0] - half_x;
    self[1] = center[1] - half_y;
    self[2] = center[2] - half_z;
    self[3] = center[0] + half_x;
    self[4] = center[1] + half_y;
    self[5] = center[2] + half_z;
    return self;
}

int aabb_contains_point(const float *self, const float *point)
{
    float px = point[0], py = point[1], pz = point[2];
    if (px <= self[0] || px >= self[3]) return 0;
    if (py <= self[1] || py >= self[4]) return 0;
    if (pz <= self[2] || pz >= self[5]) return 0;
    return 1;
}

int aabb_contains_aabb(const float *self, const float *other)
{
    if (other[0] <= self[0] || other[0] >= self[3]) return 0;
    if (other[1] <= self[1] || other[1] >= self[4]) return 0;
    if (other[2] <= self[2] || other[2] >= self[5]) return 0;
    if (other[3] <= self[0] || other[3] >= self[3]) return 0;
    if (other[4] <= self[1] || other[4] >= self[4]) return 0;
    if (other[5] <= self[2] || other[5] >= self[5]) return 0;
    return 1;
}

void *aabb_expand(float *result, const float *self, const float *offset)
{
    float min_x = self[0];
    float min_y = self[1];
    float min_z = self[2];
    float max_x = self[3];
    float max_y = self[4];
    float max_z = self[5];
    float dx = offset[0];
    float dy = offset[1];
    float dz = offset[2];

    if (dx < 0.0f) {
        min_x += dx;
    } else if (dx > 0.0f) {
        max_x += dx;
    }

    if (dy < 0.0f) {
        min_y += dy;
    } else if (dy > 0.0f) {
        max_y += dy;
    }

    if (dz < 0.0f) {
        min_z += dz;
    } else if (dz > 0.0f) {
        max_z += dz;
    }

    result[0] = min_x;
    result[1] = min_y;
    result[2] = min_z;
    result[3] = max_x;
    result[4] = max_y;
    result[5] = max_z;
    ((u8 *)result)[24] = (self[0] == 0.0f && self[1] == 0.0f && self[2] == 0.0f &&
                          self[3] == 0.0f && self[4] == 0.0f && self[5] == 0.0f) ? 1 : 0;
    return result;
}

int aabb_fast_install_hooks(void)
{
    static const u32 targets[18] = {
        0x00718F60u,
        0x00718F24u,
        0x0071A330u,
        0x0071891Cu,
        0x0052DCA4u,
        0x0052E1E4u,
        0x0052E428u,
        0x007192BCu,
        0x00718F0Cu,
        0x00718F18u,
        0x00719D90u,
        0x007189A0u,
        0x00719DFCu,
        0x0052E178u,
        0x0071A08Cu,
        0x0071A104u,
        0x0071A1D0u,
        0x00718A0Cu
    };
    static const u32 expected[18][2] = {
        { 0xE92D47F0u, 0xE1A05001u },
        { 0xE92D4010u, 0xE1A04001u },
        { 0xE52DE004u, 0xED910A03u },
        { 0xED910A03u, 0xEDD00A00u },
        { 0xE92D41F0u, 0xE1A06000u },
        { 0xE92D41F0u, 0xE1A04002u },
        { 0xE92D4070u, 0xE1A04000u },
        { 0xE1A03002u, 0xE3A02000u },
        { 0xE1A03002u, 0xE3A02001u },
        { 0xE1A03002u, 0xE3A02002u },
        { 0xE52DE004u, 0xED910A03u },
        { 0xE52DE004u, 0xED910A03u },
        { 0xE92D4030u, 0xE281300Cu },
        { 0xEDD00A05u, 0xEDD01A02u },
        { 0xED910A00u, 0xEDD00A00u },
        { 0xED910A00u, 0xEDD02A00u },
        { 0xE92D4070u, 0xE24DD020u },
        { 0xE92D4FFFu, 0xE3A05000u }
    };
    u32 replacements[18];
    u32 i;
    replacements[0] = (u32)aabb_distance_to_sqr_aabb;
    replacements[1] = (u32)aabb_distance_to_sqr_vec3;
    replacements[2] = (u32)aabb_offset;
    replacements[3] = (u32)aabb_intersects;
    replacements[4] = (u32)aabb_set;
    replacements[5] = (u32)aabb_ctor_vec3;
    replacements[6] = (u32)aabb_ctor_floats;
    replacements[7] = (u32)aabb_clip_x_collide;
    replacements[8] = (u32)aabb_clip_y_collide;
    replacements[9] = (u32)aabb_clip_z_collide;
    replacements[10] = (u32)aabb_expanded_clone;
    replacements[11] = (u32)aabb_offset;
    replacements[12] = (u32)aabb_merge;
    replacements[13] = (u32)aabb_center_at;
    replacements[14] = (u32)aabb_contains_point;
    replacements[15] = (u32)aabb_contains_aabb;
    replacements[16] = (u32)aabb_expand;
    replacements[17] = (u32)aabb_clip_collide;

    for (i = 0; i < 18; i++) {
        aabb_hooks[i].target = targets[i];
        aabb_hooks[i].replacement = replacements[i];
        aabb_hooks[i].expected[0] = expected[i][0];
        aabb_hooks[i].expected[1] = expected[i][1];
        if (s->host.install_hook(&aabb_hooks[i])) return -60 - (int)i;
    }
    return 0;
}
