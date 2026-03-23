#ifndef DC_ABOUT_PAGE_H
#define DC_ABOUT_PAGE_H

#include <gtk/gtk.h>

typedef struct _DcAboutPage DcAboutPage;

DcAboutPage *dc_about_page_new(void);
void dc_about_page_free(DcAboutPage *page);

GtkWidget *dc_about_page_get_widget(DcAboutPage *page);

void dc_about_page_set_os_info(DcAboutPage *page, const char *distro, const char *de, const char *kernel, const char *os_type, const char *model, const char *shell);
void dc_about_page_set_hardware_info(DcAboutPage *page, const char *ram, const char *disk);
void dc_about_page_set_graphics_info(DcAboutPage *page, const char *graphics);

#endif // DC_ABOUT_PAGE_H
