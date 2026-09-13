#ifndef NUMC3DS_AABB_FAST_H
#define NUMC3DS_AABB_FAST_H

void *aabb_offset(float *result, const float *src, const float *offset);
float aabb_distance_to_sqr_aabb(const float *self, const float *other);
float aabb_distance_to_sqr_vec3(const float *self, const float *point);
int aabb_intersects(const float *self, const float *other);
void *aabb_set(float *self, const float *min, const float *max);
void *aabb_ctor_vec3(float *self, const float *min, const float *max);
void *aabb_ctor_floats(float *self, float minX, float minY, float minZ,
                       float maxX, float maxY, float maxZ);
typedef struct { float x, y, z; } Vec3Float;
Vec3Float aabb_clip_collide(const float *other, const float *self, const float *motion, int no_clamp, float *hit_distance);
float aabb_clip_x_collide(const float *self, const float *other, float motion, int no_clamp);
float aabb_clip_y_collide(const float *self, const float *other, float motion, int no_clamp);
float aabb_clip_z_collide(const float *self, const float *other, float motion, int no_clamp);

void *aabb_expanded_clone(float *result, const float *self, const float *offset);
void *aabb_merge(float *result, const float *self, const float *other);
void *aabb_center_at(float *self, const float *center);
int aabb_contains_point(const float *self, const float *point);
int aabb_contains_aabb(const float *self, const float *other);
void *aabb_expand(float *result, const float *self, const float *offset);

int aabb_fast_install_hooks(void);

#endif
