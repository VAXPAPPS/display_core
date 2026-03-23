#include "services/power_service.h"

struct _DcPowerService {
    GObject parent_instance;
    GDBusConnection *connection;
    guint dbus_subscription_id;
};

G_DEFINE_TYPE(DcPowerService, dc_power_service, G_TYPE_OBJECT)

enum {
    SIGNAL_BRIGHTNESS_CHANGED,
    SIGNAL_KEYBOARD_BRIGHTNESS_CHANGED,
    SIGNAL_BATTERY_CHANGED,
    SIGNAL_PROFILE_CHANGED,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

static void on_dbus_signal(GDBusConnection *connection, const gchar *sender_name,
                           const gchar *object_path, const gchar *interface_name,
                           const gchar *signal_name, GVariant *parameters, gpointer user_data) {
    DcPowerService *self = DC_POWER_SERVICE(user_data);

    if (g_strcmp0(signal_name, "BrightnessChanged") == 0) {
        gint level;
        g_variant_get(parameters, "(i)", &level);
        g_signal_emit(self, signals[SIGNAL_BRIGHTNESS_CHANGED], 0, level);
    } 
    else if (g_strcmp0(signal_name, "KeyboardBrightnessChanged") == 0) {
        gint level;
        g_variant_get(parameters, "(i)", &level);
        g_signal_emit(self, signals[SIGNAL_KEYBOARD_BRIGHTNESS_CHANGED], 0, level);
    } 
    else if (g_strcmp0(signal_name, "BatteryChanged") == 0) {
        gdouble percentage;
        gboolean charging;
        g_variant_get(parameters, "(db)", &percentage, &charging);
        g_signal_emit(self, signals[SIGNAL_BATTERY_CHANGED], 0, percentage, charging);
    } 
    else if (g_strcmp0(signal_name, "ProfileChanged") == 0) {
        gchar *profile;
        g_variant_get(parameters, "(s)", &profile);
        g_signal_emit(self, signals[SIGNAL_PROFILE_CHANGED], 0, profile);
        g_free(profile);
    }
}

static void dc_power_service_class_init(DcPowerServiceClass *klass) {
    signals[SIGNAL_BRIGHTNESS_CHANGED] = g_signal_new(
        "brightness-changed", G_TYPE_FROM_CLASS(klass), G_SIGNAL_RUN_LAST,
        0, NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_INT);

    signals[SIGNAL_KEYBOARD_BRIGHTNESS_CHANGED] = g_signal_new(
        "keyboard-brightness-changed", G_TYPE_FROM_CLASS(klass), G_SIGNAL_RUN_LAST,
        0, NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_INT);

    signals[SIGNAL_BATTERY_CHANGED] = g_signal_new(
        "battery-changed", G_TYPE_FROM_CLASS(klass), G_SIGNAL_RUN_LAST,
        0, NULL, NULL, NULL, G_TYPE_NONE, 2, G_TYPE_DOUBLE, G_TYPE_BOOLEAN);

    signals[SIGNAL_PROFILE_CHANGED] = g_signal_new(
        "profile-changed", G_TYPE_FROM_CLASS(klass), G_SIGNAL_RUN_LAST,
        0, NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_STRING);
}

static void dc_power_service_init(DcPowerService *self) {
    GError *error = NULL;
    self->connection = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);
    if (!self->connection) {
        g_warning("Power service failed to connect to session D-Bus: %s", error->message);
        g_error_free(error);
        return;
    }

    self->dbus_subscription_id = g_dbus_connection_signal_subscribe(
        self->connection,
        "org.venom.Power",
        "org.venom.Power",
        NULL,
        "/org/venom/Power",
        NULL,
        G_DBUS_SIGNAL_FLAGS_NONE,
        on_dbus_signal,
        self,
        NULL);
}

DcPowerService *dc_power_service_new(void) {
    return g_object_new(DC_TYPE_POWER_SERVICE, NULL);
}

// -----------------------------------------------------
// Helper
// -----------------------------------------------------
static GVariant* call_sync(DcPowerService *self, const gchar *method, GVariant *params) {
    if (!self->connection) return NULL;
    GError *error = NULL;
    GVariant *result = g_dbus_connection_call_sync(
        self->connection,
        "org.venom.Power",
        "/org/venom/Power",
        "org.venom.Power",
        method,
        params,
        NULL,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        &error
    );
    if (error) {
        g_warning("Power D-Bus error (%s): %s", method, error->message);
        g_error_free(error);
        return NULL;
    }
    return result;
}

// -----------------------------------------------------
// Actions Implementation
// -----------------------------------------------------
int dc_power_service_get_brightness(DcPowerService *self) {
    GVariant *res = call_sync(self, "GetBrightness", NULL);
    if (!res) return 0;
    int val = 0;
    g_variant_get(res, "(i)", &val);
    g_variant_unref(res);
    return val;
}

void dc_power_service_set_brightness(DcPowerService *self, int level) {
    GVariant *res = call_sync(self, "SetBrightness", g_variant_new("(i)", level));
    if (res) g_variant_unref(res);
}

gboolean dc_power_service_is_keyboard_supported(DcPowerService *self) {
    GVariant *res = call_sync(self, "IsKeyboardBacklightSupported", NULL);
    if (!res) return FALSE;
    gboolean supported = FALSE;
    g_variant_get(res, "(b)", &supported);
    g_variant_unref(res);
    return supported;
}

int dc_power_service_get_keyboard_brightness(DcPowerService *self) {
    GVariant *res = call_sync(self, "GetKeyboardBrightness", NULL);
    if (!res) return 0;
    int val = 0;
    g_variant_get(res, "(i)", &val);
    g_variant_unref(res);
    return val;
}

void dc_power_service_set_keyboard_brightness(DcPowerService *self, int level) {
    GVariant *res = call_sync(self, "SetKeyboardBrightness", g_variant_new("(i)", level));
    if (res) g_variant_unref(res);
}

void dc_power_service_get_battery_info(DcPowerService *self, double *percentage, gboolean *charging, gint64 *time_to_empty) {
    GVariant *res = call_sync(self, "GetBatteryInfo", NULL);
    if (!res) {
        if (percentage) *percentage = 0.0;
        if (charging) *charging = FALSE;
        if (time_to_empty) *time_to_empty = 0;
        return;
    }
    gdouble p; gboolean c; gint64 t;
    g_variant_get(res, "(dbx)", &p, &c, &t);
    if (percentage) *percentage = p;
    if (charging) *charging = c;
    if (time_to_empty) *time_to_empty = t;
    g_variant_unref(res);
}

gboolean dc_power_service_get_power_source(DcPowerService *self) {
    GVariant *res = call_sync(self, "GetPowerSource", NULL);
    if (!res) return FALSE;
    gboolean val = FALSE;
    g_variant_get(res, "(b)", &val);
    g_variant_unref(res);
    return val;
}

gboolean dc_power_service_is_profiles_available(DcPowerService *self) {
    GVariant *res = call_sync(self, "IsProfilesAvailable", NULL);
    if (!res) return FALSE;
    gboolean val = FALSE;
    g_variant_get(res, "(b)", &val);
    g_variant_unref(res);
    return val;
}

gchar* dc_power_service_get_active_profile(DcPowerService *self) {
    GVariant *res = call_sync(self, "GetActiveProfile", NULL);
    if (!res) return g_strdup("");
    gchar *profile = NULL;
    g_variant_get(res, "(s)", &profile);
    g_variant_unref(res);
    return profile;
}

void dc_power_service_set_active_profile(DcPowerService *self, const gchar *profile) {
    GVariant *res = call_sync(self, "SetActiveProfile", g_variant_new("(s)", profile));
    if (res) g_variant_unref(res);
}

char** dc_power_service_get_profiles(DcPowerService *self) {
    GVariant *res = call_sync(self, "GetProfiles", NULL);
    if (!res) return NULL;
    
    GVariant *profiles_var = NULL;
    g_variant_get(res, "(@as)", &profiles_var);
    if (!profiles_var) {
        g_variant_unref(res);
        return NULL;
    }

    gsize length = g_variant_n_children(profiles_var);
    char **arr = g_new0(char*, length + 1);
    
    for (gsize i = 0; i < length; i++) {
        GVariant *child = g_variant_get_child_value(profiles_var, i);
        arr[i] = g_strdup(g_variant_get_string(child, NULL));
        g_variant_unref(child);
    }
    
    g_variant_unref(profiles_var);
    g_variant_unref(res);
    return arr;
}

void dc_power_service_get_idle_timeouts(DcPowerService *self, guint *dim, guint *blank, guint *suspend) {
    GVariant *res = call_sync(self, "GetIdleTimeouts", NULL);
    if (!res) {
        if (dim) *dim = 0;
        if (blank) *blank = 0;
        if (suspend) *suspend = 0;
        return;
    }
    guint d, b, s;
    g_variant_get(res, "(uuu)", &d, &b, &s);
    if (dim) *dim = d;
    if (blank) *blank = b;
    if (suspend) *suspend = s;
    g_variant_unref(res);
}

void dc_power_service_set_idle_timeouts(DcPowerService *self, guint dim, guint blank, guint suspend) {
    GVariant *res = call_sync(self, "SetIdleTimeouts", g_variant_new("(uuu)", dim, blank, suspend));
    if (res) g_variant_unref(res);
}

void dc_power_service_get_lid_action(DcPowerService *self, gchar **ac_action, gchar **batt_action) {
    GVariant *res = call_sync(self, "GetLidAction", NULL);
    if (!res) {
        if (ac_action) *ac_action = g_strdup("");
        if (batt_action) *batt_action = g_strdup("");
        return;
    }
    gchar *ac = NULL, *batt = NULL;
    g_variant_get(res, "(ss)", &ac, &batt);
    if (ac_action) *ac_action = ac;
    else g_free(ac);
    if (batt_action) *batt_action = batt;
    else g_free(batt);
    g_variant_unref(res);
}

void dc_power_service_set_lid_action(DcPowerService *self, const gchar *ac_action, const gchar *batt_action) {
    GVariant *res = call_sync(self, "SetLidAction", g_variant_new("(ss)", ac_action, batt_action));
    if (res) g_variant_unref(res);
}

void dc_power_service_get_power_button_action(DcPowerService *self, gchar **action) {
    GVariant *res = call_sync(self, "GetPowerButtonAction", NULL);
    if (!res) {
        if (action) *action = g_strdup("");
        return;
    }
    gchar *act = NULL;
    g_variant_get(res, "(s)", &act);
    if (action) *action = act;
    else g_free(act);
    g_variant_unref(res);
}

void dc_power_service_set_power_button_action(DcPowerService *self, const gchar *action) {
    GVariant *res = call_sync(self, "SetPowerButtonAction", g_variant_new("(s)", action));
    if (res) g_variant_unref(res);
}

void dc_power_service_get_critical_action(DcPowerService *self, gchar **action) {
    GVariant *res = call_sync(self, "GetCriticalAction", NULL);
    if (!res) {
        if (action) *action = g_strdup("");
        return;
    }
    gchar *act = NULL;
    g_variant_get(res, "(s)", &act);
    if (action) *action = act;
    else g_free(act);
    g_variant_unref(res);
}

void dc_power_service_set_critical_action(DcPowerService *self, const gchar *action) {
    GVariant *res = call_sync(self, "SetCriticalAction", g_variant_new("(s)", action));
    if (res) g_variant_unref(res);
}
