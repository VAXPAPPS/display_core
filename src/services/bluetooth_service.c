#include "services/bluetooth_service.h"

struct _DcBluetoothService {
    GObject parent_instance;
    GDBusConnection *system_bus;
    char *default_adapter_path;
};

G_DEFINE_TYPE(DcBluetoothService, dc_bluetooth_service, G_TYPE_OBJECT)

enum {
    PROP_0,
    LAST_PROP
};

enum {
    DEVICE_ADDED,
    DEVICE_REMOVED,
    DEVICE_CHANGED,
    ADAPTER_CHANGED,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

static void find_default_adapter(DcBluetoothService *self) {
    if (!self->system_bus) return;

    GError *error = NULL;
    GVariant *res = g_dbus_connection_call_sync(self->system_bus,
        "org.bluez", "/", "org.freedesktop.DBus.ObjectManager", "GetManagedObjects",
        NULL, G_VARIANT_TYPE("(a{oa{sa{sv}}})"), G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);

    if (error) {
        g_warning("Failed to get BlueZ managed objects: %s", error->message);
        g_error_free(error);
        return;
    }

    if (res) {
        GVariantIter *iter;
        gchar *object_path;
        GVariant *interfaces_and_properties;

        g_variant_get(res, "(a{oa{sa{sv}}})", &iter);
        while (g_variant_iter_loop(iter, "{&o@a{sa{sv}}}", &object_path, &interfaces_and_properties)) {
            if (g_variant_lookup(interfaces_and_properties, "org.bluez.Adapter1", "@a{sv}", NULL)) {
                if (!self->default_adapter_path) {
                    self->default_adapter_path = g_strdup(object_path);
                }
            }
        }
        g_variant_iter_free(iter);
        g_variant_unref(res);
    }
}

static void on_properties_changed(GDBusConnection *connection,
                                  const gchar *sender_name,
                                  const gchar *object_path,
                                  const gchar *interface_name,
                                  const gchar *signal_name,
                                  GVariant *parameters,
                                  gpointer user_data) {
    (void)connection; (void)sender_name; (void)signal_name; (void)parameters;
    DcBluetoothService *self = DC_BLUETOOTH_SERVICE(user_data);

    if (g_strcmp0(interface_name, "org.freedesktop.DBus.Properties") == 0) {
        const gchar *changed_interface;
        g_variant_get_child(parameters, 0, "&s", &changed_interface);

        if (g_strcmp0(changed_interface, "org.bluez.Adapter1") == 0) {
            g_signal_emit(self, signals[ADAPTER_CHANGED], 0);
        } else if (g_strcmp0(changed_interface, "org.bluez.Device1") == 0) {
            g_signal_emit(self, signals[DEVICE_CHANGED], 0, object_path);
        }
    }
}

static void on_interfaces_added(GDBusConnection *connection,
                                const gchar *sender_name,
                                const gchar *object_path,
                                const gchar *interface_name,
                                const gchar *signal_name,
                                GVariant *parameters,
                                gpointer user_data) {
    (void)connection; (void)sender_name; (void)interface_name; (void)signal_name; (void)parameters;
    DcBluetoothService *self = DC_BLUETOOTH_SERVICE(user_data);
    g_signal_emit(self, signals[DEVICE_ADDED], 0, object_path);
}

static void on_interfaces_removed(GDBusConnection *connection,
                                  const gchar *sender_name,
                                  const gchar *object_path,
                                  const gchar *interface_name,
                                  const gchar *signal_name,
                                  GVariant *parameters,
                                  gpointer user_data) {
    (void)connection; (void)sender_name; (void)interface_name; (void)signal_name; (void)parameters;
    DcBluetoothService *self = DC_BLUETOOTH_SERVICE(user_data);
    g_signal_emit(self, signals[DEVICE_REMOVED], 0, object_path);
}

static void dc_bluetooth_service_init(DcBluetoothService *self) {
    GError *error = NULL;
    self->system_bus = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
    if (error) {
        g_warning("Could not connect to system bus: %s", error->message);
        g_error_free(error);
        return;
    }

    find_default_adapter(self);

    if (self->system_bus) {
        g_dbus_connection_signal_subscribe(self->system_bus,
                                           "org.bluez",
                                           "org.freedesktop.DBus.Properties",
                                           "PropertiesChanged",
                                           NULL, NULL, G_DBUS_SIGNAL_FLAGS_NONE,
                                           on_properties_changed, self, NULL);

        g_dbus_connection_signal_subscribe(self->system_bus,
                                           "org.bluez",
                                           "org.freedesktop.DBus.ObjectManager",
                                           "InterfacesAdded",
                                           NULL, NULL, G_DBUS_SIGNAL_FLAGS_NONE,
                                           on_interfaces_added, self, NULL);

        g_dbus_connection_signal_subscribe(self->system_bus,
                                           "org.bluez",
                                           "org.freedesktop.DBus.ObjectManager",
                                           "InterfacesRemoved",
                                           NULL, NULL, G_DBUS_SIGNAL_FLAGS_NONE,
                                           on_interfaces_removed, self, NULL);
    }
}

static void dc_bluetooth_service_dispose(GObject *object) {
    DcBluetoothService *self = DC_BLUETOOTH_SERVICE(object);
    if (self->system_bus) {
        g_object_unref(self->system_bus);
        self->system_bus = NULL;
    }
    if (self->default_adapter_path) {
        g_free(self->default_adapter_path);
        self->default_adapter_path = NULL;
    }
    G_OBJECT_CLASS(dc_bluetooth_service_parent_class)->dispose(object);
}

static void dc_bluetooth_service_class_init(DcBluetoothServiceClass *klass) {
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dc_bluetooth_service_dispose;

    signals[DEVICE_ADDED] = g_signal_new("device-added", G_TYPE_FROM_CLASS(klass),
        G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_STRING);
    signals[DEVICE_REMOVED] = g_signal_new("device-removed", G_TYPE_FROM_CLASS(klass),
        G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_STRING);
    signals[DEVICE_CHANGED] = g_signal_new("device-changed", G_TYPE_FROM_CLASS(klass),
        G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_STRING);
    signals[ADAPTER_CHANGED] = g_signal_new("adapter-changed", G_TYPE_FROM_CLASS(klass),
        G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL, G_TYPE_NONE, 0);
}

DcBluetoothService *dc_bluetooth_service_new(void) {
    return g_object_new(DC_TYPE_BLUETOOTH_SERVICE, NULL);
}

static GVariant *get_property(DcBluetoothService *self, const char *path, const char *interface, const char *property) {
    if (!self->system_bus || !path) return NULL;
    GError *error = NULL;
    GVariant *res = g_dbus_connection_call_sync(self->system_bus,
        "org.bluez", path, "org.freedesktop.DBus.Properties", "Get",
        g_variant_new("(ss)", interface, property),
        G_VARIANT_TYPE("(v)"), G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
    if (error) {
        g_error_free(error);
        return NULL;
    }
    GVariant *val = NULL;
    g_variant_get(res, "(v)", &val);
    g_variant_unref(res);
    return val;
}

static void set_property(DcBluetoothService *self, const char *path, const char *interface, const char *property, GVariant *value) {
    if (!self->system_bus || !path) {
        if (value) g_variant_unref(value);
        return;
    }
    g_dbus_connection_call(self->system_bus, "org.bluez", path, "org.freedesktop.DBus.Properties", "Set",
        g_variant_new("(ssv)", interface, property, value), NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL, NULL);
}

gboolean dc_bluetooth_service_get_powered(DcBluetoothService *self) {
    GVariant *val = get_property(self, self->default_adapter_path, "org.bluez.Adapter1", "Powered");
    if (val) {
        gboolean powered = g_variant_get_boolean(val);
        g_variant_unref(val);
        return powered;
    }
    return FALSE;
}

void dc_bluetooth_service_set_powered(DcBluetoothService *self, gboolean powered) {
    set_property(self, self->default_adapter_path, "org.bluez.Adapter1", "Powered", g_variant_new_boolean(powered));
}

gboolean dc_bluetooth_service_get_discovering(DcBluetoothService *self) {
    GVariant *val = get_property(self, self->default_adapter_path, "org.bluez.Adapter1", "Discovering");
    if (val) {
        gboolean discovering = g_variant_get_boolean(val);
        g_variant_unref(val);
        return discovering;
    }
    return FALSE;
}

void dc_bluetooth_service_start_discovery(DcBluetoothService *self) {
    if (!self->system_bus || !self->default_adapter_path) return;
    g_dbus_connection_call(self->system_bus, "org.bluez", self->default_adapter_path, "org.bluez.Adapter1", "StartDiscovery",
        NULL, NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL, NULL);
}

void dc_bluetooth_service_stop_discovery(DcBluetoothService *self) {
    if (!self->system_bus || !self->default_adapter_path) return;
    g_dbus_connection_call(self->system_bus, "org.bluez", self->default_adapter_path, "org.bluez.Adapter1", "StopDiscovery",
        NULL, NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL, NULL);
}

void dc_bluetooth_device_free(DcBluetoothDevice *device) {
    if (!device) return;
    if (device->object_path) g_free(device->object_path);
    if (device->address) g_free(device->address);
    if (device->name) g_free(device->name);
    if (device->icon) g_free(device->icon);
    g_free(device);
}

GList *dc_bluetooth_service_get_devices(DcBluetoothService *self) {
    if (!self->system_bus) return NULL;

    GError *error = NULL;
    GVariant *res = g_dbus_connection_call_sync(self->system_bus,
        "org.bluez", "/", "org.freedesktop.DBus.ObjectManager", "GetManagedObjects",
        NULL, G_VARIANT_TYPE("(a{oa{sa{sv}}})"), G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);

    if (error || !res) {
        if (error) g_error_free(error);
        return NULL;
    }

    GList *devices = NULL;
    GVariantIter *iter;
    gchar *object_path;
    GVariant *interfaces_and_properties;

    g_variant_get(res, "(a{oa{sa{sv}}})", &iter);
    while (g_variant_iter_loop(iter, "{&o@a{sa{sv}}}", &object_path, &interfaces_and_properties)) {
        GVariant *dev_props = g_variant_lookup_value(interfaces_and_properties, "org.bluez.Device1", G_VARIANT_TYPE("a{sv}"));
        if (dev_props) {
            DcBluetoothDevice *dev = g_new0(DcBluetoothDevice, 1);
            dev->object_path = g_strdup(object_path);

            g_variant_lookup(dev_props, "Address", "s", &dev->address);
            if (!g_variant_lookup(dev_props, "Name", "s", &dev->name)) {
                if (!g_variant_lookup(dev_props, "Alias", "s", &dev->name)) {
                    dev->name = g_strdup(dev->address ? dev->address : "Unknown Device");
                }
            }
            g_variant_lookup(dev_props, "Icon", "s", &dev->icon);
            g_variant_lookup(dev_props, "Paired", "b", &dev->paired);
            g_variant_lookup(dev_props, "Connected", "b", &dev->connected);
            g_variant_lookup(dev_props, "Trusted", "b", &dev->trusted);

            devices = g_list_append(devices, dev);
            g_variant_unref(dev_props);
        }
    }
    g_variant_iter_free(iter);
    g_variant_unref(res);

    return devices;
}

void dc_bluetooth_service_pair_device(DcBluetoothService *self, const char *object_path) {
    if (!self->system_bus || !object_path) return;
    g_dbus_connection_call(self->system_bus, "org.bluez", object_path, "org.bluez.Device1", "Pair",
        NULL, NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL, NULL);
}

void dc_bluetooth_service_connect_device(DcBluetoothService *self, const char *object_path) {
    if (!self->system_bus || !object_path) return;
    g_dbus_connection_call(self->system_bus, "org.bluez", object_path, "org.bluez.Device1", "Connect",
        NULL, NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL, NULL);
}

void dc_bluetooth_service_disconnect_device(DcBluetoothService *self, const char *object_path) {
    if (!self->system_bus || !object_path) return;
    g_dbus_connection_call(self->system_bus, "org.bluez", object_path, "org.bluez.Device1", "Disconnect",
        NULL, NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL, NULL);
}

void dc_bluetooth_service_remove_device(DcBluetoothService *self, const char *object_path) {
    if (!self->system_bus || !self->default_adapter_path || !object_path) return;
    g_dbus_connection_call(self->system_bus, "org.bluez", self->default_adapter_path, "org.bluez.Adapter1", "RemoveDevice",
        g_variant_new("(o)", object_path), NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL, NULL);
}
