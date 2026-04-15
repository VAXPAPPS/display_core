#include "app_controller_internal.h"

/* ═══════════════════════════════════════════════════════════════════════════
 * Internal helpers
 * ═══════════════════════════════════════════════════════════════════════════ */

/* Refresh the Wi-Fi AP list from the service into the page */
static void refresh_ap_list(DcAppController *app)
{
    if (!app->wifi_page || !app->network_service) return;

    char *wifi_dev = dc_network_service_get_wifi_device_path(app->network_service);
    if (!wifi_dev) {
        dc_wifi_page_populate_aps(app->wifi_page, NULL);
        return;
    }

    GList *aps = dc_network_service_get_access_points(app->network_service, wifi_dev);
    dc_wifi_page_populate_aps(app->wifi_page, aps);
    g_list_free_full(aps, (GDestroyNotify)dc_access_point_free);
    g_free(wifi_dev);
}

/* Refresh Ethernet adapter list */
static void refresh_ethernet(DcAppController *app)
{
    if (!app->ethernet_page || !app->network_service) return;

    GList *devs = dc_network_service_get_ethernet_devices(app->network_service);

    /* Build matching list of IP info in the same order */
    GList *ip_list = NULL;
    for (GList *it = devs; it; it = it->next) {
        DcNetworkDevice *dev  = it->data;
        DcNetworkIpInfo *info =
            dc_network_service_get_ip_info(app->network_service, dev->object_path);
        ip_list = g_list_append(ip_list, info);   /* info may be NULL */
    }

    dc_ethernet_page_populate_devices(app->ethernet_page, devs, ip_list);

    g_list_free_full(devs,    (GDestroyNotify)dc_network_device_free);
    g_list_free_full(ip_list, (GDestroyNotify)dc_network_ip_info_free);
}

/* ─────────────────────────────────────────────────────────────────────────── */

/* Called after a RequestScan — wait briefly then refresh */
static gboolean on_scan_done_idle(gpointer user_data)
{
    DcAppController *app = user_data;
    dc_wifi_page_set_scanning(app->wifi_page, FALSE);
    refresh_ap_list(app);
    return G_SOURCE_REMOVE;
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Wi-Fi page callbacks (page → controller)
 * ═══════════════════════════════════════════════════════════════════════════ */

static void on_wifi_toggle(gboolean enabled, gpointer data)
{
    DcAppController *app = data;
    if (!app->network_service) return;
    dc_network_service_set_wifi_enabled(app->network_service, enabled);

    if (enabled) {
        /* Trigger an immediate scan so the list populates quickly */
        char *wifi_dev = dc_network_service_get_wifi_device_path(app->network_service);
        if (wifi_dev) {
            dc_network_service_request_scan(app->network_service, wifi_dev);
            g_free(wifi_dev);
        }
    } else {
        /* Clear APs display when disabled */
        dc_wifi_page_populate_aps(app->wifi_page, NULL);
    }
}

static void on_wifi_scan(gpointer data)
{
    DcAppController *app = data;
    if (!app->network_service || !app->wifi_page) return;

    dc_wifi_page_set_scanning(app->wifi_page, TRUE);

    char *wifi_dev = dc_network_service_get_wifi_device_path(app->network_service);
    if (wifi_dev) {
        dc_network_service_request_scan(app->network_service, wifi_dev);
        g_free(wifi_dev);
    }

    /* NetworkManager scan typically takes 3-5 s; refresh after 4 s */
    g_timeout_add_seconds(4, on_scan_done_idle, app);
}

static void on_wifi_connect(const char *ap_path,
                             const char *ssid,
                             const char *password,
                             gpointer    data)
{
    DcAppController *app = data;
    if (!app->network_service) return;

    char *wifi_dev = dc_network_service_get_wifi_device_path(app->network_service);
    if (!wifi_dev) return;

    dc_network_service_connect_to_ap(app->network_service,
                                     wifi_dev, ap_path, ssid, password);
    g_free(wifi_dev);

    /* Refresh after short delay to show new state */
    g_timeout_add_seconds(2, on_scan_done_idle, app);
}

static void on_wifi_disconnect(gpointer data)
{
    DcAppController *app = data;
    if (!app->network_service) return;

    char *wifi_dev = dc_network_service_get_wifi_device_path(app->network_service);
    if (!wifi_dev) return;

    dc_network_service_disconnect(app->network_service, wifi_dev);
    g_free(wifi_dev);

    g_timeout_add_seconds(2, on_scan_done_idle, app);
}

static void on_wifi_forget(const char *ssid, gpointer data)
{
    DcAppController *app = data;
    if (!app->network_service || !ssid) return;

    dc_network_service_forget_ap(app->network_service, ssid);
    
    /* Give NetworkManager a moment to process the deletion, then refresh */
    g_timeout_add_seconds(1, on_scan_done_idle, app);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Ethernet page callbacks (page → controller)
 * ═══════════════════════════════════════════════════════════════════════════ */

static void on_ethernet_disconnect(const char *device_path, gpointer data)
{
    DcAppController *app = data;
    if (!app->network_service || !device_path) return;
    dc_network_service_disconnect(app->network_service, device_path);

    /* Refresh after short delay */
    g_timeout_add_seconds(2, (GSourceFunc)refresh_ethernet, app);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * NetworkManager service signal handlers (service → controller)
 * ═══════════════════════════════════════════════════════════════════════════ */

static void on_nm_wifi_enabled_changed(DcNetworkService *service, gpointer user_data)
{
    (void)service;
    DcAppController *app = user_data;
    if (!app->wifi_page || !app->network_service) return;

    gboolean enabled = dc_network_service_get_wifi_enabled(app->network_service);
    dc_wifi_page_set_wifi_enabled(app->wifi_page, enabled);

    if (enabled)
        refresh_ap_list(app);
    else
        dc_wifi_page_populate_aps(app->wifi_page, NULL);
}

static void on_nm_device_state_changed(DcNetworkService *service, gpointer user_data)
{
    (void)service;
    DcAppController *app = user_data;
    /* Refresh both pages — a device state change affects both */
    refresh_ap_list(app);
    refresh_ethernet(app);
}

static void on_nm_ap_list_changed(DcNetworkService *service, gpointer user_data)
{
    (void)service;
    DcAppController *app = user_data;
    refresh_ap_list(app);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Public: load  (called once after activate())
 * ═══════════════════════════════════════════════════════════════════════════ */

void dc_app_wifi_load(DcAppController *app)
{
    if (!app->wifi_page || !app->network_service) return;

    /* Register page → controller callbacks */
    DcWifiPageCallbacks cb = {
        .on_toggle_wifi = on_wifi_toggle,
        .on_scan        = on_wifi_scan,
        .on_connect     = on_wifi_connect,
        .on_disconnect  = on_wifi_disconnect,
        .on_forget      = on_wifi_forget,
    };
    dc_wifi_page_set_callbacks(app->wifi_page, &cb, app);

    /* Set initial Wi-Fi switch state */
    gboolean enabled = dc_network_service_get_wifi_enabled(app->network_service);
    dc_wifi_page_set_wifi_enabled(app->wifi_page, enabled);

    /* Populate AP list if Wi-Fi is already on */
    if (enabled)
        refresh_ap_list(app);
}

void dc_app_ethernet_load(DcAppController *app)
{
    if (!app->ethernet_page || !app->network_service) return;

    DcEthernetPageCallbacks cb = {
        .on_disconnect = on_ethernet_disconnect,
    };
    dc_ethernet_page_set_callbacks(app->ethernet_page, &cb, app);

    refresh_ethernet(app);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Public: connect_signals  (called once after load)
 * ═══════════════════════════════════════════════════════════════════════════ */

void dc_app_wifi_connect_signals(DcAppController *app)
{
    if (!app->network_service) return;

    g_signal_connect(app->network_service, "wifi-enabled-changed",
                     G_CALLBACK(on_nm_wifi_enabled_changed), app);
    g_signal_connect(app->network_service, "device-state-changed",
                     G_CALLBACK(on_nm_device_state_changed), app);
    g_signal_connect(app->network_service, "ap-list-changed",
                     G_CALLBACK(on_nm_ap_list_changed), app);
}

void dc_app_ethernet_connect_signals(DcAppController *app)
{
    if (!app->network_service) return;

    /* Ethernet updates come through the same device-state-changed signal
     * that wifi already subscribed to — no extra subscription needed.
     * This function is kept for symmetry and future extension. */
    (void)app;
}
