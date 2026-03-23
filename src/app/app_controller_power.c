#include "app_controller_internal.h"
#include <string.h>

/* ----------------------------------------------------- */
/*  Signal Handlers from UI                             */
/* ----------------------------------------------------- */

static void on_brightness_changed(GtkRange *range, gpointer user_data) {
    DcAppController *app = user_data;
    if (app->suppress_power_updates) return;
    int level = (int)gtk_range_get_value(range);
    dc_power_service_set_brightness(app->power_service, level);
}

static void on_kb_brightness_changed(GtkRange *range, gpointer user_data) {
    DcAppController *app = user_data;
    if (app->suppress_power_updates) return;
    int level = (int)gtk_range_get_value(range);
    dc_power_service_set_keyboard_brightness(app->power_service, level);
}

static void on_profile_combo_changed(GtkComboBox *combo, gpointer user_data) {
    DcAppController *app = user_data;
    if (app->suppress_power_updates) return;
    gchar *profile = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(combo));
    if (profile) {
        dc_power_service_set_active_profile(app->power_service, profile);
        g_free(profile);
    }
}

static void on_idle_timeout_changed(GtkComboBox *combo, gpointer user_data) {
    DcAppController *app = user_data;
    if (app->suppress_power_updates) return;

    const gchar *dim_id = gtk_combo_box_get_active_id(GTK_COMBO_BOX(dc_power_page_get_dim_combo(app->power_page)));
    const gchar *blank_id = gtk_combo_box_get_active_id(GTK_COMBO_BOX(dc_power_page_get_blank_combo(app->power_page)));
    const gchar *suspend_id = gtk_combo_box_get_active_id(GTK_COMBO_BOX(dc_power_page_get_suspend_combo(app->power_page)));

    guint dim = dim_id ? strtoul(dim_id, NULL, 10) : 0;
    guint blank = blank_id ? strtoul(blank_id, NULL, 10) : 0;
    guint suspend = suspend_id ? strtoul(suspend_id, NULL, 10) : 0;

    dc_power_service_set_idle_timeouts(app->power_service, dim, blank, suspend);
}

static void on_hw_action_changed(GtkComboBox *combo, gpointer user_data) {
    DcAppController *app = user_data;
    if (app->suppress_power_updates) return;

    gchar *lid_ac = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(dc_power_page_get_lid_ac_combo(app->power_page)));
    gchar *lid_bat = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(dc_power_page_get_lid_bat_combo(app->power_page)));
    gchar *power_btn = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(dc_power_page_get_power_btn_combo(app->power_page)));
    gchar *crit_batt = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(dc_power_page_get_critical_batt_combo(app->power_page)));

    if (lid_ac && lid_bat) {
        dc_power_service_set_lid_action(app->power_service, lid_ac, lid_bat);
    }
    if (power_btn) {
        dc_power_service_set_power_button_action(app->power_service, power_btn);
    }
    if (crit_batt) {
        dc_power_service_set_critical_action(app->power_service, crit_batt);
    }

    g_free(lid_ac); g_free(lid_bat);
    g_free(power_btn); g_free(crit_batt);
}

/* ----------------------------------------------------- */
/*  Signal Handlers from Service                        */
/* ----------------------------------------------------- */

static void on_service_brightness_changed(DcPowerService *svc, int level, gpointer user_data) {
    DcAppController *app = user_data;
    app->suppress_power_updates = TRUE;
    gtk_range_set_value(GTK_RANGE(dc_power_page_get_brightness_scale(app->power_page)), level);
    app->suppress_power_updates = FALSE;
}

static void on_service_kb_brightness_changed(DcPowerService *svc, int level, gpointer user_data) {
    DcAppController *app = user_data;
    app->suppress_power_updates = TRUE;
    gtk_range_set_value(GTK_RANGE(dc_power_page_get_kb_brightness_scale(app->power_page)), level);
    app->suppress_power_updates = FALSE;
}

static void on_service_battery_changed(DcPowerService *svc, double percentage, gboolean charging, gpointer user_data) {
    DcAppController *app = user_data;
    // For now we don't have time_to_empty from the signal directly so we re-fetch it
    double p; gboolean c; gint64 t;
    dc_power_service_get_battery_info(app->power_service, &p, &c, &t);
    dc_power_page_set_battery_status(app->power_page, p, c, t);
}

static void on_service_profile_changed(DcPowerService *svc, const char *profile, gpointer user_data) {
    DcAppController *app = user_data;
    app->suppress_power_updates = TRUE;
    
    GtkComboBox *combo = GTK_COMBO_BOX(dc_power_page_get_profiles_combo(app->power_page));
    GtkTreeModel *model = gtk_combo_box_get_model(combo);
    GtkTreeIter iter;
    gboolean valid = gtk_tree_model_get_iter_first(model, &iter);
    while (valid) {
        gchar *id = NULL;
        gtk_tree_model_get(model, &iter, 0, &id, -1);
        if (g_strcmp0(id, profile) == 0) {
            gtk_combo_box_set_active_iter(combo, &iter);
            g_free(id);
            break;
        }
        g_free(id);
        valid = gtk_tree_model_iter_next(model, &iter);
    }
    
    app->suppress_power_updates = FALSE;
}

/* ----------------------------------------------------- */
/*  Public Setup                                        */
/* ----------------------------------------------------- */

void dc_app_power_load(DcAppController *app) {
    app->suppress_power_updates = TRUE;

    // Load Brightness
    int brightness = dc_power_service_get_brightness(app->power_service);
    gtk_range_set_value(GTK_RANGE(dc_power_page_get_brightness_scale(app->power_page)), brightness);

    // Load Keyboard Brightness
    if (dc_power_service_is_keyboard_supported(app->power_service)) {
        int kb_brightness = dc_power_service_get_keyboard_brightness(app->power_service);
        gtk_range_set_value(GTK_RANGE(dc_power_page_get_kb_brightness_scale(app->power_page)), kb_brightness);
    } else {
        gtk_widget_set_sensitive(dc_power_page_get_kb_brightness_scale(app->power_page), FALSE);
    }

    // Load Battery
    double p; gboolean c; gint64 t;
    dc_power_service_get_battery_info(app->power_service, &p, &c, &t);
    dc_power_page_set_battery_status(app->power_page, p, c, t);

    // Load Profiles
    if (dc_power_service_is_profiles_available(app->power_service)) {
        char **profiles = dc_power_service_get_profiles(app->power_service);
        if (profiles) {
            GtkComboBoxText *combo = GTK_COMBO_BOX_TEXT(dc_power_page_get_profiles_combo(app->power_page));
            for (int i = 0; profiles[i]; i++) {
                gtk_combo_box_text_append(combo, profiles[i], profiles[i]);
                g_free(profiles[i]);
            }
            g_free(profiles);
        }
        gchar *active_profile = dc_power_service_get_active_profile(app->power_service);
        gtk_combo_box_set_active_id(GTK_COMBO_BOX(dc_power_page_get_profiles_combo(app->power_page)), active_profile);
        g_free(active_profile);
    } else {
        gtk_widget_set_sensitive(dc_power_page_get_profiles_combo(app->power_page), FALSE);
    }

    // Load Timeouts
    guint dim, blank, suspend;
    dc_power_service_get_idle_timeouts(app->power_service, &dim, &blank, &suspend);
    char buf[32];
    snprintf(buf, sizeof(buf), "%u", dim);
    gtk_combo_box_set_active_id(GTK_COMBO_BOX(dc_power_page_get_dim_combo(app->power_page)), buf);
    snprintf(buf, sizeof(buf), "%u", blank);
    gtk_combo_box_set_active_id(GTK_COMBO_BOX(dc_power_page_get_blank_combo(app->power_page)), buf);
    snprintf(buf, sizeof(buf), "%u", suspend);
    gtk_combo_box_set_active_id(GTK_COMBO_BOX(dc_power_page_get_suspend_combo(app->power_page)), buf);

    // Load Hardware Actions
    gchar *lid_ac = NULL, *lid_bat = NULL, *power_btn = NULL, *crit_batt = NULL;
    dc_power_service_get_lid_action(app->power_service, &lid_ac, &lid_bat);
    dc_power_service_get_power_button_action(app->power_service, &power_btn);
    dc_power_service_get_critical_action(app->power_service, &crit_batt);

    if (lid_ac) gtk_combo_box_set_active_id(GTK_COMBO_BOX(dc_power_page_get_lid_ac_combo(app->power_page)), lid_ac);
    if (lid_bat) gtk_combo_box_set_active_id(GTK_COMBO_BOX(dc_power_page_get_lid_bat_combo(app->power_page)), lid_bat);
    if (power_btn) gtk_combo_box_set_active_id(GTK_COMBO_BOX(dc_power_page_get_power_btn_combo(app->power_page)), power_btn);
    if (crit_batt) gtk_combo_box_set_active_id(GTK_COMBO_BOX(dc_power_page_get_critical_batt_combo(app->power_page)), crit_batt);

    g_free(lid_ac); g_free(lid_bat);
    g_free(power_btn); g_free(crit_batt);

    app->suppress_power_updates = FALSE;
}

void dc_app_power_connect_signals(DcAppController *app) {
    // UI -> Service
    g_signal_connect(dc_power_page_get_brightness_scale(app->power_page), "value-changed", G_CALLBACK(on_brightness_changed), app);
    g_signal_connect(dc_power_page_get_kb_brightness_scale(app->power_page), "value-changed", G_CALLBACK(on_kb_brightness_changed), app);
    g_signal_connect(dc_power_page_get_profiles_combo(app->power_page), "changed", G_CALLBACK(on_profile_combo_changed), app);
    
    g_signal_connect(dc_power_page_get_dim_combo(app->power_page), "changed", G_CALLBACK(on_idle_timeout_changed), app);
    g_signal_connect(dc_power_page_get_blank_combo(app->power_page), "changed", G_CALLBACK(on_idle_timeout_changed), app);
    g_signal_connect(dc_power_page_get_suspend_combo(app->power_page), "changed", G_CALLBACK(on_idle_timeout_changed), app);

    g_signal_connect(dc_power_page_get_lid_ac_combo(app->power_page), "changed", G_CALLBACK(on_hw_action_changed), app);
    g_signal_connect(dc_power_page_get_lid_bat_combo(app->power_page), "changed", G_CALLBACK(on_hw_action_changed), app);
    g_signal_connect(dc_power_page_get_power_btn_combo(app->power_page), "changed", G_CALLBACK(on_hw_action_changed), app);
    g_signal_connect(dc_power_page_get_critical_batt_combo(app->power_page), "changed", G_CALLBACK(on_hw_action_changed), app);

    // Service -> UI
    g_signal_connect(app->power_service, "brightness-changed", G_CALLBACK(on_service_brightness_changed), app);
    g_signal_connect(app->power_service, "keyboard-brightness-changed", G_CALLBACK(on_service_kb_brightness_changed), app);
    g_signal_connect(app->power_service, "battery-changed", G_CALLBACK(on_service_battery_changed), app);
    g_signal_connect(app->power_service, "profile-changed", G_CALLBACK(on_service_profile_changed), app);
}
