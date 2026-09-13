#include "opt.h"

#ifndef NUMC3DS_OPTIMIZATIONS
#define NUMC3DS_OPTIMIZATIONS 1
#endif

/* Math & Spatial */
#include "math/aabb_fast.h"
#include "math/math_fast.h"
#include "math/blockpos_fast.h"

/* Render */
#include "render/matrix_stack_fast.h"
#include "render/face_cull_fast.h"
#include "render/uv_fast.h"
#include "render/ao_blend_fast.h"
#include "render/tessellator_color_fast.h"
#include "render/tessellator_tex_fast.h"
#include "render/frustum_cull_fast.h"
#include "render/leaf_cull_fast.h"

/* Lighting & Particle */
#include "lighting/lighting_queue.h"
#include "lighting/light_read_fast.h"
#include "lighting/light_propagate_fast.h"
#include "lighting/particle_light_fast.h"
#include "particle/particle_fast.h"

/* World & Chunks */
#include "world/block_read_fast.h"
#include "world/chunk_deserialize_fast.h"
#include "world/chunk_tick_fast.h"
#include "world/chunk_storage_fast.h"
#include "levelgen/noise_fast.h"

/* Entity AI & Transforms */
#include "entity/entity_fast.h"

/* UI */
#include "ui/font_fast.h"

int opt_install_hooks(void)
{
#if NUMC3DS_OPTIMIZATIONS
    int r;

    /* Math & Spatial */
    if ((r = aabb_fast_install_hooks())) return r;
    if ((r = math_fast_install_hooks())) return r;
    if ((r = blockpos_fast_install_hooks())) return r;

    /* Render Matrix Fast Path & Face Culling */
    if ((r = matrix_stack_fast_install_hooks())) return r;
    if ((r = face_cull_fast_install_hook())) return r;
    if ((r = uv_fast_install_hooks())) return r;
    if ((r = ao_blend_fast_install_hook())) return r;
    if ((r = tessellator_color_fast_install_hook())) return r;
    if ((r = tessellator_tex_fast_install_hook())) return r;
    if ((r = frustum_cull_fast_install_hook())) return r;
    if ((r = leaf_cull_fast_install_hook())) return r;

    /* Lighting Pipeline & Fast Queues */
    if ((r = lighting_queue_install_hooks())) return r;
    if ((r = light_read_fast_install_hooks())) return r;
    if ((r = light_propagate_fast_install_hooks())) return r;
    if ((r = particle_light_fast_install_hooks())) return r;
    if ((r = particle_fast_install_hooks())) return r;

    /* World Chunk Operations & Storage */
    if ((r = block_read_fast_install_hooks())) return r;
    if ((r = chunk_deserialize_fast_install_hooks())) return r;
    if ((r = chunk_tick_fast_install_hooks())) return r;
    if ((r = chunk_storage_fast_install_hooks())) return r;
    if ((r = noise_fast_install_hooks())) return r;

    /* Entity AI Culling & Navigation */
    if ((r = entity_fast_install_hooks())) return r;
    /* UI Font Glyph Measurement */
    if ((r = font_fast_install_hooks())) return r;
#endif

    return 0;
}
