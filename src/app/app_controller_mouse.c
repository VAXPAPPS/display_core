#include "app_controller_internal.h"

/* ----------------------------------------------------- */
/*  Mouse Signal Handlers                               */
/* ----------------------------------------------------- */

static void on_mouse_speed_changed(GtkRange *range, gpointer user_data) {
    DcAppController *app = user_data;
    if (app->suppress_input_updates) return;
    double val = gtk_range_get_value(range);
    dc_input_service_set_mouse_speed(app->input_service, val);
}

static void on_mouse_accel_changed(GtkRange *range, gpointer user_data) {
    DcAppController *app = user_data;
    if (app->suppress_input_updates) return;
    double val = gtk_range_get_value(range);
    dc_input_service_set_mouse_accel(app->input_service, val);
}

static void on_mouse_natural_scroll_toggled(GObject *object, GParamSpec *pspec, gpointer user_data) {
    (void)pspec;
    DcAppController *app = user_data;
    if (app->suppress_input_updates) return;
    gboolean enabled = gtk_switch_get_active(GTK_SWITCH(object));
    dc_input_service_set_mouse_natural_scroll(app->input_service, enabled);
}

static void on_mouse_left_handed_toggled(GObject *object, GParamSpec *pspec, gpointer user_data) {
    (void)pspec;
    DcAppController *app = user_data;
    if (app->suppress_input_updates) return;
    gboolean enabled = gtk_switch_get_active(GTK_SWITCH(object));
    dc_input_service_set_mouse_left_handed(app->input_service, enabled);
}

/* ----------------------------------------------------- */
/*  Touchpad Signal Handlers                            */
/* ----------------------------------------------------- */

static void on_tp_enabled_toggled(GObject *object, GParamSpec *pspec, gpointer user_data) {
    (void)pspec;
    DcAppController *app = user_data;
    if (app->suppress_input_updates) return;
    gboolean enabled = gtk_switch_get_active(GTK_SWITCH(object));
    dc_input_service_set_touchpad_enabled(app->input_service, enabled);
}

static void on_tp_tap_to_click_toggled(GObject *object, GParamSpec *pspec, gpointer user_data) {
    (void)pspec;
    DcAppController *app = user_data;
    if (app->suppress_input_updates) return;
    gboolean enabled = gtk_switch_get_active(GTK_SWITCH(object));
    dc_input_service_set_touchpad_tap_to_click(app->input_service, enabled);
}

static void on_tp_natural_scroll_toggled(GObject *object, GParamSpec *pspec, gpointer user_data) {
    (void)pspec;
    DcAppController *app = user_data;
    if (app->suppress_input_updates) return;
    gboolean enabled = gtk_switch_get_active(GTK_SWITCH(object));
    dc_input_service_set_touchpad_natural_scroll(app->input_service, enabled);
}

static void on_tp_disable_typing_toggled(GObject *object, GParamSpec *pspec, gpointer user_data) {
    (void)pspec;
    DcAppController *app = user_data;
    if (app->suppress_input_updates) return;
    gboolean enabled = gtk_switch_get_active(GTK_SWITCH(object));
    dc_input_service_set_touchpad_disable_while_typing(app->input_service, enabled);
}

static void on_tp_scroll_method_changed(GtkComboBox *combo, gpointer user_data) {
    DcAppController *app = user_data;
    if (app->suppress_input_updates) return;
    gchar *method = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(combo));
    if (method) {
        // Dropdown strings shown via UI are capitalized like "Two Finger", but the ID handles the programmatic name
        // Wait, did we use IDs in mouse_page.c?
        // Yes: gtk_combo_box_text_append(combo, "two-finger", "Two Finger")
        const char *id = gtk_combo_box_get_active_id(GTK_COMBO_BOX(combo));
        if (id) {
            dc_input_service_set_touchpad_scroll_method(app->input_service, id);
        }
        g_free(method);
    }
}

static void on_tp_speed_changed(GtkRange *range, gpointer user_data) {
    DcAppController *app = user_data;
    if (app->suppress_input_updates) return;
    double val = gtk_range_get_value(range);
    dc_input_service_set_touchpad_speed(app->input_service, val);
}

/* ----------------------------------------------------- */
/*  Public Setup                                        */
/* ----------------------------------------------------- */

void dc_app_mouse_load(DcAppController *app) {
    app->suppress_input_updates = TRUE;

    // 1. Mouse Settings
    double m_accel = 0.0, m_speed = 1.0;
    gboolean m_ns = FALSE, m_lh = FALSE;
    dc_input_service_get_mouse_settings(app->input_service, &m_accel, &m_speed, &m_ns, &m_lh);

    gtk_range_set_value(GTK_RANGE(dc_mouse_page_get_mouse_accel_scale(app->mouse_page)), m_accel);
    gtk_range_set_value(GTK_RANGE(dc_mouse_page_get_mouse_speed_scale(app->mouse_page)), m_speed);
    gtk_switch_set_active(GTK_SWITCH(dc_mouse_page_get_mouse_natural_scroll_switch(app->mouse_page)), m_ns);
    gtk_switch_set_active(GTK_SWITCH(dc_mouse_page_get_mouse_left_handed_switch(app->mouse_page)), m_lh);

    // 2. Touchpad Settings
    gboolean tp_enabled = TRUE, tp_tap = TRUE, tp_ns = FALSE, tp_dis = TRUE;
    gchar *tp_method = NULL;
    double tp_speed = 0.5;

    dc_input_service_get_touchpad_settings(app->input_service, &tp_enabled, &tp_tap, &tp_ns, &tp_method, &tp_speed, &tp_dis);

    gtk_switch_set_active(GTK_SWITCH(dc_mouse_page_get_touchpad_enabled_switch(app->mouse_page)), tp_enabled);
    gtk_switch_set_active(GTK_SWITCH(dc_mouse_page_get_touchpad_tap_to_click_switch(app->mouse_page)), tp_tap);
    gtk_switch_set_active(GTK_SWITCH(dc_mouse_page_get_touchpad_natural_scroll_switch(app->mouse_page)), tp_ns);
    gtk_switch_set_active(GTK_SWITCH(dc_mouse_page_get_touchpad_disable_while_typing_switch(app->mouse_page)), tp_dis);
    gtk_range_set_value(GTK_RANGE(dc_mouse_page_get_touchpad_speed_scale(app->mouse_page)), tp_speed);

    if (tp_method) {
        gtk_combo_box_set_active_id(GTK_COMBO_BOX(dc_mouse_page_get_touchpad_scroll_method_combo(app->mouse_page)), tp_method);
        g_free(tp_method);
    }

    app->suppress_input_updates = FALSE;
}

void dc_app_mouse_connect_signals(DcAppController *app) {
    // Mouse
    g_signal_connect(dc_mouse_page_get_mouse_accel_scale(app->mouse_page), "value-changed", G_CALLBACK(on_mouse_accel_changed), app);
    g_signal_connect(dc_mouse_page_get_mouse_speed_scale(app->mouse_page), "value-changed", G_CALLBACK(on_mouse_speed_changed), app);
    g_signal_connect(dc_mouse_page_get_mouse_natural_scroll_switch(app->mouse_page), "notify::active", G_CALLBACK(on_mouse_natural_scroll_toggled), app);
    g_signal_connect(dc_mouse_page_get_mouse_left_handed_switch(app->mouse_page), "notify::active", G_CALLBACK(on_mouse_left_handed_toggled), app);

    // Touchpad
    g_signal_connect(dc_mouse_page_get_touchpad_enabled_switch(app->mouse_page), "notify::active", G_CALLBACK(on_tp_enabled_toggled), app);
    g_signal_connect(dc_mouse_page_get_touchpad_tap_to_click_switch(app->mouse_page), "notify::active", G_CALLBACK(on_tp_tap_to_click_toggled), app);
    g_signal_connect(dc_mouse_page_get_touchpad_natural_scroll_switch(app->mouse_page), "notify::active", G_CALLBACK(on_tp_natural_scroll_toggled), app);
    g_signal_connect(dc_mouse_page_get_touchpad_disable_while_typing_switch(app->mouse_page), "notify::active", G_CALLBACK(on_tp_disable_typing_toggled), app);
    g_signal_connect(dc_mouse_page_get_touchpad_speed_scale(app->mouse_page), "value-changed", G_CALLBACK(on_tp_speed_changed), app);
    g_signal_connect(dc_mouse_page_get_touchpad_scroll_method_combo(app->mouse_page), "changed", G_CALLBACK(on_tp_scroll_method_changed), app);
}
