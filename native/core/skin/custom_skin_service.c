#include "custom_skin_service.h"
#include "skin_opt.h"
#include "skin_service.h"
#include "skin_picker_hooks.h"
#include "skin_texture_loader.h"
#include "remote_skin_service.h"

int custom_skin_service_install_hooks(void)
{
    int result;

    result = skin_opt_install_hooks();
    if (result) return result;

    result = skin_service_init();
    if (result < 0) return result;

    result = skin_texture_loader_install_hook();
    if (result) return result;

    result = skin_picker_hooks_install();
    if (result) return result;

    result = remote_skin_service_install_hook();
    if (result) return result;

    return 0;
}
