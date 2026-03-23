#ifndef DC_SYSTEM_PAGE_H
#define DC_SYSTEM_PAGE_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _DcSystemPage DcSystemPage;

DcSystemPage *dc_system_page_new(void);
void dc_system_page_free(DcSystemPage *page);

GtkWidget *dc_system_page_get_widget(DcSystemPage *page);

GtkWidget *dc_system_page_get_locale_combo(DcSystemPage *page);
GtkWidget *dc_system_page_get_timezone_combo(DcSystemPage *page);
GtkWidget *dc_system_page_get_ntp_switch(DcSystemPage *page);

G_END_DECLS

#endif /* DC_SYSTEM_PAGE_H */
