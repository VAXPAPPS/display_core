#ifndef DC_WINDOW_MANAGER_PAGE_H
#define DC_WINDOW_MANAGER_PAGE_H

#include <gtk/gtk.h>

typedef struct _DcWindowManagerPage DcWindowManagerPage;

DcWindowManagerPage *dc_window_manager_page_new(void);
void dc_window_manager_page_free(DcWindowManagerPage *page);
GtkWidget *dc_window_manager_page_get_widget(DcWindowManagerPage *page);

#endif
