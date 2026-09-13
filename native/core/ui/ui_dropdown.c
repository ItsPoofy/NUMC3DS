#include "ui_dropdown.h"
int ui_dropdown_create(void *screen,int id,int x,int y,int width,int height,const UiDropdownOptions *options,UiShared *out){
    void *memory,*control;
    if(!options||!options->keys||!options->count)return 0;
    memory=((GameAllocWithSelector)SEAM_Heap_allocWithSelector)(0xf4,(void*)SEAM_game_alloc_selector);
    if(!memory)return 0;
    control=((void*(*)(void*,void*,int,int,int,int,int))0x005D9BACu)(memory,ui_screen_game(screen),id,x,y,width,height);
    if(!control)return 0;
    ((void*(*)(void*,const char*const*,int,void*))0x005D94D4u)(control,options->keys,options->count,options->listener);
    ((UiButtonView*)control)->drop_shadow=0;
    ((UiButtonView*)control)->owner=ui_screen_button_owner(screen);
    ui_element_set_screen(control,0x80);ui_element_set_visible(control,1);
    if(!ui_shared_from_object(control,out))return 0;
    ui_screen_add_button(screen,out);return 1;
}
int ui_dropdown_value(void *control){return control?*(int*)((u8*)control+0xd0):0;}
void ui_dropdown_set_value(void *control,int value){if(control)((void(*)(void*,int))0x005D90B4u)(control,value);}
int ui_dropdown_opened(void *control){return control&&*((u8*)control+0xca);}
void ui_dropdown_input(void *screen,void *control,int binding){if(control)((void(*)(void*,void*,int))0x005D92FCu)(control,ui_screen_game(screen),binding);}
void ui_dropdown_draw_popup(void *screen,void *control,int x,int y){if(ui_dropdown_opened(control))((void(*)(void*,void*,int,int))0x005D99B0u)(control,ui_screen_game(screen),x,y);}
