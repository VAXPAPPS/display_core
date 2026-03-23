#ifndef DC_MOUSE_PAGE_H
#define DC_MOUSE_PAGE_H

#include <gtk/gtk.h>

typedef struct _DcMousePage DcMousePage;

DcMousePage *dc_mouse_page_new(void);
void dc_mouse_page_free(DcMousePage *page);

GtkWidget *dc_mouse_page_get_widget(DcMousePage *page);

/* Mouse widgets */
GtkWidget *dc_mouse_page_get_mouse_speed_scale(DcMousePage *page);
GtkWidget *dc_mouse_page_get_mouse_accel_scale(DcMousePage *page);
GtkWidget *dc_mouse_page_get_mouse_natural_scroll_switch(DcMousePage *page);
GtkWidget *dc_mouse_page_get_mouse_left_handed_switch(DcMousePage *page);

/* Touchpad widgets */
GtkWidget *dc_mouse_page_get_touchpad_enabled_switch(DcMousePage *page);
GtkWidget *dc_mouse_page_get_touchpad_tap_to_click_switch(DcMousePage *page);
GtkWidget *dc_mouse_page_get_touchpad_natural_scroll_switch(DcMousePage *page);
GtkWidget *dc_mouse_page_get_touchpad_scroll_method_combo(DcMousePage *page);
GtkWidget *dc_mouse_page_get_touchpad_speed_scale(DcMousePage *page);
GtkWidget *dc_mouse_page_get_touchpad_disable_while_typing_switch(DcMousePage *page);

#endif // DC_MOUSE_PAGE_H
