#ifndef NUMC3DS_ENTITY_FAST_H
#define NUMC3DS_ENTITY_FAST_H

int entity_intersects_fast(const void *self, const float *min, const float *max);
float entity_distance_to_vec3_sq_fast(const void *self, const float *pos);
float entity_distance_to_entity_sq_fast(const void *self, const void *other);
float entity_distance_to_vec3_fast(const void *self, const float *pos);
float entity_distance_to_entity_fast(const void *self, const void *other);
void entity_turn_fast(void *self, const float *rot, int slow);
void entity_set_rot_fast(void *self, const float *rot);
void entity_sub_hit_fast(void *self, const float *point, const float *inflation);

int entity_fast_install_hooks(void);

#endif /* NUMC3DS_ENTITY_FAST_H */
