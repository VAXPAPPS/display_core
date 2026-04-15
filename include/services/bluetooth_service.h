#ifndef DC_BLUETOOTH_SERVICE_H
#define DC_BLUETOOTH_SERVICE_H

#include <glib-object.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define DC_TYPE_BLUETOOTH_SERVICE (dc_bluetooth_service_get_type())
G_DECLARE_FINAL_TYPE(DcBluetoothService, dc_bluetooth_service, DC, BLUETOOTH_SERVICE, GObject)

typedef struct _DcBluetoothDevice {
    char *object_path;
    char *address;
    char *name;
    char *icon;
    gboolean paired;
    gboolean connected;
    gboolean trusted;
} DcBluetoothDevice;

void dc_bluetooth_device_free(DcBluetoothDevice *device);

DcBluetoothService *dc_bluetooth_service_new(void);

gboolean dc_bluetooth_service_get_powered(DcBluetoothService *self);
void     dc_bluetooth_service_set_powered(DcBluetoothService *self, gboolean powered);

gboolean dc_bluetooth_service_get_discovering(DcBluetoothService *self);
void     dc_bluetooth_service_start_discovery(DcBluetoothService *self);
void     dc_bluetooth_service_stop_discovery(DcBluetoothService *self);

GList *dc_bluetooth_service_get_devices(DcBluetoothService *self);

void dc_bluetooth_service_pair_device(DcBluetoothService *self, const char *object_path);
void dc_bluetooth_service_connect_device(DcBluetoothService *self, const char *object_path);
void dc_bluetooth_service_disconnect_device(DcBluetoothService *self, const char *object_path);
void dc_bluetooth_service_remove_device(DcBluetoothService *self, const char *object_path);

G_END_DECLS

#endif /* DC_BLUETOOTH_SERVICE_H */
