#include "app_controller_internal.h"

static void refresh_devices_list(DcAppController *app) {
    if (!app->bluetooth_page || !app->bluetooth_service) return;
    GList *devices = dc_bluetooth_service_get_devices(app->bluetooth_service);
    dc_bluetooth_page_populate_devices(app->bluetooth_page, devices);
    g_list_free_full(devices, (GDestroyNotify)dc_bluetooth_device_free);
}

static void on_pair_requested(const char *object_path, gpointer data) {
    DcAppController *app = data;
    if (app->bluetooth_service) {
        dc_bluetooth_service_pair_device(app->bluetooth_service, object_path);
    }
}

static void on_connect_requested(const char *object_path, gpointer data) {
    DcAppController *app = data;
    if (app->bluetooth_service) {
        dc_bluetooth_service_connect_device(app->bluetooth_service, object_path);
    }
}

static void on_disconnect_requested(const char *object_path, gpointer data) {
    DcAppController *app = data;
    if (app->bluetooth_service) {
        dc_bluetooth_service_disconnect_device(app->bluetooth_service, object_path);
    }
}

static void on_remove_requested(const char *object_path, gpointer data) {
    DcAppController *app = data;
    if (app->bluetooth_service) {
        dc_bluetooth_service_remove_device(app->bluetooth_service, object_path);
    }
}

static void on_device_added(DcBluetoothService *service, const gchar *path, gpointer user_data) {
    (void)service; (void)path;
    refresh_devices_list((DcAppController *)user_data);
}

static void on_device_removed(DcBluetoothService *service, const gchar *path, gpointer user_data) {
    (void)service; (void)path;
    refresh_devices_list((DcAppController *)user_data);
}

static void on_device_changed(DcBluetoothService *service, const gchar *path, gpointer user_data) {
    (void)service; (void)path;
    refresh_devices_list((DcAppController *)user_data);
}

static void on_adapter_changed(DcBluetoothService *service, gpointer user_data) {
    DcAppController *app = user_data;
    if (!app->bluetooth_page) return;

    gboolean powered = dc_bluetooth_service_get_powered(service);
    GtkWidget *sw = dc_bluetooth_page_get_power_switch(app->bluetooth_page);
    
    g_signal_handlers_block_by_func(sw, G_CALLBACK(gtk_switch_get_active), NULL); 
    gtk_switch_set_active(GTK_SWITCH(sw), powered);
    
    // Refresh list since adapter change might affect it
    refresh_devices_list(app);
}

void dc_app_bluetooth_load(DcAppController *app) {
    if (!app->bluetooth_page || !app->bluetooth_service) return;

    DcBluetoothPageCallbacks cb = {
        .on_pair = on_pair_requested,
        .on_connect = on_connect_requested,
        .on_disconnect = on_disconnect_requested,
        .on_remove = on_remove_requested
    };
    dc_bluetooth_page_set_callbacks(app->bluetooth_page, &cb, app);

    GtkWidget *sw = dc_bluetooth_page_get_power_switch(app->bluetooth_page);
    gboolean powered = dc_bluetooth_service_get_powered(app->bluetooth_service);
    gtk_switch_set_active(GTK_SWITCH(sw), powered);

    refresh_devices_list(app);
}

static void on_power_switched(GtkSwitch *sw, GParamSpec *pspec, gpointer user_data) {
    (void)pspec;
    DcAppController *app = user_data;
    if (!app->bluetooth_service) return;

    gboolean is_on = gtk_switch_get_active(sw);
    dc_bluetooth_service_set_powered(app->bluetooth_service, is_on);

    if (is_on) {
        dc_bluetooth_service_start_discovery(app->bluetooth_service);
    } else {
        dc_bluetooth_service_stop_discovery(app->bluetooth_service);
    }
}

void dc_app_bluetooth_connect_signals(DcAppController *app) {
    if (!app->bluetooth_page || !app->bluetooth_service) return;

    GtkWidget *sw = dc_bluetooth_page_get_power_switch(app->bluetooth_page);
    if (sw) {
        g_signal_connect(sw, "notify::active", G_CALLBACK(on_power_switched), app);
    }

    g_signal_connect(app->bluetooth_service, "device-added", G_CALLBACK(on_device_added), app);
    g_signal_connect(app->bluetooth_service, "device-removed", G_CALLBACK(on_device_removed), app);
    g_signal_connect(app->bluetooth_service, "device-changed", G_CALLBACK(on_device_changed), app);
    g_signal_connect(app->bluetooth_service, "adapter-changed", G_CALLBACK(on_adapter_changed), app);
}
