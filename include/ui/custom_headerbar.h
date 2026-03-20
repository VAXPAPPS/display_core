#ifndef DC_CUSTOM_HEADERBAR_H
#define DC_CUSTOM_HEADERBAR_H

#include <gtk/gtk.h>

GtkWidget *dc_custom_headerbar_new(GtkWindow *window,
                                   const char *title_text,
                                   GtkWidget *center_widget);

#endif
