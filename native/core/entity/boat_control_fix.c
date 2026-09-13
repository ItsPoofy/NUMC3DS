#include "boat_control_fix.h"
#include "../state.h"

typedef void (*BoatControlFn)(void *);
typedef int (*EntityHasCategoryFn)(void *, u32);
typedef void (__attribute__((pcs("aapcs-vfp"))) *BoatSetPaddleStateFn)(void *, int, float);

static int boat_has_player_controller(void *boat) {
    void **riders;
    void **rider_end;

    if (!boat) return 0;
    riders = *(void ***)((u8 *)boat + SEAM_Boat_riderVectorOffset);
    rider_end = *(void ***)((u8 *)boat + SEAM_Boat_riderVectorOffset + sizeof(void *));
    if (!riders || riders == rider_end || !riders[0]) return 0;
    return ((EntityHasCategoryFn)SEAM_Entity_hasCategory)(riders[0], 1u) != 0;
}

static void boat_clear_paddle_input(void *boat) {
    *(float *)((u8 *)boat + SEAM_Boat_leftPaddleForceOffset) = 0.0f;
    *(float *)((u8 *)boat + SEAM_Boat_rightPaddleForceOffset) = 0.0f;
    ((BoatSetPaddleStateFn)SEAM_Boat_setPaddleState)(boat, 0, 0.0f);
    ((BoatSetPaddleStateFn)SEAM_Boat_setPaddleState)(boat, 1, 0.0f);
}

static void on_boat_control(void *boat) {
    if (boat && !boat_has_player_controller(boat)) boat_clear_paddle_input(boat);
    ((BoatControlFn)s->boat_control.trampoline)(boat);
}

int boat_control_fix_install_hook(void) {
    NuMC3DS_Hook *hook = &s->boat_control;

    hook->target = SEAM_Boat_control;
    hook->replacement = (u32)on_boat_control;
    hook->expected[0] = 0xE92D47F0u;
    hook->expected[1] = 0xE1A04000u;
    return s->host.install_hook(hook) ? -41 : 0;
}
