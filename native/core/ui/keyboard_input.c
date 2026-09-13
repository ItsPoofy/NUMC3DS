#include "keyboard_input.h"

static const char inputfail[]="NuMC3DS chat: controller input hook failed\n";

static void poll_controllers(void*controller){
    s->keyboard.keyboard_controller=controller;
    ((ThisFn)s->keyboard_input_poll.trampoline)(controller);
}

int keyboard_input_install_hook(void){
    NuMC3DS_Hook*hook=&s->keyboard_input_poll;
    hook->target=SEAM_InputPlatform_pollControllers;
    hook->replacement=(u32)poll_controllers;
    hook->expected[0]=0xE92D4FF0u;
    hook->expected[1]=0xE24DD054u;
    if(s->host.install_hook(hook)){
        s->host.debug_string(inputfail,sizeof(inputfail)-1);
        return -15;
    }
    return 0;
}

int keyboard_input_sample(u32*hold,u32*trigger){
    u8*status;
    if(hold)*hold=0;
    if(trigger)*trigger=0;
    if(!s->keyboard.keyboard_controller)return 0;
    status=(u8*)s->keyboard.keyboard_controller+SEAM_GameController_statusOffset;
    if(hold)*hold=*(u32*)(status+8);
    if(trigger)*trigger=*(u32*)(status+12);
    return 1;
}
