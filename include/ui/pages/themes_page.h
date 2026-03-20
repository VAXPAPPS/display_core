#ifndef DC_THEMES_PAGE_H
#define DC_THEMES_PAGE_H

#include <gtk/gtk.h>

typedef struct _DcThemesPage DcThemesPage;

DcThemesPage *dc_themes_page_new(void);
void dc_themes_page_free(DcThemesPage *page);
GtkWidget *dc_themes_page_get_widget(DcThemesPage *page);
GtkWidget *dc_themes_page_get_mode_combo(DcThemesPage *page);
GtkWidget *dc_themes_page_get_theme_combo(DcThemesPage *page);
GtkWidget *dc_themes_page_get_icons_combo(DcThemesPage *page);
GtkWidget *dc_themes_page_get_cursor_combo(DcThemesPage *page);
GtkWidget *dc_themes_page_get_font_combo(DcThemesPage *page);
GtkWidget *dc_themes_page_get_mono_font_combo(DcThemesPage *page);
GtkWidget *dc_themes_page_get_cursor_size_scale(DcThemesPage *page);
GtkWidget *dc_themes_page_get_text_scale(DcThemesPage *page);

#endif
