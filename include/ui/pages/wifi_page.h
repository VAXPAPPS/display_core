#ifndef DC_WIFI_PAGE_H
#define DC_WIFI_PAGE_H

#include <gtk/gtk.h>
#include "services/network_service.h"

typedef struct _DcWifiPage DcWifiPage;

typedef struct {
    void (*on_toggle_wifi)(gboolean enabled, gpointer data);
    void (*on_scan)(gpointer data);
    void (*on_connect)(const char *ap_path,
                       const char *ssid,
                       const char *password,   /* NULL if open */
                       gpointer data);
    void (*on_disconnect)(gpointer data);
    void (*on_forget)(const char *ssid, gpointer data);
} DcWifiPageCallbacks;

DcWifiPage *dc_wifi_page_new(void);
void        dc_wifi_page_free(DcWifiPage *page);
GtkWidget  *dc_wifi_page_get_widget(DcWifiPage *page);

GtkWidget  *dc_wifi_page_get_wifi_switch(DcWifiPage *page);

void dc_wifi_page_set_wifi_enabled(DcWifiPage *page, gboolean enabled);
void dc_wifi_page_set_scanning(DcWifiPage *page, gboolean scanning);

void dc_wifi_page_set_callbacks(DcWifiPage          *page,
                                DcWifiPageCallbacks *cb,
                                gpointer             user_data);

void dc_wifi_page_populate_aps(DcWifiPage *page, GList *aps);

#endif /* DC_WIFI_PAGE_H */
