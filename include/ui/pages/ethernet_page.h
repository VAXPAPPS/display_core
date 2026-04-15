#ifndef DC_ETHERNET_PAGE_H
#define DC_ETHERNET_PAGE_H

#include <gtk/gtk.h>
#include "services/network_service.h"

typedef struct _DcEthernetPage DcEthernetPage;

typedef struct {
    void (*on_disconnect)(const char *device_path, gpointer data);
} DcEthernetPageCallbacks;

DcEthernetPage *dc_ethernet_page_new(void);
void            dc_ethernet_page_free(DcEthernetPage *page);
GtkWidget      *dc_ethernet_page_get_widget(DcEthernetPage *page);

void dc_ethernet_page_set_callbacks(DcEthernetPage          *page,
                                    DcEthernetPageCallbacks *cb,
                                    gpointer                 user_data);

/* Populate or refresh the list of Ethernet adapters */
void dc_ethernet_page_populate_devices(DcEthernetPage  *page,
                                       GList           *devices,   /* DcNetworkDevice* */
                                       GList           *ip_infos); /* DcNetworkIpInfo*, same length */

#endif /* DC_ETHERNET_PAGE_H */
