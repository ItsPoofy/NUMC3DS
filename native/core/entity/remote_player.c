#include "remote_player.h"
#include "../hook_manager.h"
#include "../seams.h"
#include "../state.h"

typedef void (*PlayerDieFn)(void *, void *);
typedef void (*MobDieFn)(void *, void *);

static NuMC3DS_Hook remote_player_die_hook;

static void on_player_die(void *player, void *source) {
    if (!player) return;
    if (*(u32 *)player == SEAM_RemotePlayer_vtable || *(void **)((u8 *)player + 0x18ECu) == 0) {
        ((MobDieFn)SEAM_Mob_die)(player, source);
        return;
    }
    ((PlayerDieFn)remote_player_die_hook.trampoline)(player, source);
}

int remote_player_install_hooks(void) {
    remote_player_die_hook.target = SEAM_Player_die;
    remote_player_die_hook.replacement = (u32)on_player_die;
    remote_player_die_hook.expected[0] = 0xE92D4070u;
    remote_player_die_hook.expected[1] = 0xE1A06001u;
    return s->host.install_hook(&remote_player_die_hook);
}
