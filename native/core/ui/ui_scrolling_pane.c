#include "ui_scrolling_pane.h"
#include "ui_runtime.h"

void *ui_scrolling_pane_get_scroll_bar(void *pane) {
    return pane ? (void*)&((UiScrollingPaneView*)pane)->scroll_bar : 0;
}

void ui_scrolling_pane_adjust_content_size(void *pane, int total_content_height) {
    if (!pane) return;
    ((UiScrollingPaneView*)pane)->content_height = total_content_height;
    ((void(*)(void*))SEAM_ScrollingPane_adjustContentSize)(pane);
    ((void(*)(void*))SEAM_ScrollingPane_updateVerticalScrollIndicator)(pane);
}

void ui_scrolling_pane_set_offset(void *pane, float offset) {
    if (pane) {
        ((void(*)(void*, float))SEAM_ScrollingPane_setScrollT)(pane, offset);
    }
}

float ui_scrolling_pane_get_offset(const void *pane) {
    return pane ? ((const UiScrollingPaneView*)pane)->content_offset : 0.0f;
}

void ui_scrolling_pane_snap_to_bounds(void *pane) {
    if (pane) {
        ((void(*)(void*))SEAM_ScrollingPane_snapContentOffsetToBounds)(pane);
    }
}

void ui_scrolling_pane_advance_animation(void *pane) {
    if (pane) {
        ((void(*)(void*))SEAM_ScrollingPane_advanceAnimation)(pane);
    }
}

void *ui_scrolling_container_get_pane(void *container) {
    return container ? *(void**)((u8*)container + 0x7C) : 0;
}

void ui_scrolling_container_relayout(void *screen, void *container) {
    UiContainerView *c = (UiContainerView*)container;
    UiSharedVector *children;
    UiElementView *grid_elem;
    void *pane;
    unsigned count, i;
    int max_bottom, total_content_h;

    (void)screen;
    if (!container) return;
    children = &c->children;
    if (!children->begin || !children->end) return;
    count = (unsigned)(children->end - children->begin);
    if (count == 0) return;

    grid_elem = (UiElementView*)container;
    max_bottom = grid_elem->y;
    for (i = 0; i < count; i++) {
        UiElementView *e = (UiElementView*)children->begin[i].object;
        if (e && e->visible) {
            int b = e->y + e->height;
            if (b > max_bottom) max_bottom = b;
        }
    }

    total_content_h = max_bottom - grid_elem->y + 8;
    *(int*)((u8*)container + 0x80) = total_content_h;

    pane = ui_scrolling_container_get_pane(container);
    if (pane) {
        ui_scrolling_pane_adjust_content_size(pane, total_content_h);
    }
}

void ui_scroll_bar_set_screen(void *scroll_bar, int screen_mask) {
    if (scroll_bar) {
        ((UiElementView*)scroll_bar)->screen_mask = screen_mask;
    }
}
