#ifndef DC_DISPLAY_EDIT_PAGE_H
#define DC_DISPLAY_EDIT_PAGE_H

#include <gtk/gtk.h>

typedef struct _DcDisplayEditPage DcDisplayEditPage;

DcDisplayEditPage *dc_display_edit_page_new(void);
void dc_display_edit_page_free(DcDisplayEditPage *page);
GtkWidget *dc_display_edit_page_get_widget(DcDisplayEditPage *page);
GtkWidget *dc_display_edit_page_get_night_light_switch(DcDisplayEditPage *page);
GtkWidget *dc_display_edit_page_get_night_light_use_schedule_switch(DcDisplayEditPage *page);
GtkWidget *dc_display_edit_page_get_night_light_temperature_scale(DcDisplayEditPage *page);
GtkWidget *dc_display_edit_page_get_night_light_schedule_combo(DcDisplayEditPage *page);
GtkWidget *dc_display_edit_page_get_night_light_custom_start_spin(DcDisplayEditPage *page);
GtkWidget *dc_display_edit_page_get_night_light_custom_end_spin(DcDisplayEditPage *page);
GtkWidget *dc_display_edit_page_get_night_light_status_label(DcDisplayEditPage *page);
GtkWidget *dc_display_edit_page_get_vrr_switch(DcDisplayEditPage *page);
GtkWidget *dc_display_edit_page_get_vrr_status_label(DcDisplayEditPage *page);
GtkWidget *dc_display_edit_page_get_adaptive_brightness_switch(DcDisplayEditPage *page);
GtkWidget *dc_display_edit_page_get_gamma_scale(DcDisplayEditPage *page);
GtkWidget *dc_display_edit_page_get_vibrance_scale(DcDisplayEditPage *page);

#endif
