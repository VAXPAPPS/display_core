#include "app/app_controller.h"

#include "domain/display_types.h"
#include "services/profile_service.h"
#include "services/display_edit_service.h"
#include "services/venom_config_service.h"
#include "services/xrandr_service.h"
#include "ui/output_row.h"
#include "ui/pages/compositor_page.h"
#include "ui/pages/display_edit_page.h"
#include "ui/pages/display_page.h"
#include "ui/pages/window_manager_page.h"
#include "ui/preview_canvas.h"

#define DC_REVERT_TIMEOUT_SECONDS 15
#define DC_PRIMARY_VENOM_CONFIG_PATH "/home/x/.config/venom-miasma/venom.conf"
#define DC_FALLBACK_VENOM_CONFIG_PATH "/etc/venom/venom.conf"

typedef struct {
    DcXrandrService *service;
    GtkApplication *gtk_app;
    GtkWidget *window;
    GtkWidget *stack;
    DcDisplayPage *display_page;
    DcDisplayEditPage *display_edit_page;
    DcWindowManagerPage *window_manager_page;
    DcCompositorPage *compositor_page;
    GPtrArray *output_models;
    GPtrArray *rows;
    DcPreviewCanvas *preview;
    guint compositor_autosave_timeout_id;
    gboolean suppress_compositor_autosave;
    guint display_edit_autosave_timeout_id;
    gboolean suppress_display_edit_autosave;
    guint display_edit_refresh_timeout_id;
} DcAppController;

typedef struct {
    GtkWidget *dialog;
    GtkWidget *label;
    guint timeout_id;
    gint remaining_seconds;
} DcRevertDialogData;

static void add_css_class(GtkWidget *widget, const char *class_name) {
    gtk_style_context_add_class(gtk_widget_get_style_context(widget), class_name);
}

static void install_app_css(void) {
    GtkCssProvider *provider;
    GdkScreen *screen;
    char *cwd;
    char *css_path;
    GError *error = NULL;

    provider = gtk_css_provider_new();
    cwd = g_get_current_dir();
    css_path = g_build_filename(cwd, "assets", "theme.css", NULL);
    gtk_css_provider_load_from_path(provider, css_path, &error);
    screen = gdk_screen_get_default();
    if (screen != NULL) {
        gtk_style_context_add_provider_for_screen(screen,
                                                  GTK_STYLE_PROVIDER(provider),
                                                  GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    }

    if (error != NULL) {
        g_warning("Failed to load CSS from %s: %s", css_path, error->message);
        g_error_free(error);
    }

    g_free(cwd);
    g_free(css_path);
    g_object_unref(provider);
}

static void set_status(DcAppController *app, const char *message) {
    gtk_label_set_text(GTK_LABEL(dc_display_page_get_status_label(app->display_page)), message);
}

static void configure_display_edit_capabilities(DcAppController *app) {
    DcVrrSupportInfo vrr_info;
    char *error_message = NULL;
    GtkWidget *vrr_switch;
    const char *status_text;

    vrr_switch = dc_display_edit_page_get_vrr_switch(app->display_edit_page);
    if (!dc_xrandr_service_get_vrr_support_info(app->service, &vrr_info, &error_message)) {
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

static char *resolve_venom_config_path(void) {
    if (g_file_test(DC_PRIMARY_VENOM_CONFIG_PATH, G_FILE_TEST_EXISTS)) {
        return g_strdup(DC_PRIMARY_VENOM_CONFIG_PATH);
    }

    if (g_file_test(DC_FALLBACK_VENOM_CONFIG_PATH, G_FILE_TEST_EXISTS)) {
        return g_strdup(DC_FALLBACK_VENOM_CONFIG_PATH);
    }

    return g_strdup(DC_PRIMARY_VENOM_CONFIG_PATH);
}

static void apply_venom_config_to_ui(DcAppController *app, const DcVenomConfig *config) {
    gtk_switch_set_active(GTK_SWITCH(dc_compositor_page_get_shadow_switch(app->compositor_page)), config->shadow);
    gtk_range_set_value(GTK_RANGE(dc_compositor_page_get_shadow_radius_scale(app->compositor_page)), config->shadow_radius);
    gtk_range_set_value(GTK_RANGE(dc_compositor_page_get_shadow_opacity_scale(app->compositor_page)), config->shadow_opacity * 100.0);
    gtk_range_set_value(GTK_RANGE(dc_compositor_page_get_shadow_red_scale(app->compositor_page)), config->shadow_red);
    gtk_range_set_value(GTK_RANGE(dc_compositor_page_get_shadow_green_scale(app->compositor_page)), config->shadow_green);
    gtk_range_set_value(GTK_RANGE(dc_compositor_page_get_shadow_blue_scale(app->compositor_page)), config->shadow_blue);
    gtk_switch_set_active(GTK_SWITCH(dc_compositor_page_get_fading_switch(app->compositor_page)), config->fading);
    gtk_range_set_value(GTK_RANGE(dc_compositor_page_get_active_opacity_scale(app->compositor_page)), config->active_opacity * 100.0);
    gtk_range_set_value(GTK_RANGE(dc_compositor_page_get_inactive_opacity_scale(app->compositor_page)), config->inactive_opacity * 100.0);
    gtk_range_set_value(GTK_RANGE(dc_compositor_page_get_corner_radius_scale(app->compositor_page)), config->corner_radius);
    gtk_switch_set_active(GTK_SWITCH(dc_compositor_page_get_detect_rounded_switch(app->compositor_page)), config->detect_rounded_corners);
    gtk_combo_box_set_active_id(GTK_COMBO_BOX(dc_compositor_page_get_blur_method_combo(app->compositor_page)), config->blur_method);
    gtk_range_set_value(GTK_RANGE(dc_compositor_page_get_blur_strength_scale(app->compositor_page)), config->blur_strength);
    gtk_switch_set_active(GTK_SWITCH(dc_compositor_page_get_blur_background_switch(app->compositor_page)), config->blur_background);
    gtk_switch_set_active(GTK_SWITCH(dc_compositor_page_get_blur_background_frame_switch(app->compositor_page)), config->blur_background_frame);
    gtk_combo_box_set_active_id(GTK_COMBO_BOX(dc_compositor_page_get_backend_combo(app->compositor_page)), config->backend);
    gtk_switch_set_active(GTK_SWITCH(dc_compositor_page_get_vsync_switch(app->compositor_page)), config->vsync);
    gtk_switch_set_active(GTK_SWITCH(dc_compositor_page_get_use_damage_switch(app->compositor_page)), config->use_damage);
}

static void apply_display_edit_config_to_ui(DcAppController *app, const DcDisplayEditConfig *config) {
    gtk_switch_set_active(GTK_SWITCH(dc_display_edit_page_get_night_light_switch(app->display_edit_page)), config->night_light_enabled);
    gtk_range_set_value(GTK_RANGE(dc_display_edit_page_get_night_light_temperature_scale(app->display_edit_page)), config->night_light_temperature);
    gtk_combo_box_set_active_id(GTK_COMBO_BOX(dc_display_edit_page_get_night_light_schedule_combo(app->display_edit_page)), config->night_light_schedule);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(dc_display_edit_page_get_night_light_custom_start_spin(app->display_edit_page)), config->night_light_custom_start_hour);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(dc_display_edit_page_get_night_light_custom_end_spin(app->display_edit_page)), config->night_light_custom_end_hour);
    gtk_switch_set_active(GTK_SWITCH(dc_display_edit_page_get_vrr_switch(app->display_edit_page)), config->vrr_enabled);
    gtk_switch_set_active(GTK_SWITCH(dc_display_edit_page_get_adaptive_brightness_switch(app->display_edit_page)), config->adaptive_brightness);
    gtk_range_set_value(GTK_RANGE(dc_display_edit_page_get_gamma_scale(app->display_edit_page)), config->gamma);
    gtk_range_set_value(GTK_RANGE(dc_display_edit_page_get_vibrance_scale(app->display_edit_page)), config->vibrance);
}

static DcVenomConfig *collect_venom_config_from_ui(DcAppController *app) {
    DcVenomConfig *config = dc_venom_config_new();
    const char *blur_method;
    const char *backend;

    config->shadow = gtk_switch_get_active(GTK_SWITCH(dc_compositor_page_get_shadow_switch(app->compositor_page)));
    config->shadow_radius = (int) gtk_range_get_value(GTK_RANGE(dc_compositor_page_get_shadow_radius_scale(app->compositor_page)));
    config->shadow_opacity = gtk_range_get_value(GTK_RANGE(dc_compositor_page_get_shadow_opacity_scale(app->compositor_page))) / 100.0;
    config->shadow_red = gtk_range_get_value(GTK_RANGE(dc_compositor_page_get_shadow_red_scale(app->compositor_page)));
    config->shadow_green = gtk_range_get_value(GTK_RANGE(dc_compositor_page_get_shadow_green_scale(app->compositor_page)));
    config->shadow_blue = gtk_range_get_value(GTK_RANGE(dc_compositor_page_get_shadow_blue_scale(app->compositor_page)));
    config->fading = gtk_switch_get_active(GTK_SWITCH(dc_compositor_page_get_fading_switch(app->compositor_page)));
    config->active_opacity = gtk_range_get_value(GTK_RANGE(dc_compositor_page_get_active_opacity_scale(app->compositor_page))) / 100.0;
    config->inactive_opacity = gtk_range_get_value(GTK_RANGE(dc_compositor_page_get_inactive_opacity_scale(app->compositor_page))) / 100.0;
    config->corner_radius = (int) gtk_range_get_value(GTK_RANGE(dc_compositor_page_get_corner_radius_scale(app->compositor_page)));
    config->detect_rounded_corners = gtk_switch_get_active(GTK_SWITCH(dc_compositor_page_get_detect_rounded_switch(app->compositor_page)));

    blur_method = gtk_combo_box_get_active_id(GTK_COMBO_BOX(dc_compositor_page_get_blur_method_combo(app->compositor_page)));
    if (blur_method != NULL) {
        g_free(config->blur_method);
        config->blur_method = g_strdup(blur_method);
    }

    config->blur_strength = (int) gtk_range_get_value(GTK_RANGE(dc_compositor_page_get_blur_strength_scale(app->compositor_page)));
    config->blur_background = gtk_switch_get_active(GTK_SWITCH(dc_compositor_page_get_blur_background_switch(app->compositor_page)));
    config->blur_background_frame = gtk_switch_get_active(GTK_SWITCH(dc_compositor_page_get_blur_background_frame_switch(app->compositor_page)));

    backend = gtk_combo_box_get_active_id(GTK_COMBO_BOX(dc_compositor_page_get_backend_combo(app->compositor_page)));
    if (backend != NULL) {
        g_free(config->backend);
        config->backend = g_strdup(backend);
    }

    config->vsync = gtk_switch_get_active(GTK_SWITCH(dc_compositor_page_get_vsync_switch(app->compositor_page)));
    config->use_damage = gtk_switch_get_active(GTK_SWITCH(dc_compositor_page_get_use_damage_switch(app->compositor_page)));
    return config;
}

static DcDisplayEditConfig *collect_display_edit_config_from_ui(DcAppController *app) {
    DcDisplayEditConfig *config;
    const char *schedule;

    config = dc_display_edit_config_new();
    config->night_light_enabled = gtk_switch_get_active(GTK_SWITCH(dc_display_edit_page_get_night_light_switch(app->display_edit_page)));
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

static void on_compositor_load_clicked(GtkButton *button, gpointer user_data) {
    DcAppController *app = user_data;
    DcVenomConfig *config = NULL;
    char *error_message = NULL;
    char *config_path;

    (void) button;

    config_path = resolve_venom_config_path();
    app->suppress_compositor_autosave = TRUE;

    if (dc_venom_config_load(config_path, &config, &error_message)) {
        apply_venom_config_to_ui(app, config);
        dc_venom_config_free(config);
        app->suppress_compositor_autosave = FALSE;
        g_free(config_path);
        return;
    }

    app->suppress_compositor_autosave = FALSE;
    g_warning("%s", error_message != NULL ? error_message : "Failed to load venom.conf.");
    g_free(error_message);
    g_free(config_path);
}

static void on_compositor_save_clicked(GtkButton *button, gpointer user_data) {
    DcAppController *app = user_data;
    DcVenomConfig *config;
    char *error_message = NULL;
    char *config_path;

    (void) button;

    config = collect_venom_config_from_ui(app);
    config_path = resolve_venom_config_path();

    if (!dc_venom_config_save(config_path, config, &error_message)) {
        g_warning("%s", error_message != NULL ? error_message : "Failed to save venom.conf.");
    }

    g_free(error_message);
    g_free(config_path);
    dc_venom_config_free(config);
}

static void on_display_edit_load(DcAppController *app) {
    DcDisplayEditConfig *config = NULL;
    char *error_message = NULL;

    app->suppress_display_edit_autosave = TRUE;

    if (dc_display_edit_config_load(&config, &error_message)) {
        apply_display_edit_config_to_ui(app, config);
        dc_xrandr_service_apply_display_edit(app->service, config, NULL);
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
    if (!dc_xrandr_service_apply_display_edit(app->service, config, &error_message)) {
        g_warning("%s", error_message != NULL ? error_message : "Failed to apply display edit config.");
    }

    g_free(error_message);
    dc_display_edit_config_free(config);
}

static gboolean refresh_display_edit_runtime(gpointer user_data) {
    DcAppController *app = user_data;
    DcDisplayEditConfig *config;
    char *error_message = NULL;

    if (!dc_display_edit_config_load(&config, &error_message)) {
        g_free(error_message);
        return G_SOURCE_CONTINUE;
    }

    if (!dc_xrandr_service_apply_display_edit(app->service, config, &error_message)) {
        g_warning("%s", error_message != NULL ? error_message : "Failed to refresh display edit state.");
    }

    g_free(error_message);
    dc_display_edit_config_free(config);
    return G_SOURCE_CONTINUE;
}

static gboolean flush_compositor_autosave(gpointer user_data) {
    DcAppController *app = user_data;

    app->compositor_autosave_timeout_id = 0;
    on_compositor_save_clicked(NULL, app);
    return G_SOURCE_REMOVE;
}

static gboolean flush_display_edit_autosave(gpointer user_data) {
    DcAppController *app = user_data;

    app->display_edit_autosave_timeout_id = 0;
    on_display_edit_save(app);
    return G_SOURCE_REMOVE;
}

static void queue_compositor_autosave(gpointer user_data) {
    DcAppController *app = user_data;

    if (app->suppress_compositor_autosave) {
        return;
    }

    if (app->compositor_autosave_timeout_id != 0) {
        g_source_remove(app->compositor_autosave_timeout_id);
    }

    app->compositor_autosave_timeout_id = g_timeout_add(200, flush_compositor_autosave, app);
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

static void on_compositor_widget_changed(GtkWidget *widget, gpointer user_data) {
    (void) widget;
    queue_compositor_autosave(user_data);
}

static void on_compositor_switch_active_changed(GObject *object, GParamSpec *pspec, gpointer user_data) {
    (void) object;
    (void) pspec;
    queue_compositor_autosave(user_data);
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

static void connect_compositor_autosave_signals(DcAppController *app) {
    g_signal_connect(dc_compositor_page_get_shadow_switch(app->compositor_page), "notify::active", G_CALLBACK(on_compositor_switch_active_changed), app);
    g_signal_connect(dc_compositor_page_get_shadow_radius_scale(app->compositor_page), "value-changed", G_CALLBACK(on_compositor_widget_changed), app);
    g_signal_connect(dc_compositor_page_get_shadow_opacity_scale(app->compositor_page), "value-changed", G_CALLBACK(on_compositor_widget_changed), app);
    g_signal_connect(dc_compositor_page_get_shadow_red_scale(app->compositor_page), "value-changed", G_CALLBACK(on_compositor_widget_changed), app);
    g_signal_connect(dc_compositor_page_get_shadow_green_scale(app->compositor_page), "value-changed", G_CALLBACK(on_compositor_widget_changed), app);
    g_signal_connect(dc_compositor_page_get_shadow_blue_scale(app->compositor_page), "value-changed", G_CALLBACK(on_compositor_widget_changed), app);
    g_signal_connect(dc_compositor_page_get_fading_switch(app->compositor_page), "notify::active", G_CALLBACK(on_compositor_switch_active_changed), app);
    g_signal_connect(dc_compositor_page_get_active_opacity_scale(app->compositor_page), "value-changed", G_CALLBACK(on_compositor_widget_changed), app);
    g_signal_connect(dc_compositor_page_get_inactive_opacity_scale(app->compositor_page), "value-changed", G_CALLBACK(on_compositor_widget_changed), app);
    g_signal_connect(dc_compositor_page_get_corner_radius_scale(app->compositor_page), "value-changed", G_CALLBACK(on_compositor_widget_changed), app);
    g_signal_connect(dc_compositor_page_get_detect_rounded_switch(app->compositor_page), "notify::active", G_CALLBACK(on_compositor_switch_active_changed), app);
    g_signal_connect(dc_compositor_page_get_blur_method_combo(app->compositor_page), "changed", G_CALLBACK(on_compositor_widget_changed), app);
    g_signal_connect(dc_compositor_page_get_blur_strength_scale(app->compositor_page), "value-changed", G_CALLBACK(on_compositor_widget_changed), app);
    g_signal_connect(dc_compositor_page_get_blur_background_switch(app->compositor_page), "notify::active", G_CALLBACK(on_compositor_switch_active_changed), app);
    g_signal_connect(dc_compositor_page_get_blur_background_frame_switch(app->compositor_page), "notify::active", G_CALLBACK(on_compositor_switch_active_changed), app);
    g_signal_connect(dc_compositor_page_get_backend_combo(app->compositor_page), "changed", G_CALLBACK(on_compositor_widget_changed), app);
    g_signal_connect(dc_compositor_page_get_vsync_switch(app->compositor_page), "notify::active", G_CALLBACK(on_compositor_switch_active_changed), app);
    g_signal_connect(dc_compositor_page_get_use_damage_switch(app->compositor_page), "notify::active", G_CALLBACK(on_compositor_switch_active_changed), app);
}

static void connect_display_edit_autosave_signals(DcAppController *app) {
    g_signal_connect(dc_display_edit_page_get_night_light_switch(app->display_edit_page), "notify::active", G_CALLBACK(on_display_edit_switch_active_changed), app);
    g_signal_connect(dc_display_edit_page_get_night_light_temperature_scale(app->display_edit_page), "value-changed", G_CALLBACK(on_display_edit_widget_changed), app);
    g_signal_connect(dc_display_edit_page_get_night_light_schedule_combo(app->display_edit_page), "changed", G_CALLBACK(on_display_edit_widget_changed), app);
    g_signal_connect(dc_display_edit_page_get_night_light_custom_start_spin(app->display_edit_page), "value-changed", G_CALLBACK(on_display_edit_widget_changed), app);
    g_signal_connect(dc_display_edit_page_get_night_light_custom_end_spin(app->display_edit_page), "value-changed", G_CALLBACK(on_display_edit_widget_changed), app);
    g_signal_connect(dc_display_edit_page_get_vrr_switch(app->display_edit_page), "notify::active", G_CALLBACK(on_display_edit_switch_active_changed), app);
    g_signal_connect(dc_display_edit_page_get_adaptive_brightness_switch(app->display_edit_page), "notify::active", G_CALLBACK(on_display_edit_switch_active_changed), app);
    g_signal_connect(dc_display_edit_page_get_gamma_scale(app->display_edit_page), "value-changed", G_CALLBACK(on_display_edit_widget_changed), app);
    g_signal_connect(dc_display_edit_page_get_vibrance_scale(app->display_edit_page), "value-changed", G_CALLBACK(on_display_edit_widget_changed), app);
}


static gboolean is_internal_output_name(const char *name) {
    return g_str_has_prefix(name, "eDP") ||
           g_str_has_prefix(name, "LVDS") ||
           g_str_has_prefix(name, "DSI");
}

static void refresh_profile_list(DcAppController *app) {
    GStrv profile_names = NULL;
    char *error_message = NULL;
    guint i;

    gtk_combo_box_text_remove_all(GTK_COMBO_BOX_TEXT(dc_display_page_get_profile_combo(app->display_page)));

    if (!dc_profile_service_list(&profile_names, &error_message)) {
        set_status(app, error_message != NULL ? error_message : "Failed to load profile list.");
        g_free(error_message);
        return;
    }

    for (i = 0; profile_names[i] != NULL; i++) {
            gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(dc_display_page_get_profile_combo(app->display_page)), profile_names[i]);
    }

    if (profile_names[0] != NULL) {
        gtk_combo_box_set_active(GTK_COMBO_BOX(dc_display_page_get_profile_combo(app->display_page)), 0);
    }

    g_strfreev(profile_names);
    g_free(error_message);
}

static GPtrArray *collect_current_configs(DcAppController *app) {
    GPtrArray *configs = g_ptr_array_new_with_free_func((GDestroyNotify) dc_display_config_free);
    guint i;

    for (i = 0; i < app->rows->len; i++) {
        DcOutputRow *row = g_ptr_array_index(app->rows, i);
        DcDisplayConfig *config = dc_display_config_new();

        config->output_id = dc_output_row_get_output(row)->output_id;
        config->output_name = g_strdup(dc_output_row_get_output(row)->name);
        config->enabled = dc_output_row_is_enabled(row);
        config->primary = dc_output_row_is_primary(row);
        config->mode = dc_output_row_get_selected_mode(row);
        config->rotation = dc_output_row_get_selected_rotation(row);
        config->x = dc_output_row_get_x(row);
        config->y = dc_output_row_get_y(row);
        g_ptr_array_add(configs, config);
    }

    return configs;
}

static DcOutputRow *find_row_by_name(DcAppController *app, const char *output_name) {
    guint i;

    for (i = 0; i < app->rows->len; i++) {
        DcOutputRow *row = g_ptr_array_index(app->rows, i);
        if (g_strcmp0(dc_output_row_get_output(row)->name, output_name) == 0) {
            return row;
        }
    }

    return NULL;
}

static void apply_configs_to_rows(DcAppController *app, GPtrArray *configs) {
    guint i;

    for (i = 0; i < app->rows->len; i++) {
        dc_output_row_set_primary(g_ptr_array_index(app->rows, i), FALSE);
    }

    for (i = 0; i < configs->len; i++) {
        DcDisplayConfig *config = g_ptr_array_index(configs, i);
        DcOutputRow *row = find_row_by_name(app, config->output_name);

        if (row == NULL) {
            continue;
        }

        dc_output_row_set_enabled(row, config->enabled);
        dc_output_row_set_mode(row, config->mode);
        dc_output_row_set_rotation(row, config->rotation);
        dc_output_row_set_position(row, config->x, config->y);
        dc_output_row_set_primary(row, config->primary);
    }

    dc_preview_canvas_queue_draw(app->preview);
}

static void free_row_list(GPtrArray **rows) {
    if (*rows == NULL) {
        return;
    }

    g_ptr_array_free(*rows, TRUE);
    *rows = NULL;
}

static void free_output_models(GPtrArray **outputs) {
    if (*outputs == NULL) {
        return;
    }

    g_ptr_array_free(*outputs, TRUE);
    *outputs = NULL;
}

static void on_row_changed(gpointer user_data) {
    DcAppController *app = user_data;

    dc_preview_canvas_queue_draw(app->preview);
}

static void on_primary_selected(DcOutputRow *selected_row, gpointer user_data) {
    DcAppController *app = user_data;
    guint i;

    for (i = 0; i < app->rows->len; i++) {
        DcOutputRow *row = g_ptr_array_index(app->rows, i);
        if (row != selected_row) {
            dc_output_row_set_primary(row, FALSE);
        }
    }

    dc_preview_canvas_queue_draw(app->preview);
}

static void clear_rows(DcAppController *app) {
    guint i;

    if (app->rows == NULL) {
        return;
    }

    for (i = 0; i < app->rows->len; i++) {
        DcOutputRow *row = g_ptr_array_index(app->rows, i);
        gtk_widget_destroy(dc_output_row_get_widget(row));
    }

    free_row_list(&app->rows);
    free_output_models(&app->output_models);
    dc_preview_canvas_set_rows(app->preview, NULL);
}

static void reload_outputs(DcAppController *app) {
    GPtrArray *loaded_outputs = NULL;
    char *error_message = NULL;
    guint i;

    clear_rows(app);
    set_status(app, "Refreshing outputs...");

    if (!dc_xrandr_service_load_outputs(app->service, &loaded_outputs, &error_message)) {
        set_status(app, error_message != NULL ? error_message : "Failed to load outputs.");
        g_free(error_message);
        return;
    }

    app->output_models = loaded_outputs;
    app->rows = g_ptr_array_new_with_free_func((GDestroyNotify) dc_output_row_free);

    for (i = 0; i < app->output_models->len; i++) {
        DcDisplayOutput *output = g_ptr_array_index(app->output_models, i);
        DcOutputRow *row = dc_output_row_new(output, on_row_changed, on_primary_selected, app);

        g_ptr_array_add(app->rows, row);
        gtk_box_pack_start(GTK_BOX(dc_display_page_get_content_box(app->display_page)), dc_output_row_get_widget(row), FALSE, FALSE, 0);
    }

    dc_preview_canvas_set_rows(app->preview, app->rows);
    gtk_widget_show_all(app->window);

    if (app->rows->len == 0) {
        set_status(app, "No connected outputs were found.");
    } else {
        set_status(app, "Outputs loaded. Drag displays in the preview to reposition them.");
    }

    refresh_profile_list(app);
    g_free(error_message);
}

static void on_refresh_clicked(GtkButton *button, gpointer user_data) {
    DcAppController *app = user_data;

    (void) button;
    reload_outputs(app);
}

static void apply_extend_mode(DcAppController *app) {
    guint i;
    int next_x = 0;
    gboolean first_enabled = TRUE;

    for (i = 0; i < app->rows->len; i++) {
        DcOutputRow *row = g_ptr_array_index(app->rows, i);
        int x;
        int y;
        int width;
        int height;

        dc_output_row_set_enabled(row, TRUE);
        dc_output_row_set_primary(row, FALSE);
        dc_output_row_set_position(row, next_x, 0);
        dc_output_row_set_rotation(row, RR_Rotate_0);

        if (first_enabled) {
            dc_output_row_set_primary(row, TRUE);
            first_enabled = FALSE;
        }

        if (dc_output_row_get_geometry(row, &x, &y, &width, &height)) {
            next_x += width;
        }
    }

    dc_preview_canvas_queue_draw(app->preview);
    set_status(app, "Extend mode prepared. Review and press Apply.");
}

static void apply_mirror_mode(DcAppController *app) {
    RRMode common_mode = None;
    guint i;

    for (i = 0; i < app->rows->len; i++) {
        DcOutputRow *row = g_ptr_array_index(app->rows, i);
        RRMode mode = dc_output_row_get_selected_mode(row);

        if (mode != None) {
            common_mode = mode;
            break;
        }
    }

    for (i = 0; i < app->rows->len; i++) {
        DcOutputRow *row = g_ptr_array_index(app->rows, i);

        dc_output_row_set_enabled(row, TRUE);
        dc_output_row_set_primary(row, i == 0);
        dc_output_row_set_position(row, 0, 0);
        dc_output_row_set_rotation(row, RR_Rotate_0);
        if (common_mode != None) {
            dc_output_row_set_mode(row, common_mode);
        }
    }

    dc_preview_canvas_queue_draw(app->preview);
    set_status(app, "Mirror mode prepared. Review and press Apply.");
}

static void apply_internal_only_mode(DcAppController *app) {
    guint i;
    DcOutputRow *fallback_row = app->rows->len > 0 ? g_ptr_array_index(app->rows, 0) : NULL;

    for (i = 0; i < app->rows->len; i++) {
        DcOutputRow *row = g_ptr_array_index(app->rows, i);
        gboolean enabled = is_internal_output_name(dc_output_row_get_output(row)->name);

        dc_output_row_set_enabled(row, enabled);
        dc_output_row_set_primary(row, FALSE);
        if (enabled) {
            dc_output_row_set_position(row, 0, 0);
            dc_output_row_set_primary(row, TRUE);
        }
    }

    if (fallback_row != NULL) {
        gboolean any_enabled = FALSE;

        for (i = 0; i < app->rows->len; i++) {
            if (dc_output_row_is_enabled(g_ptr_array_index(app->rows, i))) {
                any_enabled = TRUE;
                break;
            }
        }

        if (!any_enabled) {
            dc_output_row_set_enabled(fallback_row, TRUE);
            dc_output_row_set_position(fallback_row, 0, 0);
            dc_output_row_set_primary(fallback_row, TRUE);
        }
    }

    dc_preview_canvas_queue_draw(app->preview);
    set_status(app, "Internal-only mode prepared. Review and press Apply.");
}

static void apply_external_only_mode(DcAppController *app) {
    guint i;
    DcOutputRow *first_external = NULL;

    for (i = 0; i < app->rows->len; i++) {
        DcOutputRow *row = g_ptr_array_index(app->rows, i);
        gboolean enabled = !is_internal_output_name(dc_output_row_get_output(row)->name);

        if (enabled && first_external == NULL) {
            first_external = row;
        }

        dc_output_row_set_enabled(row, enabled);
        dc_output_row_set_primary(row, FALSE);
        if (enabled) {
            dc_output_row_set_position(row, 0, 0);
        }
    }

    if (first_external == NULL && app->rows->len > 0) {
        first_external = g_ptr_array_index(app->rows, 0);
        dc_output_row_set_enabled(first_external, TRUE);
        dc_output_row_set_position(first_external, 0, 0);
    }

    if (first_external != NULL) {
        dc_output_row_set_primary(first_external, TRUE);
    }

    dc_preview_canvas_queue_draw(app->preview);
    set_status(app, "External-only mode prepared. Review and press Apply.");
}

static void on_extend_clicked(GtkButton *button, gpointer user_data) {
    (void) button;
    apply_extend_mode(user_data);
}

static void on_mirror_clicked(GtkButton *button, gpointer user_data) {
    (void) button;
    apply_mirror_mode(user_data);
}

static void on_internal_clicked(GtkButton *button, gpointer user_data) {
    (void) button;
    apply_internal_only_mode(user_data);
}

static void on_external_clicked(GtkButton *button, gpointer user_data) {
    (void) button;
    apply_external_only_mode(user_data);
}

static void on_save_profile_clicked(GtkButton *button, gpointer user_data) {
    DcAppController *app = user_data;
    const char *profile_name = gtk_entry_get_text(GTK_ENTRY(dc_display_page_get_profile_entry(app->display_page)));
    GPtrArray *configs;
    char *error_message = NULL;

    (void) button;

    configs = collect_current_configs(app);
    if (dc_profile_service_save(profile_name, configs, &error_message)) {
        set_status(app, "Profile saved successfully.");
        refresh_profile_list(app);
    } else {
        set_status(app, error_message != NULL ? error_message : "Failed to save profile.");
    }

    g_free(error_message);
    g_ptr_array_free(configs, TRUE);
}

static void on_load_profile_clicked(GtkButton *button, gpointer user_data) {
    DcAppController *app = user_data;
    char *profile_name = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(dc_display_page_get_profile_combo(app->display_page)));
    GPtrArray *configs = NULL;
    char *error_message = NULL;

    (void) button;

    if (profile_name == NULL) {
        set_status(app, "Choose a saved profile first.");
        return;
    }

    if (dc_profile_service_load(profile_name, &configs, &error_message)) {
        apply_configs_to_rows(app, configs);
        set_status(app, "Profile loaded into the form. Review and press Apply.");
        g_ptr_array_free(configs, TRUE);
    } else {
        set_status(app, error_message != NULL ? error_message : "Failed to load profile.");
    }

    g_free(profile_name);
    g_free(error_message);
}

static gboolean on_revert_timeout(gpointer user_data) {
    DcRevertDialogData *data = user_data;
    char buffer[160];

    data->remaining_seconds--;
    g_snprintf(buffer,
               sizeof(buffer),
               "Keep these display settings? They will revert automatically in %d seconds.",
               data->remaining_seconds);
    gtk_label_set_text(GTK_LABEL(data->label), buffer);

    if (data->remaining_seconds <= 0) {
        gtk_dialog_response(GTK_DIALOG(data->dialog), GTK_RESPONSE_CANCEL);
        return G_SOURCE_REMOVE;
    }

    return G_SOURCE_CONTINUE;
}

static gboolean confirm_or_revert(DcAppController *app, GPtrArray *previous_configs) {
    GtkWidget *dialog;
    GtkWidget *content_area;
    GtkWidget *label;
    DcRevertDialogData data;
    gint response;
    char *error_message = NULL;

    dialog = gtk_dialog_new_with_buttons("Confirm Display Settings",
                                         GTK_WINDOW(app->window),
                                         GTK_DIALOG_MODAL,
                                         "Keep",
                                         GTK_RESPONSE_OK,
                                         "Revert",
                                         GTK_RESPONSE_CANCEL,
                                         NULL);
    content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    label = gtk_label_new(NULL);

    gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
    gtk_container_add(GTK_CONTAINER(content_area), label);

    data.dialog = dialog;
    data.label = label;
    data.remaining_seconds = DC_REVERT_TIMEOUT_SECONDS;
    on_revert_timeout(&data);
    data.timeout_id = g_timeout_add_seconds(1, on_revert_timeout, &data);

    gtk_widget_show_all(dialog);
    response = gtk_dialog_run(GTK_DIALOG(dialog));

    if (data.timeout_id != 0) {
        g_source_remove(data.timeout_id);
    }

    gtk_widget_destroy(dialog);

    if (response == GTK_RESPONSE_OK) {
        set_status(app, "Display settings applied successfully.");
        return TRUE;
    }

    if (!dc_xrandr_service_apply_configs(app->service, previous_configs, &error_message)) {
        set_status(app, error_message != NULL ? error_message : "Failed to revert display settings.");
        g_free(error_message);
        return FALSE;
    }

    reload_outputs(app);
    set_status(app, "Display settings were reverted.");
    g_free(error_message);
    return FALSE;
}

static void on_apply_clicked(GtkButton *button, gpointer user_data) {
    DcAppController *app = user_data;
    GPtrArray *configs;
    GPtrArray *previous_configs;
    char *error_message = NULL;
    guint i;
    gboolean success;

    (void) button;

    previous_configs = g_ptr_array_new_with_free_func((GDestroyNotify) dc_display_config_free);
    for (i = 0; i < app->output_models->len; i++) {
        DcDisplayOutput *output = g_ptr_array_index(app->output_models, i);
        DcDisplayConfig *config = dc_display_config_new();

        config->output_id = output->output_id;
        config->output_name = g_strdup(output->name);
        config->enabled = output->enabled;
        config->primary = output->primary;
        config->mode = output->current_mode;
        config->rotation = output->current_rotation;
        config->x = output->x;
        config->y = output->y;
        g_ptr_array_add(previous_configs, config);
    }

    configs = collect_current_configs(app);

    success = dc_xrandr_service_apply_configs(app->service, configs, &error_message);
    if (success) {
        reload_outputs(app);
        confirm_or_revert(app, previous_configs);
    } else {
        set_status(app, error_message != NULL ? error_message : "Failed to apply display settings.");
    }

    g_free(error_message);
    g_ptr_array_free(previous_configs, TRUE);
    g_ptr_array_free(configs, TRUE);
}

static void activate(GtkApplication *gtk_app, gpointer user_data) {
    DcAppController *app = user_data;
    GtkWidget *root_box;
    GtkWidget *main_box;
    GtkWidget *sidebar_frame;
    GtkWidget *sidebar_box;
    GtkWidget *stack_sidebar;
    GtkWidget *stack_frame;

    app->preview = dc_preview_canvas_new();
    app->display_page = dc_display_page_new(dc_preview_canvas_get_widget(app->preview));
    app->display_edit_page = dc_display_edit_page_new();
    app->window_manager_page = dc_window_manager_page_new();
    app->compositor_page = dc_compositor_page_new();
    app->window = gtk_application_window_new(gtk_app);
    gtk_window_set_title(GTK_WINDOW(app->window), "Display Settings");
    gtk_window_set_default_size(GTK_WINDOW(app->window), 980, 760);
    gtk_container_set_border_width(GTK_CONTAINER(app->window), 12);
    gtk_widget_set_name(app->window, "display-core-window");
    add_css_class(app->window, "app-window");
    gtk_widget_set_app_paintable(app->window, TRUE);

    {
        GdkScreen *screen = gtk_widget_get_screen(app->window);
        GdkVisual *visual = gdk_screen_get_rgba_visual(screen);

        if (visual != NULL && gdk_screen_is_composited(screen)) {
            gtk_widget_set_visual(app->window, visual);
        }
    }

    root_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    main_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 16);
    sidebar_frame = gtk_frame_new(NULL);
    sidebar_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);
    stack_sidebar = gtk_stack_sidebar_new();
    stack_frame = gtk_frame_new(NULL);
    app->stack = gtk_stack_new();
    gtk_stack_set_transition_type(GTK_STACK(app->stack), GTK_STACK_TRANSITION_TYPE_CROSSFADE);
    gtk_stack_set_transition_duration(GTK_STACK(app->stack), 220);
    gtk_stack_sidebar_set_stack(GTK_STACK_SIDEBAR(stack_sidebar), GTK_STACK(app->stack));

    add_css_class(root_box, "app-shell");
    add_css_class(sidebar_frame, "sidebar-card");
    add_css_class(stack_frame, "content-card");

    gtk_container_add(GTK_CONTAINER(app->window), root_box);
    gtk_container_add(GTK_CONTAINER(sidebar_frame), sidebar_box);
    gtk_container_add(GTK_CONTAINER(stack_frame), app->stack);

    gtk_frame_set_shadow_type(GTK_FRAME(sidebar_frame), GTK_SHADOW_NONE);
    gtk_container_set_border_width(GTK_CONTAINER(sidebar_frame), 0);
    gtk_frame_set_shadow_type(GTK_FRAME(stack_frame), GTK_SHADOW_NONE);
    gtk_container_set_border_width(GTK_CONTAINER(stack_frame), 0);

    gtk_box_pack_start(GTK_BOX(root_box), main_box, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(main_box), sidebar_frame, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(main_box), stack_frame, TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(sidebar_box), stack_sidebar, TRUE, TRUE, 0);

    gtk_stack_add_titled(GTK_STACK(app->stack),
                         dc_display_page_get_widget(app->display_page),
                         "display",
                         "Display");
    gtk_stack_add_titled(GTK_STACK(app->stack),
                         dc_display_edit_page_get_widget(app->display_edit_page),
                         "display-edit",
                         "Display Edit");
    gtk_stack_add_titled(GTK_STACK(app->stack),
                         dc_window_manager_page_get_widget(app->window_manager_page),
                         "window-manager",
                         "Window Manager");
    gtk_stack_add_titled(GTK_STACK(app->stack),
                         dc_compositor_page_get_widget(app->compositor_page),
                         "compositor",
                         "Compositor");

    g_signal_connect(dc_display_page_get_refresh_button(app->display_page), "clicked", G_CALLBACK(on_refresh_clicked), app);
    g_signal_connect(dc_display_page_get_apply_button(app->display_page), "clicked", G_CALLBACK(on_apply_clicked), app);
    g_signal_connect(dc_display_page_get_extend_button(app->display_page), "clicked", G_CALLBACK(on_extend_clicked), app);
    g_signal_connect(dc_display_page_get_mirror_button(app->display_page), "clicked", G_CALLBACK(on_mirror_clicked), app);
    g_signal_connect(dc_display_page_get_internal_button(app->display_page), "clicked", G_CALLBACK(on_internal_clicked), app);
    g_signal_connect(dc_display_page_get_external_button(app->display_page), "clicked", G_CALLBACK(on_external_clicked), app);
    g_signal_connect(dc_display_page_get_save_profile_button(app->display_page), "clicked", G_CALLBACK(on_save_profile_clicked), app);
    g_signal_connect(dc_display_page_get_load_profile_button(app->display_page), "clicked", G_CALLBACK(on_load_profile_clicked), app);
    gtk_widget_show_all(app->window);
    reload_outputs(app);
    on_display_edit_load(app);
    configure_display_edit_capabilities(app);
    on_compositor_load_clicked(NULL, app);
    connect_display_edit_autosave_signals(app);
    connect_compositor_autosave_signals(app);
    app->display_edit_refresh_timeout_id = g_timeout_add_seconds(60, refresh_display_edit_runtime, app);
}

static DcAppController *dc_app_controller_new(char **error_message) {
    DcAppController *app = g_new0(DcAppController, 1);

    app->service = dc_xrandr_service_new(error_message);
    if (app->service == NULL) {
        g_free(app);
        return NULL;
    }

    install_app_css();
    app->gtk_app = gtk_application_new("com.displaycore.settings", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app->gtk_app, "activate", G_CALLBACK(activate), app);
    return app;
}

static void dc_app_controller_free(DcAppController *app) {
    if (app == NULL) {
        return;
    }

    clear_rows(app);

    if (app->display_page != NULL) {
        dc_display_page_free(app->display_page);
    }
    if (app->display_edit_page != NULL) {
        dc_display_edit_page_free(app->display_edit_page);
    }
    if (app->window_manager_page != NULL) {
        dc_window_manager_page_free(app->window_manager_page);
    }
    if (app->compositor_page != NULL) {
        dc_compositor_page_free(app->compositor_page);
    }
    if (app->preview != NULL) {
        dc_preview_canvas_free(app->preview);
    }
    if (app->compositor_autosave_timeout_id != 0) {
        g_source_remove(app->compositor_autosave_timeout_id);
    }
    if (app->display_edit_autosave_timeout_id != 0) {
        g_source_remove(app->display_edit_autosave_timeout_id);
    }
    if (app->display_edit_refresh_timeout_id != 0) {
        g_source_remove(app->display_edit_refresh_timeout_id);
    }

    if (app->gtk_app != NULL) {
        g_object_unref(app->gtk_app);
    }

    if (app->service != NULL) {
        dc_xrandr_service_free(app->service);
    }

    g_free(app);
}

int dc_app_controller_run(int argc, char **argv) {
    DcAppController *app;
    char *error_message = NULL;
    int status;

    app = dc_app_controller_new(&error_message);
    if (app == NULL) {
        g_printerr("%s\n", error_message != NULL ? error_message : "Failed to initialize the app.");
        g_free(error_message);
        return 1;
    }

    status = g_application_run(G_APPLICATION(app->gtk_app), argc, argv);
    dc_app_controller_free(app);
    g_free(error_message);
    return status;
}
