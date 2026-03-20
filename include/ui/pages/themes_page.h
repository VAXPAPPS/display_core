#ifndef DC_THEMES_PAGE_H
#define DC_THEMES_PAGE_H

#include <gtk/gtk.h>

typedef struct _DcThemesPage DcThemesPage;

DcThemesPage *dc_themes_page_new(void);
void dc_themes_page_free(DcThemesPage *page);
GtkWidget *dc_themes_page_get_widget(DcThemesPage *page);

#endif
