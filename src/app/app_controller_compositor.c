#include "app_controller_internal.h"

#include "services/venom_config_service.h"

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

static void on_compositor_load_clicked(GtkButton *button, gpointer user_data) {
    DcAppController *app = user_data;
    DcVenomConfig *config = NULL;
    char *error_message = NULL;
    char *config_path;

    (void) button;

    config_path = dc_app_resolve_venom_config_path();
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
    config_path = dc_app_resolve_venom_config_path();

    if (!dc_venom_config_save(config_path, config, &error_message)) {
        g_warning("%s", error_message != NULL ? error_message : "Failed to save venom.conf.");
    }

    g_free(error_message);
    g_free(config_path);
    dc_venom_config_free(config);
}

static gboolean flush_compositor_autosave(gpointer user_data) {
    DcAppController *app = user_data;

    app->compositor_autosave_timeout_id = 0;
    on_compositor_save_clicked(NULL, app);
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

static void on_compositor_widget_changed(GtkWidget *widget, gpointer user_data) {
    (void) widget;
    queue_compositor_autosave(user_data);
}

static void on_compositor_switch_active_changed(GObject *object, GParamSpec *pspec, gpointer user_data) {
    (void) object;
    (void) pspec;
    queue_compositor_autosave(user_data);
}

void dc_app_compositor_load(DcAppController *app) {
    on_compositor_load_clicked(NULL, app);
}

void dc_app_compositor_connect_signals(DcAppController *app) {
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
