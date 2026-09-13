#include "frustum_cull_fast.h"
#include "../../state.h"

static NuMC3DS_Hook cube_in_frustum_hook;

typedef struct {
    float x, y, z;
} Vec3;

typedef struct {
    float a, b, c, d;
    float pad[12];
} FrustumPlane;

typedef struct {
    FrustumPlane planes[5];
} FrustumData;

__attribute__((optimize("O3")))
static int cube_in_frustum_fast(const FrustumData* frustum, const Vec3* min, const Vec3* max)
{
    float min_x = min->x, min_y = min->y, min_z = min->z;
    float max_x = max->x, max_y = max->y, max_z = max->z;

    for (int i = 0; i < 5; ++i) {
        float a = frustum->planes[i].a;
        float b = frustum->planes[i].b;
        float c = frustum->planes[i].c;
        float d = frustum->planes[i].d;

        float px = (a > 0.0f) ? max_x : min_x;
        float py = (b > 0.0f) ? max_y : min_y;
        float pz = (c > 0.0f) ? max_z : min_z;

        float dot = a * px + b * py + c * pz + d;
        if (dot <= 0.0f) {
            return 0;
        }
    }
    return 1;
}

int frustum_cull_fast_install_hook(void)
{
    cube_in_frustum_hook.target = 0x0018F4F8u;
    cube_in_frustum_hook.replacement = (u32)cube_in_frustum_fast;
    cube_in_frustum_hook.expected[0] = 0xE92D03F0u;
    cube_in_frustum_hook.expected[1] = 0xE3A05000u;
    return s->host.install_hook(&cube_in_frustum_hook) ? -65 : 0;
}
