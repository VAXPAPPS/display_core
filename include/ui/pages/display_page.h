#ifndef DC_DISPLAY_PAGE_H
#define DC_DISPLAY_PAGE_H

#include <gtk/gtk.h>

typedef struct _DcDisplayPage DcDisplayPage;

DcDisplayPage *dc_display_page_new(GtkWidget *preview_widget);
void dc_display_page_free(DcDisplayPage *page);

GtkWidget *dc_display_page_get_widget(DcDisplayPage *page);
GtkWidget *dc_display_page_get_content_box(DcDisplayPage *page);
GtkWidget *dc_display_page_get_status_label(DcDisplayPage *page);
GtkWidget *dc_display_page_get_profile_combo(DcDisplayPage *page);
GtkWidget *dc_display_page_get_profile_entry(DcDisplayPage *page);

GtkWidget *dc_display_page_get_refresh_button(DcDisplayPage *page);
GtkWidget *dc_display_page_get_apply_button(DcDisplayPage *page);
GtkWidget *dc_display_page_get_extend_button(DcDisplayPage *page);
GtkWidget *dc_display_page_get_mirror_button(DcDisplayPage *page);
GtkWidget *dc_display_page_get_internal_button(DcDisplayPage *page);
GtkWidget *dc_display_page_get_external_button(DcDisplayPage *page);
GtkWidget *dc_display_page_get_save_profile_button(DcDisplayPage *page);
GtkWidget *dc_display_page_get_load_profile_button(DcDisplayPage *page);

#endif
