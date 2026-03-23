#ifndef DC_DEFAULT_APPS_PAGE_H
#define DC_DEFAULT_APPS_PAGE_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _DcDefaultAppsPage DcDefaultAppsPage;

DcDefaultAppsPage *dc_default_apps_page_new(void);
void dc_default_apps_page_free(DcDefaultAppsPage *page);

GtkWidget *dc_default_apps_page_get_widget(DcDefaultAppsPage *page);

GtkWidget *dc_default_apps_page_get_combo(DcDefaultAppsPage *page, const char *category);

G_END_DECLS

#endif /* DC_DEFAULT_APPS_PAGE_H */
