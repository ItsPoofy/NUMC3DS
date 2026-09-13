#include "ui_grid_row.h"

int ui_grid_append_row(void *screen,void *container,UiIntVector *rows,UiShared *left,UiShared *right,void *target_before){
    (void)screen;
    if(!container||!rows||!left||!right||!left->object||!left->control||!right->object||!right->control)return 0;
    ui_container_add_shared(container,left,1);
    ui_container_add_shared(container,right,1);
    ui_int_vector_push(rows,2);
    if(target_before&&!ui_grid_move_appended_row_before(container,rows,left->object,right->object,target_before))return 0;
    return 1;
}
