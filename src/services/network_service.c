#include "services/network_service.h"

#include <string.h>

/* ═══════════════════════════════════════════════════════════════════════════
 * GObject boilerplate
 * ═══════════════════════════════════════════════════════════════════════════ */

struct _DcNetworkService {
    GObject          parent_instance;
    GDBusConnection *system_bus;
    /* Subscription IDs so we can unsubscribe on dispose */
    guint            nm_props_sub;
    guint            device_added_sub;
    guint            device_removed_sub;
    /* Re-entrancy guard: sync D-Bus calls inside a query dispatch
     * signals that would re-enter the same query — block them. */
    gboolean         in_query;
};

G_DEFINE_TYPE(DcNetworkService, dc_network_service, G_TYPE_OBJECT)

enum {
    SIGNAL_WIFI_ENABLED_CHANGED,  /* wifi enabled/disabled */
    SIGNAL_DEVICE_STATE_CHANGED,  /* any device state update */
    SIGNAL_AP_LIST_CHANGED,       /* access-point list refreshed */
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

/* ═══════════════════════════════════════════════════════════════════════════
 * Internal D-Bus helpers
 * ═══════════════════════════════════════════════════════════════════════════ */

#define NM_BUS          "org.freedesktop.NetworkManager"
#define NM_PATH         "/org/freedesktop/NetworkManager"
#define NM_IFACE        "org.freedesktop.NetworkManager"
#define NM_DEVICE_IFACE "org.freedesktop.NetworkManager.Device"
#define NM_WIFI_IFACE   "org.freedesktop.NetworkManager.Device.Wireless"
#define NM_WIRED_IFACE  "org.freedesktop.NetworkManager.Device.Wired"
#define NM_AP_IFACE     "org.freedesktop.NetworkManager.AccessPoint"
#define NM_DBUS_PROPS   "org.freedesktop.DBus.Properties"

/* Get a single property from any D-Bus object/interface */
static GVariant *nm_get_property(DcNetworkService *self,
                                  const char       *path,
                                  const char       *iface,
                                  const char       *prop)
{
    if (!self->system_bus || !path || !iface || !prop)
        return NULL;

    GError   *error = NULL;
    GVariant *res = g_dbus_connection_call_sync(
        self->system_bus,
        NM_BUS, path, NM_DBUS_PROPS, "Get",
        g_variant_new("(ss)", iface, prop),
        G_VARIANT_TYPE("(v)"),
        G_DBUS_CALL_FLAGS_NONE, 3000, NULL, &error);

    if (error) {
        g_error_free(error);
        return NULL;
    }
    if (!res) return NULL;

    GVariant *val = NULL;
    g_variant_get(res, "(v)", &val);
    g_variant_unref(res);
    return val;   /* caller must g_variant_unref() */
}

/* Set a writable property on NM */
static void nm_set_property(DcNetworkService *self,
                             const char       *path,
                             const char       *iface,
                             const char       *prop,
                             GVariant         *value)
{
    if (!self->system_bus || !path || !value) {
        if (value) g_variant_unref(value);
        return;
    }
    g_dbus_connection_call(
        self->system_bus,
        NM_BUS, path, NM_DBUS_PROPS, "Set",
        g_variant_new("(ssv)", iface, prop, value),
        NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL, NULL);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Signal handlers — live updates from NetworkManager
 * ═══════════════════════════════════════════════════════════════════════════ */

static void on_nm_properties_changed(GDBusConnection *conn,
                                     const gchar     *sender,
                                     const gchar     *object_path,
                                     const gchar     *iface_name,
                                     const gchar     *signal_name,
                                     GVariant        *parameters,
                                     gpointer         user_data)
{
    (void)conn; (void)sender; (void)object_path;
    (void)iface_name; (void)signal_name;

    DcNetworkService *self = DC_NETWORK_SERVICE(user_data);

    /* While we are executing a synchronous D-Bus query, GLib dispatches
     * pending messages (including signals) in its internal iteration loop.
     * Emitting our own signals here would re-enter get_access_points and
     * corrupt the in-progress GVariant iteration.  Drop the notification;
     * the caller will refresh immediately after the query anyway. */
    if (self->in_query) return;

    /* PropertiesChanged: (sa{sv}as) — read safely via child-value API */
    if (g_variant_n_children(parameters) < 2) return;

    GVariant *iface_v = g_variant_get_child_value(parameters, 0);
    GVariant *props_v = g_variant_get_child_value(parameters, 1);
    const gchar *changed_iface = g_variant_get_string(iface_v, NULL);

    if (g_strcmp0(changed_iface, NM_IFACE) == 0) {
        GVariant *we = g_variant_lookup_value(props_v, "WirelessEnabled",
                                              G_VARIANT_TYPE_BOOLEAN);
        if (we) {
            g_signal_emit(self, signals[SIGNAL_WIFI_ENABLED_CHANGED], 0);
            g_variant_unref(we);
        }
    } else if (g_strcmp0(changed_iface, NM_DEVICE_IFACE) == 0) {
        g_signal_emit(self, signals[SIGNAL_DEVICE_STATE_CHANGED], 0);
    } else if (g_strcmp0(changed_iface, NM_WIFI_IFACE) == 0) {
        g_signal_emit(self, signals[SIGNAL_AP_LIST_CHANGED], 0);
    }

    g_variant_unref(iface_v);
    g_variant_unref(props_v);
}

static void on_device_added(GDBusConnection *conn,
                             const gchar     *sender,
                             const gchar     *object_path,
                             const gchar     *iface_name,
                             const gchar     *signal_name,
                             GVariant        *parameters,
                             gpointer         user_data)
{
    (void)conn; (void)sender; (void)object_path;
    (void)iface_name; (void)signal_name; (void)parameters;
    DcNetworkService *self = DC_NETWORK_SERVICE(user_data);
    if (self->in_query) return;
    g_signal_emit(self, signals[SIGNAL_DEVICE_STATE_CHANGED], 0);
}

static void on_device_removed(GDBusConnection *conn,
                               const gchar     *sender,
                               const gchar     *object_path,
                               const gchar     *iface_name,
                               const gchar     *signal_name,
                               GVariant        *parameters,
                               gpointer         user_data)
{
    (void)conn; (void)sender; (void)object_path;
    (void)iface_name; (void)signal_name; (void)parameters;
    DcNetworkService *self = DC_NETWORK_SERVICE(user_data);
    if (self->in_query) return;
    g_signal_emit(self, signals[SIGNAL_DEVICE_STATE_CHANGED], 0);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * GObject lifecycle
 * ═══════════════════════════════════════════════════════════════════════════ */

static void dc_network_service_init(DcNetworkService *self)
{
    GError *error = NULL;
    self->system_bus = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
    if (error) {
        g_warning("DcNetworkService: could not connect to system bus: %s",
                  error->message);
        g_error_free(error);
        return;
    }

    /* Subscribe to PropertiesChanged on every NM-related path/iface */
    self->nm_props_sub = g_dbus_connection_signal_subscribe(
        self->system_bus,
        NM_BUS,
        "org.freedesktop.DBus.Properties",
        "PropertiesChanged",
        NULL, NULL,
        G_DBUS_SIGNAL_FLAGS_NONE,
        on_nm_properties_changed, self, NULL);

    /* Subscribe to DeviceAdded / DeviceRemoved on the manager object */
    self->device_added_sub = g_dbus_connection_signal_subscribe(
        self->system_bus,
        NM_BUS,
        NM_IFACE,
        "DeviceAdded",
        NM_PATH, NULL,
        G_DBUS_SIGNAL_FLAGS_NONE,
        on_device_added, self, NULL);

    self->device_removed_sub = g_dbus_connection_signal_subscribe(
        self->system_bus,
        NM_BUS,
        NM_IFACE,
        "DeviceRemoved",
        NM_PATH, NULL,
        G_DBUS_SIGNAL_FLAGS_NONE,
        on_device_removed, self, NULL);
}

static void dc_network_service_dispose(GObject *object)
{
    DcNetworkService *self = DC_NETWORK_SERVICE(object);

    if (self->system_bus) {
        if (self->nm_props_sub)
            g_dbus_connection_signal_unsubscribe(self->system_bus,
                                                 self->nm_props_sub);
        if (self->device_added_sub)
            g_dbus_connection_signal_unsubscribe(self->system_bus,
                                                 self->device_added_sub);
        if (self->device_removed_sub)
            g_dbus_connection_signal_unsubscribe(self->system_bus,
                                                 self->device_removed_sub);
        g_object_unref(self->system_bus);
        self->system_bus = NULL;
    }
    G_OBJECT_CLASS(dc_network_service_parent_class)->dispose(object);
}

static void dc_network_service_class_init(DcNetworkServiceClass *klass)
{
    GObjectClass *oc = G_OBJECT_CLASS(klass);
    oc->dispose = dc_network_service_dispose;

    signals[SIGNAL_WIFI_ENABLED_CHANGED] =
        g_signal_new("wifi-enabled-changed",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL,
                     G_TYPE_NONE, 0);

    signals[SIGNAL_DEVICE_STATE_CHANGED] =
        g_signal_new("device-state-changed",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL,
                     G_TYPE_NONE, 0);

    signals[SIGNAL_AP_LIST_CHANGED] =
        g_signal_new("ap-list-changed",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL,
                     G_TYPE_NONE, 0);
}

DcNetworkService *dc_network_service_new(void)
{
    return g_object_new(DC_TYPE_NETWORK_SERVICE, NULL);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Public API helpers
 * ═══════════════════════════════════════════════════════════════════════════ */

void dc_network_device_free(DcNetworkDevice *device)
{
    if (!device) return;
    g_free(device->object_path);
    g_free(device->interface);
    g_free(device->hw_address);
    g_free(device->active_ap_path);
    g_free(device);
}

void dc_access_point_free(DcAccessPoint *ap)
{
    if (!ap) return;
    g_free(ap->object_path);
    g_free(ap->ssid);
    g_free(ap);
}

void dc_network_ip_info_free(DcNetworkIpInfo *info)
{
    if (!info) return;
    g_free(info->address);
    g_free(info->gateway);
    g_free(info);
}

/* ── Retrieve all Devices paths from NM ────────────────────────────────── */
static GVariant *nm_get_devices_variant(DcNetworkService *self)
{
    GVariant *v = nm_get_property(self, NM_PATH, NM_IFACE, "Devices");
    return v;   /* ao variant */
}

/* ── Build SSID → connection-path map from NM saved profiles ────────────
 * Returns GHashTable<char* ssid, char* connection_path>, both owned by table.
 * Must be freed with g_hash_table_destroy().                               */
#define NM_SETTINGS_PATH  "/org/freedesktop/NetworkManager/Settings"
#define NM_SETTINGS_IFACE "org.freedesktop.NetworkManager.Settings"
#define NM_CONN_IFACE     "org.freedesktop.NetworkManager.Settings.Connection"

static GHashTable *nm_build_saved_ssid_map(DcNetworkService *self)
{
    GHashTable *map = g_hash_table_new_full(g_str_hash, g_str_equal,
                                            g_free, g_free);
    if (!self->system_bus) return map;

    /* ListConnections → ao */
    GError   *error = NULL;
    GVariant *res = g_dbus_connection_call_sync(
        self->system_bus,
        NM_BUS, NM_SETTINGS_PATH, NM_SETTINGS_IFACE, "ListConnections",
        NULL, G_VARIANT_TYPE("(ao)"),
        G_DBUS_CALL_FLAGS_NONE, 3000, NULL, &error);

    if (error) { g_error_free(error); return map; }
    if (!res)  { return map; }

    GVariant *ao = g_variant_get_child_value(res, 0);
    g_variant_unref(res);

    gsize n = g_variant_n_children(ao);
    for (gsize i = 0; i < n; i++) {
        GVariant   *conn_path_v = g_variant_get_child_value(ao, i);
        const char *conn_path   = g_variant_get_string(conn_path_v, NULL);

        GError   *s_err  = NULL;
        GVariant *s_res  = g_dbus_connection_call_sync(
            self->system_bus,
            NM_BUS, conn_path, NM_CONN_IFACE, "GetSettings",
            NULL, G_VARIANT_TYPE("(a{sa{sv}})"),
            G_DBUS_CALL_FLAGS_NONE, 3000, NULL, &s_err);

        if (s_err) { g_error_free(s_err); g_variant_unref(conn_path_v); continue; }
        if (!s_res) { g_variant_unref(conn_path_v); continue; }

        GVariant *settings = g_variant_get_child_value(s_res, 0);
        g_variant_unref(s_res);

        /* Look for "802-11-wireless" → "ssid" */
        GVariant *wifi_section = g_variant_lookup_value(settings,
                                                        "802-11-wireless",
                                                        G_VARIANT_TYPE("a{sv}"));
        if (wifi_section) {
            GVariant *ssid_v = g_variant_lookup_value(wifi_section, "ssid",
                                                      G_VARIANT_TYPE("ay"));
            if (ssid_v) {
                gsize        ssid_len   = 0;
                const guchar *ssid_bytes = g_variant_get_fixed_array(ssid_v,
                                                                      &ssid_len, 1);
                if (ssid_bytes && ssid_len > 0) {
                    char *ssid_str = g_strndup((const char *)ssid_bytes, ssid_len);
                    /* Keep only the first match per SSID */
                    if (!g_hash_table_contains(map, ssid_str))
                        g_hash_table_insert(map, ssid_str, g_strdup(conn_path));
                    else
                        g_free(ssid_str);
                }
                g_variant_unref(ssid_v);
            }
            g_variant_unref(wifi_section);
        }

        g_variant_unref(settings);
        g_variant_unref(conn_path_v);
    }

    g_variant_unref(ao);
    return map;
}


/* Build a DcNetworkDevice from a device object path */
static DcNetworkDevice *build_device(DcNetworkService *self, const char *path)
{
    GVariant *type_v   = nm_get_property(self, path, NM_DEVICE_IFACE, "DeviceType");
    GVariant *iface_v  = nm_get_property(self, path, NM_DEVICE_IFACE, "Interface");
    GVariant *mac_v    = nm_get_property(self, path, NM_DEVICE_IFACE, "HwAddress");
    GVariant *state_v  = nm_get_property(self, path, NM_DEVICE_IFACE, "State");

    if (!type_v) return NULL;   /* could not query — skip */

    DcNetworkDevice *dev = g_new0(DcNetworkDevice, 1);
    dev->object_path = g_strdup(path);
    dev->type        = (DcNetworkDeviceType)g_variant_get_uint32(type_v);
    dev->interface   = iface_v  ? g_strdup(g_variant_get_string(iface_v, NULL))  : g_strdup("");
    dev->hw_address  = mac_v    ? g_strdup(g_variant_get_string(mac_v,   NULL))  : g_strdup("");
    dev->state       = state_v  ? g_variant_get_uint32(state_v)                  : 0;

    if (type_v)  g_variant_unref(type_v);
    if (iface_v) g_variant_unref(iface_v);
    if (mac_v)   g_variant_unref(mac_v);
    if (state_v) g_variant_unref(state_v);

    if (dev->type == DC_DEVICE_TYPE_WIFI) {
        GVariant *ap_v = nm_get_property(self, path, NM_WIFI_IFACE, "ActiveAccessPoint");
        dev->active_ap_path = ap_v ? g_strdup(g_variant_get_string(ap_v, NULL)) : g_strdup("/");
        if (ap_v) g_variant_unref(ap_v);
    } else if (dev->type == DC_DEVICE_TYPE_ETHERNET) {
        GVariant *spd_v     = nm_get_property(self, path, NM_WIRED_IFACE, "Speed");
        GVariant *carrier_v = nm_get_property(self, path, NM_WIRED_IFACE, "Carrier");
        dev->speed   = spd_v     ? g_variant_get_uint32(spd_v)    : 0;
        dev->carrier = carrier_v ? g_variant_get_boolean(carrier_v) : FALSE;
        if (spd_v)     g_variant_unref(spd_v);
        if (carrier_v) g_variant_unref(carrier_v);
    }

    return dev;
}

/* ─────────────────────────────────────────────────────────────────────────── */

gboolean dc_network_service_get_wifi_enabled(DcNetworkService *self)
{
    GVariant *v = nm_get_property(self, NM_PATH, NM_IFACE, "WirelessEnabled");
    if (!v) return FALSE;
    gboolean enabled = g_variant_get_boolean(v);
    g_variant_unref(v);
    return enabled;
}

void dc_network_service_set_wifi_enabled(DcNetworkService *self, gboolean enabled)
{
    nm_set_property(self, NM_PATH, NM_IFACE, "WirelessEnabled",
                    g_variant_new_boolean(enabled));
}

char *dc_network_service_get_wifi_device_path(DcNetworkService *self)
{
    if (!self->system_bus) return NULL;

    self->in_query = TRUE;
    GVariant *devs = nm_get_property(self, NM_PATH, NM_IFACE, "Devices");
    char *wifi_path = NULL;

    if (devs) {
        /* Iterate by index — avoids g_variant_iter_loop + break unsafety */
        gsize n = g_variant_n_children(devs);
        for (gsize i = 0; i < n && !wifi_path; i++) {
            GVariant *path_v = g_variant_get_child_value(devs, i);
            const char *path = g_variant_get_string(path_v, NULL);
            GVariant *type_v = nm_get_property(self, path, NM_DEVICE_IFACE, "DeviceType");
            if (type_v) {
                if (g_variant_get_uint32(type_v) == DC_DEVICE_TYPE_WIFI)
                    wifi_path = g_strdup(path);
                g_variant_unref(type_v);
            }
            g_variant_unref(path_v);
        }
        g_variant_unref(devs);
    }
    self->in_query = FALSE;
    return wifi_path;
}

GList *dc_network_service_get_access_points(DcNetworkService *self,
                                             const char       *wifi_device_path)
{
    if (!self->system_bus || !wifi_device_path) return NULL;

    self->in_query = TRUE;

    /* 1. Get the list of AP object paths */
    GError   *error = NULL;
    GVariant *res = g_dbus_connection_call_sync(
        self->system_bus,
        NM_BUS, wifi_device_path, NM_WIFI_IFACE, "GetAllAccessPoints",
        NULL, G_VARIANT_TYPE("(ao)"),
        G_DBUS_CALL_FLAGS_NONE, 5000, NULL, &error);

    if (error) {
        g_warning("GetAllAccessPoints: %s", error->message);
        g_error_free(error);
        self->in_query = FALSE;
        return NULL;
    }
    if (!res) { self->in_query = FALSE; return NULL; }

    /* Extract ao child safely from the (ao) tuple */
    GVariant *ao = g_variant_get_child_value(res, 0);
    g_variant_unref(res);

    /* 2. Which AP is currently active? */
    char *active_ap = NULL;
    GVariant *aap_v = nm_get_property(self, wifi_device_path,
                                      NM_WIFI_IFACE, "ActiveAccessPoint");
    if (aap_v) {
        active_ap = g_strdup(g_variant_get_string(aap_v, NULL));
        g_variant_unref(aap_v);
    }

    /* 3. Build saved-SSID map so we can skip password prompts */
    GHashTable *saved_map = nm_build_saved_ssid_map(self);

    /* 4. For each AP path, fetch ALL properties in ONE GetAll call */
    GList *aps  = NULL;
    gsize  n    = g_variant_n_children(ao);

    for (gsize i = 0; i < n; i++) {
        GVariant   *path_v  = g_variant_get_child_value(ao, i);
        const char *ap_path = g_variant_get_string(path_v, NULL);

        GError   *ap_err    = NULL;
        GVariant *all_res   = g_dbus_connection_call_sync(
            self->system_bus,
            NM_BUS, ap_path, NM_DBUS_PROPS, "GetAll",
            g_variant_new("(s)", NM_AP_IFACE),
            G_VARIANT_TYPE("(a{sv})"),
            G_DBUS_CALL_FLAGS_NONE, 3000, NULL, &ap_err);

        if (ap_err) { g_error_free(ap_err); g_variant_unref(path_v); continue; }
        if (!all_res) { g_variant_unref(path_v); continue; }

        /* all_res = (a{sv}) — extract the dict */
        GVariant *props = g_variant_get_child_value(all_res, 0);
        g_variant_unref(all_res);

        DcAccessPoint *ap = g_new0(DcAccessPoint, 1);
        ap->object_path   = g_strdup(ap_path);

        /* SSID — byte array (ay); use bytestring API which is D-Bus-safe */
        GVariant *ssid_v = g_variant_lookup_value(props, "Ssid",
                                                  G_VARIANT_TYPE("ay"));
        if (ssid_v) {
            gsize ssid_len = 0;
            const guchar *ssid_bytes = g_variant_get_fixed_array(ssid_v,
                                                                  &ssid_len, 1);
            if (ssid_bytes && ssid_len > 0)
                ap->ssid = g_strndup((const char *)ssid_bytes, ssid_len);
            else
                ap->ssid = g_strdup("Hidden Network");
            g_variant_unref(ssid_v);
        } else {
            ap->ssid = g_strdup("Hidden Network");
        }

        /* Strength */
        GVariant *str_v = g_variant_lookup_value(props, "Strength",
                                                 G_VARIANT_TYPE("y"));
        ap->strength = str_v ? g_variant_get_byte(str_v) : 0;
        if (str_v) g_variant_unref(str_v);

        /* Security: Privacy flag + WpaFlags + RsnFlags */
        GVariant *flags_v = g_variant_lookup_value(props, "Flags",
                                                   G_VARIANT_TYPE("u"));
        GVariant *wpa_v   = g_variant_lookup_value(props, "WpaFlags",
                                                   G_VARIANT_TYPE("u"));
        GVariant *rsn_v   = g_variant_lookup_value(props, "RsnFlags",
                                                   G_VARIANT_TYPE("u"));
        guint32 flags     = flags_v ? g_variant_get_uint32(flags_v) : 0;
        guint32 wpa_flags = wpa_v   ? g_variant_get_uint32(wpa_v)   : 0;
        guint32 rsn_flags = rsn_v   ? g_variant_get_uint32(rsn_v)   : 0;
        ap->secured = (flags & 0x1) || (wpa_flags != 0) || (rsn_flags != 0);
        if (flags_v) g_variant_unref(flags_v);
        if (wpa_v)   g_variant_unref(wpa_v);
        if (rsn_v)   g_variant_unref(rsn_v);

        ap->active = (active_ap && g_strcmp0(active_ap, ap_path) == 0);
        ap->has_profile = (ap->ssid &&
                           g_hash_table_contains(saved_map, ap->ssid));

        g_variant_unref(props);
        g_variant_unref(path_v);
        aps = g_list_append(aps, ap);
    }

    g_hash_table_destroy(saved_map);
    g_variant_unref(ao);
    g_free(active_ap);
    self->in_query = FALSE;
    return aps;
}

void dc_network_service_request_scan(DcNetworkService *self,
                                      const char       *wifi_device_path)
{
    if (!self->system_bus || !wifi_device_path) return;

    /* Build an empty a{sv} options dict using a builder — avoids
     * the floating-reference leak from g_variant_new_array + g_variant_new_tuple */
    GVariantBuilder opts;
    g_variant_builder_init(&opts, G_VARIANT_TYPE("a{sv}"));
    g_dbus_connection_call(
        self->system_bus,
        NM_BUS, wifi_device_path, NM_WIFI_IFACE, "RequestScan",
        g_variant_new("(a{sv})", &opts),
        NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL, NULL);
}

void dc_network_service_connect_to_ap(DcNetworkService *self,
                                       const char       *device_path,
                                       const char       *ap_path,
                                       const char       *ssid,
                                       const char       *password)
{
    if (!self->system_bus || !device_path || !ap_path || !ssid) return;

    /* No password: reuse a saved profile if one exists, else let NM auto-pick */
    if (!password) {
        GHashTable *saved      = nm_build_saved_ssid_map(self);
        const char *saved_conn = g_hash_table_lookup(saved, ssid);
        const char *use_conn   = saved_conn ? saved_conn : "/";

        g_dbus_connection_call(
            self->system_bus,
            NM_BUS, NM_PATH, NM_IFACE, "ActivateConnection",
            g_variant_new("(ooo)", use_conn, device_path, ap_path),
            G_VARIANT_TYPE("(o)"),
            G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL, NULL);

        g_hash_table_destroy(saved);
        return;
    }

    /* Password provided: create a new profile and activate it */
    GVariantBuilder conn_builder;
    g_variant_builder_init(&conn_builder, G_VARIANT_TYPE("a{sa{sv}}"));

    GVariantBuilder wifi_builder;
    g_variant_builder_init(&wifi_builder, G_VARIANT_TYPE("a{sv}"));
    GBytes *ssid_bytes = g_bytes_new(ssid, strlen(ssid));
    g_variant_builder_add(&wifi_builder, "{sv}", "ssid",
                          g_variant_new_from_bytes(G_VARIANT_TYPE("ay"),
                                                   ssid_bytes, TRUE));
    g_bytes_unref(ssid_bytes);
    g_variant_builder_add(&conn_builder, "{sa{sv}}", "802-11-wireless",
                          &wifi_builder);

    GVariantBuilder sec_builder;
    g_variant_builder_init(&sec_builder, G_VARIANT_TYPE("a{sv}"));
    g_variant_builder_add(&sec_builder, "{sv}", "key-mgmt",
                          g_variant_new_string("wpa-psk"));
    g_variant_builder_add(&sec_builder, "{sv}", "psk",
                          g_variant_new_string(password));
    g_variant_builder_add(&conn_builder, "{sa{sv}}",
                          "802-11-wireless-security", &sec_builder);

    g_dbus_connection_call(
        self->system_bus,
        NM_BUS, NM_PATH, NM_IFACE, "AddAndActivateConnection",
        g_variant_new("(a{sa{sv}}oo)", &conn_builder, device_path, ap_path),
        G_VARIANT_TYPE("(oo)"),
        G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL, NULL);
}

void dc_network_service_forget_ap(DcNetworkService *self, const char *ssid)
{
    if (!self->system_bus || !ssid) return;

    /* Look up the existing profile path for this SSID */
    GHashTable *saved = nm_build_saved_ssid_map(self);
    const char *saved_conn = g_hash_table_lookup(saved, ssid);

    if (saved_conn) {
        g_dbus_connection_call(
            self->system_bus,
            NM_BUS, saved_conn, NM_CONN_IFACE, "Delete",
            NULL, NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL, NULL);
    }
    
    g_hash_table_destroy(saved);
}

GList *dc_network_service_get_ethernet_devices(DcNetworkService *self)
{
    if (!self->system_bus) return NULL;

    self->in_query = TRUE;
    GVariant *devs = nm_get_property(self, NM_PATH, NM_IFACE, "Devices");
    GList    *list = NULL;

    if (devs) {
        gsize n = g_variant_n_children(devs);
        for (gsize i = 0; i < n; i++) {
            GVariant   *path_v = g_variant_get_child_value(devs, i);
            const char *path   = g_variant_get_string(path_v, NULL);
            GVariant   *type_v = nm_get_property(self, path, NM_DEVICE_IFACE, "DeviceType");
            if (type_v) {
                if (g_variant_get_uint32(type_v) == DC_DEVICE_TYPE_ETHERNET) {
                    DcNetworkDevice *dev = build_device(self, path);
                    if (dev) list = g_list_append(list, dev);
                }
                g_variant_unref(type_v);
            }
            g_variant_unref(path_v);
        }
        g_variant_unref(devs);
    }
    self->in_query = FALSE;
    return list;
}

void dc_network_service_disconnect(DcNetworkService *self,
                                    const char       *device_path)
{
    if (!self->system_bus || !device_path) return;
    g_dbus_connection_call(
        self->system_bus,
        NM_BUS, device_path, NM_DEVICE_IFACE, "Disconnect",
        NULL, NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL, NULL);
}

DcNetworkIpInfo *dc_network_service_get_ip_info(DcNetworkService *self,
                                                  const char       *device_path)
{
    if (!self->system_bus || !device_path) return NULL;

    /* Get Ip4Config object path from the device */
    GVariant *ip4_v = nm_get_property(self, device_path, NM_DEVICE_IFACE, "Ip4Config");
    if (!ip4_v) return NULL;

    const char *ip4_path = g_variant_get_string(ip4_v, NULL);
    if (!ip4_path || g_strcmp0(ip4_path, "/") == 0) {
        g_variant_unref(ip4_v);
        return NULL;
    }

    /* Ip4Config.AddressData → aa{sv} */
    GVariant *addr_data = nm_get_property(self, ip4_path,
                                           "org.freedesktop.NetworkManager.IP4Config",
                                           "AddressData");
    GVariant *gw_v      = nm_get_property(self, ip4_path,
                                           "org.freedesktop.NetworkManager.IP4Config",
                                           "Gateway");
    g_variant_unref(ip4_v);

    DcNetworkIpInfo *info = g_new0(DcNetworkIpInfo, 1);

    if (gw_v) {
        info->gateway = g_strdup(g_variant_get_string(gw_v, NULL));
        g_variant_unref(gw_v);
    } else {
        info->gateway = g_strdup("");
    }

    if (addr_data) {
        GVariantIter addr_iter;
        GVariant    *entry = NULL;
        g_variant_iter_init(&addr_iter, addr_data);
        if ((entry = g_variant_iter_next_value(&addr_iter)) != NULL) {
            GVariant *addr_v   = g_variant_lookup_value(entry, "address",  G_VARIANT_TYPE_STRING);
            GVariant *prefix_v = g_variant_lookup_value(entry, "prefix",   G_VARIANT_TYPE_UINT32);
            if (addr_v) {
                info->address = g_strdup(g_variant_get_string(addr_v, NULL));
                g_variant_unref(addr_v);
            }
            if (prefix_v) {
                info->prefix = g_variant_get_uint32(prefix_v);
                g_variant_unref(prefix_v);
            }
            g_variant_unref(entry);
        }
        g_variant_unref(addr_data);
    }

    if (!info->address) info->address = g_strdup("—");
    if (!info->gateway) info->gateway = g_strdup("—");

    return info;
}
