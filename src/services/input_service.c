#include "services/input_service.h"
#include <gio/gio.h>

struct _DcInputService {
    GObject parent_instance;
    GDBusConnection *connection;
};

G_DEFINE_TYPE(DcInputService, dc_input_service, G_TYPE_OBJECT)

static void dc_input_service_class_init(DcInputServiceClass *klass) {
    (void)klass;
}

static void dc_input_service_init(DcInputService *self) {
    GError *error = NULL;
    self->connection = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);
    if (!self->connection) {
        g_warning("Input service failed to connect to session D-Bus: %s", error->message);
        g_error_free(error);
    }
}

DcInputService *dc_input_service_new(void) {
    return g_object_new(DC_TYPE_INPUT_SERVICE, NULL);
}

/* ------------------------------------------------------------------ */
/*  D-Bus Helper                                                      */
/* ------------------------------------------------------------------ */
static GVariant* call_sync(DcInputService *self, const gchar *method, GVariant *params) {
    if (!self->connection) return NULL;
    GError *error = NULL;
    GVariant *result = g_dbus_connection_call_sync(
        self->connection,
        "org.venom.Input",
        "/org/venom/Input",
        "org.venom.Input",
        method,
        params,
        NULL,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        &error
    );
    if (error) {
        g_warning("Input D-Bus error (%s): %s", method, error->message);
        g_error_free(error);
        return NULL;
    }
    return result;
}

static gboolean call_sync_bool(DcInputService *self, const gchar *method, GVariant *params) {
    GVariant *res = call_sync(self, method, params);
    if (!res) return FALSE;
    gboolean success = FALSE;
    g_variant_get(res, "(b)", &success);
    g_variant_unref(res);
    return success;
}

/* ------------------------------------------------------------------ */
/*  Keyboard                                                          */
/* ------------------------------------------------------------------ */
gboolean dc_input_service_set_keyboard_layouts(DcInputService *self, const gchar *layouts) {
    return call_sync_bool(self, "SetKeyboardLayouts", g_variant_new("(s)", layouts));
}

gboolean dc_input_service_set_keyboard_model(DcInputService *self, const gchar *model) {
    return call_sync_bool(self, "SetKeyboardModel", g_variant_new("(s)", model));
}

gboolean dc_input_service_set_keyboard_options(DcInputService *self, const gchar *options) {
    return call_sync_bool(self, "SetKeyboardOptions", g_variant_new("(s)", options));
}

void dc_input_service_get_keyboard_settings(DcInputService *self, gchar **layouts, gchar **model, gchar **options) {
    GVariant *res = call_sync(self, "GetKeyboardSettings", NULL);
    if (!res) {
        if (layouts) *layouts = g_strdup("");
        if (model) *model = g_strdup("");
        if (options) *options = g_strdup("");
        return;
    }
    gchar *l = NULL, *m = NULL, *o = NULL;
    g_variant_get(res, "(sss)", &l, &m, &o);
    if (layouts) *layouts = l; else g_free(l);
    if (model) *model = m; else g_free(m);
    if (options) *options = o; else g_free(o);
    g_variant_unref(res);
}

gboolean dc_input_service_add_keyboard_layout(DcInputService *self, const gchar *layout) {
    return call_sync_bool(self, "AddKeyboardLayout", g_variant_new("(s)", layout));
}

gboolean dc_input_service_remove_keyboard_layout(DcInputService *self, const gchar *layout) {
    return call_sync_bool(self, "RemoveKeyboardLayout", g_variant_new("(s)", layout));
}

char **dc_input_service_list_keyboard_layouts(DcInputService *self) {
    GVariant *res = call_sync(self, "ListKeyboardLayouts", NULL);
    if (!res) return NULL;
    
    GVariant *layouts_var = NULL;
    g_variant_get(res, "(@as)", &layouts_var);
    if (!layouts_var) {
        g_variant_unref(res);
        return NULL;
    }

    gsize length = g_variant_n_children(layouts_var);
    char **arr = g_new0(char*, length + 1);
    for (gsize i = 0; i < length; i++) {
        GVariant *child = g_variant_get_child_value(layouts_var, i);
        arr[i] = g_strdup(g_variant_get_string(child, NULL));
        g_variant_unref(child);
    }
    
    g_variant_unref(layouts_var);
    g_variant_unref(res);
    return arr;
}

/* ------------------------------------------------------------------ */
/*  Mouse                                                             */
/* ------------------------------------------------------------------ */
gboolean dc_input_service_set_mouse_accel(DcInputService *self, double accel) {
    return call_sync_bool(self, "SetMouseAccel", g_variant_new("(d)", accel));
}

gboolean dc_input_service_set_mouse_speed(DcInputService *self, double speed) {
    return call_sync_bool(self, "SetMouseSpeed", g_variant_new("(d)", speed));
}

gboolean dc_input_service_set_mouse_natural_scroll(DcInputService *self, gboolean enabled) {
    return call_sync_bool(self, "SetMouseNaturalScroll", g_variant_new("(b)", enabled));
}

gboolean dc_input_service_set_mouse_left_handed(DcInputService *self, gboolean enabled) {
    return call_sync_bool(self, "SetMouseLeftHanded", g_variant_new("(b)", enabled));
}

void dc_input_service_get_mouse_settings(DcInputService *self, double *accel, double *speed, gboolean *natural_scroll, gboolean *left_handed) {
    GVariant *res = call_sync(self, "GetMouseSettings", NULL);
    if (!res) {
        if (accel) *accel = 0.0;
        if (speed) *speed = 1.0;
        if (natural_scroll) *natural_scroll = FALSE;
        if (left_handed) *left_handed = FALSE;
        return;
    }
    gdouble a, s; gboolean ns, lh;
    g_variant_get(res, "(ddbb)", &a, &s, &ns, &lh);
    if (accel) *accel = a;
    if (speed) *speed = s;
    if (natural_scroll) *natural_scroll = ns;
    if (left_handed) *left_handed = lh;
    g_variant_unref(res);
}

/* ------------------------------------------------------------------ */
/*  Touchpad                                                          */
/* ------------------------------------------------------------------ */
gboolean dc_input_service_set_touchpad_enabled(DcInputService *self, gboolean enabled) {
    return call_sync_bool(self, "SetTouchpadEnabled", g_variant_new("(b)", enabled));
}

gboolean dc_input_service_set_touchpad_tap_to_click(DcInputService *self, gboolean enabled) {
    return call_sync_bool(self, "SetTouchpadTapToClick", g_variant_new("(b)", enabled));
}

gboolean dc_input_service_set_touchpad_natural_scroll(DcInputService *self, gboolean enabled) {
    return call_sync_bool(self, "SetTouchpadNaturalScroll", g_variant_new("(b)", enabled));
}

gboolean dc_input_service_set_touchpad_scroll_method(DcInputService *self, const gchar *method) {
    return call_sync_bool(self, "SetTouchpadScrollMethod", g_variant_new("(s)", method));
}

gboolean dc_input_service_set_touchpad_speed(DcInputService *self, double speed) {
    return call_sync_bool(self, "SetTouchpadSpeed", g_variant_new("(d)", speed));
}

gboolean dc_input_service_set_touchpad_disable_while_typing(DcInputService *self, gboolean enabled) {
    return call_sync_bool(self, "SetTouchpadDisableWhileTyping", g_variant_new("(b)", enabled));
}

void dc_input_service_get_touchpad_settings(DcInputService *self, gboolean *enabled, gboolean *tap_to_click, gboolean *natural_scroll, gchar **scroll_method, double *speed, gboolean *disable_while_typing) {
    GVariant *res = call_sync(self, "GetTouchpadSettings", NULL);
    if (!res) {
        if (enabled) *enabled = TRUE;
        if (tap_to_click) *tap_to_click = TRUE;
        if (natural_scroll) *natural_scroll = FALSE;
        if (scroll_method) *scroll_method = g_strdup("two-finger");
        if (speed) *speed = 0.5;
        if (disable_while_typing) *disable_while_typing = TRUE;
        return;
    }
    gboolean e, t, n, d; gchar *m = NULL; gdouble s;
    g_variant_get(res, "(bbbsdb)", &e, &t, &n, &m, &s, &d);
    if (enabled) *enabled = e;
    if (tap_to_click) *tap_to_click = t;
    if (natural_scroll) *natural_scroll = n;
    if (scroll_method) *scroll_method = m; else g_free(m);
    if (speed) *speed = s;
    if (disable_while_typing) *disable_while_typing = d;
    g_variant_unref(res);
}
