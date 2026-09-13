#ifndef NUMC3DS_UI_SCROLLING_PANE_H
#define NUMC3DS_UI_SCROLLING_PANE_H

#include "ui_types.h"

/* Returns the embedded ScrollBar component inside a ScrollingPane */
void *ui_scrolling_pane_get_scroll_bar(void *pane);

/* Adjusts the total content height and refreshes vertical scroll indicators */
void ui_scrolling_pane_adjust_content_size(void *pane, int total_content_height);

/* Sets the vertical scroll offset directly via stock ScrollingPane::setScrollT */
void ui_scrolling_pane_set_offset(void *pane, float offset);

/* Returns the current content scroll offset */
float ui_scrolling_pane_get_offset(const void *pane);

/* Snaps content offset back to valid container bounds */
void ui_scrolling_pane_snap_to_bounds(void *pane);

/* Steps animation and momentum deceleration for one tick */
void ui_scrolling_pane_advance_animation(void *pane);

/* Retrieves the ScrollingPane pointer stored inside a scroll container / OptionGrid (offset +0x7C) */
void *ui_scrolling_container_get_pane(void *container);

/* Recalculates total content height from child elements and updates container/pane bounds */
void ui_scrolling_container_relayout(void *screen, void *container);

/* Sets the target screen mask for a ScrollBar (1=Top, 2=Bottom, 3=Both) */
void ui_scroll_bar_set_screen(void *scroll_bar, int screen_mask);

#endif /* NUMC3DS_UI_SCROLLING_PANE_H */
