#include "particle_fast.h"
#include "particle_break_progress.h"
#include "particle_move_fast.h"
#include "particle_offsets_fast.h"

int particle_fast_install_hooks(void)
{
    int result = particle_move_fast_install_hook();
    if (result) return result;
    result = particle_offsets_fast_install_hook();
    if (result) return result;
    return particle_break_progress_install_hook();
}
