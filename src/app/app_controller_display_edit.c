#include "app_controller_internal.h"

#include "services/display_edit_service.h"

static gboolean is_hour_in_range(int hour, int start_hour, int end_hour) {
    if (start_hour == end_hour) {
        return TRUE;
    }
    if (start_hour < end_hour) {
        return hour >= start_hour && hour < end_hour;
    }
    return hour >= start_hour || hour < end_hour;
}

static gboolean is_night_light_active_now(const DcDisplayEditConfig *config) {
    GDateTime *now;
    int hour;
    gboolean enabled;

    if (config == NULL || !config->night_light_enabled) {
        return FALSE;
    }

    if (!config->night_light_use_schedule) {
        return TRUE;
    }

    if (g_strcmp0(config->night_light_schedule, "always") == 0) {
        return TRUE;
    }

    now = g_date_time_new_now_local();
    hour = g_date_time_get_hour(now);

    if (g_strcmp0(config->night_light_schedule, "custom") == 0) {
        enabled = is_hour_in_range(hour,
                                   config->night_light_custom_start_hour,
                                   config->night_light_custom_end_hour);
        g_date_time_unref(now);
        return enabled;
    }

    enabled = is_hour_in_range(hour, 18, 7);
    g_date_time_unref(now);
    return enabled;
}

static void update_night_light_status(DcAppController *app, const DcDisplayEditConfig *config) {
    GtkWidget *label;
    char *message = NULL;

    label = dc_display_edit_page_get_night_light_status_label(app->display_edit_page);
    if (config == NULL || !config->night_light_enabled) {
        gtk_label_set_text(GTK_LABEL(label), "Night Light status: disabled.");
        return;
    }

    if (!config->night_light_use_schedule) {
        gtk_label_set_text(GTK_LABEL(label), "Night Light status: active now, schedule override is off.");
        return;
    }

    if (is_night_light_active_now(config)) {
        gtk_label_set_text(GTK_LABEL(label), "Night Light status: active now.");
        return;
    }

    if (g_strcmp0(config->night_light_schedule, "custom") == 0) {
        message = g_strdup_printf("Night Light status: enabled, waiting for custom window %02d:00-%02d:00.",
                                  config->night_light_custom_start_hour,
                                  config->night_light_custom_end_hour);
    } else if (g_strcmp0(config->night_light_schedule, "always") == 0) {
        gtk_label_set_text(GTK_LABEL(label), "Night Light status: active now.");
        return;
    } else {
        message = g_strdup("Night Light status: enabled, waiting for sunset window 18:00-07:00.");
    }

    gtk_label_set_text(GTK_LABEL(label), message);
    g_free(message);
}

void dc_app_display_edit_configure_capabilities(DcAppController *app) {
    DcVrrSupportInfo vrr_info;
    char *error_message = NULL;
    GtkWidget *vrr_switch;
    const char *status_text;

    vrr_switch = dc_display_edit_page_get_vrr_switch(app->display_edit_page);
    if (!dc_display_backend_get_vrr_support_info(app->backend, &vrr_info, &error_message)) {
        gtk_widget_set_sensitive(vrr_switch, FALSE);
        gtk_widget_set_tooltip_text(vrr_switch, "Variable Refresh Rate support could not be queried.");
        gtk_label_set_text(GTK_LABEL(dc_display_edit_page_get_vrr_status_label(app->display_edit_page)),
                           "VRR status: support query failed.");
        g_free(error_message);
        return;
    }

    if (vrr_info.any_writable) {
        gtk_widget_set_sensitive(vrr_switch, TRUE);
        gtk_widget_set_tooltip_text(vrr_switch, "Variable Refresh Rate can be controlled on at least one connected output.");
        if (gtk_switch_get_active(GTK_SWITCH(vrr_switch))) {
            status_text = "VRR status: enabled on writable supported outputs.";
        } else {
            status_text = "VRR status: available but currently disabled by policy.";
        }
        gtk_label_set_text(GTK_LABEL(dc_display_edit_page_get_vrr_status_label(app->display_edit_page)), status_text);
        g_free(error_message);
        return;
    }

    if (vrr_info.any_supported) {
        char *message;

        gtk_widget_set_sensitive(vrr_switch, FALSE);
        gtk_widget_set_tooltip_text(vrr_switch, "Connected outputs expose VRR-related properties, but none are writable through the current XRandR path.");
        message = g_strdup_printf("VRR status: supported on %u/%u outputs, but no writable control is exposed.",
                                  vrr_info.supported_outputs,
                                  vrr_info.connected_outputs);
        gtk_label_set_text(GTK_LABEL(dc_display_edit_page_get_vrr_status_label(app->display_edit_page)), message);
        g_free(message);
        g_free(error_message);
        return;
    }

    gtk_widget_set_sensitive(vrr_switch, FALSE);
    gtk_widget_set_tooltip_text(vrr_switch, "Variable Refresh Rate is not exposed by the current connected outputs or driver.");
    gtk_label_set_text(GTK_LABEL(dc_display_edit_page_get_vrr_status_label(app->display_edit_page)),
                       "VRR status: unavailable on current outputs or driver.");
    g_free(error_message);
}

static void apply_display_edit_config_to_ui(DcAppController *app, const DcDisplayEditConfig *config) {
    gtk_switch_set_active(GTK_SWITCH(dc_display_edit_page_get_night_light_switch(app->display_edit_page)), config->night_light_enabled);
    gtk_switch_set_active(GTK_SWITCH(dc_display_edit_page_get_night_light_use_schedule_switch(app->display_edit_page)), config->night_light_use_schedule);
    gtk_range_set_value(GTK_RANGE(dc_display_edit_page_get_night_light_temperature_scale(app->display_edit_page)), config->night_light_temperature);
    gtk_combo_box_set_active_id(GTK_COMBO_BOX(dc_display_edit_page_get_night_light_schedule_combo(app->display_edit_page)), config->night_light_schedule);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(dc_display_edit_page_get_night_light_custom_start_spin(app->display_edit_page)), config->night_light_custom_start_hour);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(dc_display_edit_page_get_night_light_custom_end_spin(app->display_edit_page)), config->night_light_custom_end_hour);
    gtk_switch_set_active(GTK_SWITCH(dc_display_edit_page_get_vrr_switch(app->display_edit_page)), config->vrr_enabled);
    gtk_switch_set_active(GTK_SWITCH(dc_display_edit_page_get_adaptive_brightness_switch(app->display_edit_page)), config->adaptive_brightness);
    gtk_range_set_value(GTK_RANGE(dc_display_edit_page_get_gamma_scale(app->display_edit_page)), config->gamma);
    gtk_range_set_value(GTK_RANGE(dc_display_edit_page_get_vibrance_scale(app->display_edit_page)), config->vibrance);
    update_night_light_status(app, config);
}

static DcDisplayEditConfig *collect_display_edit_config_from_ui(DcAppController *app) {
    DcDisplayEditConfig *config;
    const char *schedule;

    config = dc_display_edit_config_new();
    config->night_light_enabled = gtk_switch_get_active(GTK_SWITCH(dc_display_edit_page_get_night_light_switch(app->display_edit_page)));
    config->night_light_use_schedule = gtk_switch_get_active(GTK_SWITCH(dc_display_edit_page_get_night_light_use_schedule_switch(app->display_edit_page)));
    config->night_light_temperature = (int) gtk_range_get_value(GTK_RANGE(dc_display_edit_page_get_night_light_temperature_scale(app->display_edit_page)));
    schedule = gtk_combo_box_get_active_id(GTK_COMBO_BOX(dc_display_edit_page_get_night_light_schedule_combo(app->display_edit_page)));
    if (schedule != NULL) {
        g_free(config->night_light_schedule);
        config->night_light_schedule = g_strdup(schedule);
    }
    config->night_light_custom_start_hour = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(dc_display_edit_page_get_night_light_custom_start_spin(app->display_edit_page)));
    config->night_light_custom_end_hour = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(dc_display_edit_page_get_night_light_custom_end_spin(app->display_edit_page)));
    config->vrr_enabled = gtk_switch_get_active(GTK_SWITCH(dc_display_edit_page_get_vrr_switch(app->display_edit_page)));
    config->adaptive_brightness = gtk_switch_get_active(GTK_SWITCH(dc_display_edit_page_get_adaptive_brightness_switch(app->display_edit_page)));
    config->gamma = gtk_range_get_value(GTK_RANGE(dc_display_edit_page_get_gamma_scale(app->display_edit_page)));
    config->vibrance = (int) gtk_range_get_value(GTK_RANGE(dc_display_edit_page_get_vibrance_scale(app->display_edit_page)));
    return config;
}

void dc_app_display_edit_load(DcAppController *app) {
    DcDisplayEditConfig *config = NULL;
    char *error_message = NULL;

    app->suppress_display_edit_autosave = TRUE;

    if (dc_display_edit_config_load(&config, &error_message)) {
        apply_display_edit_config_to_ui(app, config);
        dc_display_backend_apply_display_edit(app->backend, config, NULL);
        dc_display_edit_config_free(config);
        app->suppress_display_edit_autosave = FALSE;
        return;
    }

    app->suppress_display_edit_autosave = FALSE;
    g_warning("%s", error_message != NULL ? error_message : "Failed to load display edit config.");
    g_free(error_message);
}

static void on_display_edit_save(DcAppController *app) {
    DcDisplayEditConfig *config;
    char *error_message = NULL;

    config = collect_display_edit_config_from_ui(app);

    if (!dc_display_edit_config_save(config, &error_message)) {
        g_warning("%s", error_message != NULL ? error_message : "Failed to save display edit config.");
        g_free(error_message);
        dc_display_edit_config_free(config);
        return;
    }

    g_free(error_message);
    error_message = NULL;
    if (!dc_display_backend_apply_display_edit(app->backend, config, &error_message)) {
        g_warning("%s", error_message != NULL ? error_message : "Failed to apply display edit config.");
    }

    update_night_light_status(app, config);
    g_free(error_message);
    dc_display_edit_config_free(config);
}

gboolean dc_app_display_edit_refresh_runtime(gpointer user_data) {
    DcAppController *app = user_data;
    DcDisplayEditConfig *config;
    char *error_message = NULL;

    if (!dc_display_edit_config_load(&config, &error_message)) {
        g_free(error_message);
        return G_SOURCE_CONTINUE;
    }

    if (!dc_display_backend_apply_display_edit(app->backend, config, &error_message)) {
        g_warning("%s", error_message != NULL ? error_message : "Failed to refresh display edit state.");
    }

    update_night_light_status(app, config);
    g_free(error_message);
    dc_display_edit_config_free(config);
    return G_SOURCE_CONTINUE;
}

static gboolean flush_display_edit_autosave(gpointer user_data) {
    DcAppController *app = user_data;

    app->display_edit_autosave_timeout_id = 0;
    on_display_edit_save(app);
    return G_SOURCE_REMOVE;
}

static void queue_display_edit_autosave(gpointer user_data) {
    DcAppController *app = user_data;

    if (app->suppress_display_edit_autosave) {
        return;
    }

    if (app->display_edit_autosave_timeout_id != 0) {
        g_source_remove(app->display_edit_autosave_timeout_id);
    }

    app->display_edit_autosave_timeout_id = g_timeout_add(200, flush_display_edit_autosave, app);
}

static void on_display_edit_widget_changed(GtkWidget *widget, gpointer user_data) {
    (void) widget;
    queue_display_edit_autosave(user_data);
}

static void on_display_edit_switch_active_changed(GObject *object, GParamSpec *pspec, gpointer user_data) {
    (void) object;
    (void) pspec;
    queue_display_edit_autosave(user_data);
}

void dc_app_display_edit_connect_signals(DcAppController *app) {
    g_signal_connect(dc_display_edit_page_get_night_light_switch(app->display_edit_page), "notify::active", G_CALLBACK(on_display_edit_switch_active_changed), app);
    g_signal_connect(dc_display_edit_page_get_night_light_use_schedule_switch(app->display_edit_page), "notify::active", G_CALLBACK(on_display_edit_switch_active_changed), app);
    g_signal_connect(dc_display_edit_page_get_night_light_temperature_scale(app->display_edit_page), "value-changed", G_CALLBACK(on_display_edit_widget_changed), app);
    g_signal_connect(dc_display_edit_page_get_night_light_schedule_combo(app->display_edit_page), "changed", G_CALLBACK(on_display_edit_widget_changed), app);
    g_signal_connect(dc_display_edit_page_get_night_light_custom_start_spin(app->display_edit_page), "value-changed", G_CALLBACK(on_display_edit_widget_changed), app);
    g_signal_connect(dc_display_edit_page_get_night_light_custom_end_spin(app->display_edit_page), "value-changed", G_CALLBACK(on_display_edit_widget_changed), app);
    g_signal_connect(dc_display_edit_page_get_vrr_switch(app->display_edit_page), "notify::active", G_CALLBACK(on_display_edit_switch_active_changed), app);
    g_signal_connect(dc_display_edit_page_get_adaptive_brightness_switch(app->display_edit_page), "notify::active", G_CALLBACK(on_display_edit_switch_active_changed), app);
    g_signal_connect(dc_display_edit_page_get_gamma_scale(app->display_edit_page), "value-changed", G_CALLBACK(on_display_edit_widget_changed), app);
    g_signal_connect(dc_display_edit_page_get_vibrance_scale(app->display_edit_page), "value-changed", G_CALLBACK(on_display_edit_widget_changed), app);
}
