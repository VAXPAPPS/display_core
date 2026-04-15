#ifndef DC_BLUETOOTH_PAGE_H
#define DC_BLUETOOTH_PAGE_H

#include <gtk/gtk.h>
#include "services/bluetooth_service.h"

typedef struct _DcBluetoothPage DcBluetoothPage;

typedef struct {
    void (*on_pair)(const char *object_path, gpointer data);
    void (*on_connect)(const char *object_path, gpointer data);
    void (*on_disconnect)(const char *object_path, gpointer data);
    void (*on_remove)(const char *object_path, gpointer data);
} DcBluetoothPageCallbacks;

DcBluetoothPage *dc_bluetooth_page_new(void);
void             dc_bluetooth_page_free(DcBluetoothPage *page);
GtkWidget       *dc_bluetooth_page_get_widget(DcBluetoothPage *page);
GtkWidget       *dc_bluetooth_page_get_power_switch(DcBluetoothPage *page);

/* To handle dynamic lists */
void dc_bluetooth_page_set_callbacks(DcBluetoothPage *page, DcBluetoothPageCallbacks *cb, gpointer user_data);
void dc_bluetooth_page_populate_devices(DcBluetoothPage *page, GList *devices);

#endif /* DC_BLUETOOTH_PAGE_H */
