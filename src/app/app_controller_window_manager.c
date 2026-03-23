#include "app_controller_internal.h"

#include "services/window_manager_service.h"

static void set_window_manager_rules_text(DcAppController *app, const char *text) {
    GtkTextBuffer *buffer;

    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(dc_window_manager_page_get_rules_text_view(app->window_manager_page)));
    gtk_text_buffer_set_text(buffer, text != NULL ? text : "", -1);
}

static void refresh_window_manager_rules(DcAppController *app) {
    char *rules_output = NULL;
    char *error_message = NULL;

    if (!dc_window_manager_list_rules(&rules_output, &error_message)) {
        set_window_manager_rules_text(app, "Unable to query PoisonBlade rules on this session.");
        if (error_message != NULL) {
            dc_app_set_status(app, error_message);
        }
        g_free(error_message);
        return;
    }

    if (rules_output == NULL || *rules_output == '\0') {
        set_window_manager_rules_text(app, "No rules are currently registered.");
    } else {
        set_window_manager_rules_text(app, rules_output);
    }

    dc_app_set_status(app, "PoisonBlade rules list refreshed.");
    g_free(rules_output);
}

static void apply_window_manager_config_to_ui(DcAppController *app, const DcWindowManagerConfig *config) {
    gtk_switch_set_active(GTK_SWITCH(dc_window_manager_page_get_floating_mode_switch(app->window_manager_page)), config->floating_mode);
    gtk_range_set_value(GTK_RANGE(dc_window_manager_page_get_snap_threshold_scale(app->window_manager_page)), config->snap_threshold);
    gtk_switch_set_active(GTK_SWITCH(dc_window_manager_page_get_snap_show_preview_switch(app->window_manager_page)), config->snap_show_preview);
    gtk_combo_box_set_active_id(GTK_COMBO_BOX(dc_window_manager_page_get_layout_combo(app->window_manager_page)), config->desktop_layout);
    gtk_range_set_value(GTK_RANGE(dc_window_manager_page_get_border_width_scale(app->window_manager_page)), config->border_width);
    gtk_entry_set_text(GTK_ENTRY(dc_window_manager_page_get_focused_border_color_entry(app->window_manager_page)),
                       config->focused_border_color != NULL ? config->focused_border_color : "");
    gtk_entry_set_text(GTK_ENTRY(dc_window_manager_page_get_normal_border_color_entry(app->window_manager_page)),
                       config->normal_border_color != NULL ? config->normal_border_color : "");
    gtk_range_set_value(GTK_RANGE(dc_window_manager_page_get_window_gap_scale(app->window_manager_page)), config->window_gap);
    gtk_range_set_value(GTK_RANGE(dc_window_manager_page_get_top_padding_scale(app->window_manager_page)), config->top_padding);
    gtk_range_set_value(GTK_RANGE(dc_window_manager_page_get_bottom_padding_scale(app->window_manager_page)), config->bottom_padding);
    gtk_switch_set_active(GTK_SWITCH(dc_window_manager_page_get_focus_opacity_switch(app->window_manager_page)), config->focus_opacity);
    gtk_range_set_value(GTK_RANGE(dc_window_manager_page_get_inactive_opacity_scale(app->window_manager_page)), config->inactive_opacity);
    gtk_range_set_value(GTK_RANGE(dc_window_manager_page_get_active_opacity_scale(app->window_manager_page)), config->active_opacity);
}

static DcWindowManagerConfig *collect_window_manager_config_from_ui(DcAppController *app) {
    DcWindowManagerConfig *config;
    const char *layout;

    config = dc_window_manager_config_new();
    config->floating_mode = gtk_switch_get_active(GTK_SWITCH(dc_window_manager_page_get_floating_mode_switch(app->window_manager_page)));
    config->snap_threshold = (int) gtk_range_get_value(GTK_RANGE(dc_window_manager_page_get_snap_threshold_scale(app->window_manager_page)));
    config->snap_show_preview = gtk_switch_get_active(GTK_SWITCH(dc_window_manager_page_get_snap_show_preview_switch(app->window_manager_page)));
    layout = gtk_combo_box_get_active_id(GTK_COMBO_BOX(dc_window_manager_page_get_layout_combo(app->window_manager_page)));
    if (layout != NULL) {
        g_free(config->desktop_layout);
        config->desktop_layout = g_strdup(layout);
    }
    config->border_width = (int) gtk_range_get_value(GTK_RANGE(dc_window_manager_page_get_border_width_scale(app->window_manager_page)));
    g_free(config->focused_border_color);
    config->focused_border_color = g_strdup(gtk_entry_get_text(GTK_ENTRY(dc_window_manager_page_get_focused_border_color_entry(app->window_manager_page))));
    g_free(config->normal_border_color);
    config->normal_border_color = g_strdup(gtk_entry_get_text(GTK_ENTRY(dc_window_manager_page_get_normal_border_color_entry(app->window_manager_page))));
    config->window_gap = (int) gtk_range_get_value(GTK_RANGE(dc_window_manager_page_get_window_gap_scale(app->window_manager_page)));
    config->top_padding = (int) gtk_range_get_value(GTK_RANGE(dc_window_manager_page_get_top_padding_scale(app->window_manager_page)));
    config->bottom_padding = (int) gtk_range_get_value(GTK_RANGE(dc_window_manager_page_get_bottom_padding_scale(app->window_manager_page)));
    config->focus_opacity = gtk_switch_get_active(GTK_SWITCH(dc_window_manager_page_get_focus_opacity_switch(app->window_manager_page)));
    config->inactive_opacity = gtk_range_get_value(GTK_RANGE(dc_window_manager_page_get_inactive_opacity_scale(app->window_manager_page)));
    config->active_opacity = gtk_range_get_value(GTK_RANGE(dc_window_manager_page_get_active_opacity_scale(app->window_manager_page)));
    return config;
}

void dc_app_window_manager_load(DcAppController *app) {
    DcWindowManagerConfig *config = NULL;
    char *error_message = NULL;

    app->suppress_window_manager_autosave = TRUE;
    if (dc_window_manager_config_load(&config, &error_message)) {
        apply_window_manager_config_to_ui(app, config);
        dc_window_manager_apply_config(config, NULL);
        refresh_window_manager_rules(app);
        dc_window_manager_config_free(config);
        app->suppress_window_manager_autosave = FALSE;
        return;
    }

    app->suppress_window_manager_autosave = FALSE;
    g_warning("%s", error_message != NULL ? error_message : "Failed to load window manager config.");
    g_free(error_message);
}

static void on_window_manager_save(DcAppController *app) {
    DcWindowManagerConfig *config;
    char *error_message = NULL;

    config = collect_window_manager_config_from_ui(app);
    if (!dc_window_manager_config_save(config, &error_message)) {
        g_warning("%s", error_message != NULL ? error_message : "Failed to save window manager config.");
        g_free(error_message);
        dc_window_manager_config_free(config);
        return;
    }

    g_free(error_message);
    error_message = NULL;
    if (!dc_window_manager_apply_config(config, &error_message)) {
        g_warning("%s", error_message != NULL ? error_message : "Failed to apply PoisonBlade settings.");
    }

    g_free(error_message);
    dc_window_manager_config_free(config);
}

static void on_window_manager_add_desktop_clicked(GtkButton *button, gpointer user_data) {
    DcAppController *app = user_data;
    char *error_message = NULL;
    const char *name;

    (void) button;
    name = gtk_entry_get_text(GTK_ENTRY(dc_window_manager_page_get_add_desktop_entry(app->window_manager_page)));
    if (!dc_window_manager_add_desktop(name, &error_message)) {
        dc_app_set_status(app, error_message != NULL ? error_message : "Failed to create a new desktop.");
        g_free(error_message);
        return;
    }

    dc_app_set_status(app, "PoisonBlade created a new desktop.");
}

static void on_window_manager_rename_desktop_clicked(GtkButton *button, gpointer user_data) {
    DcAppController *app = user_data;
    char *error_message = NULL;
    const char *name;

    (void) button;
    name = gtk_entry_get_text(GTK_ENTRY(dc_window_manager_page_get_rename_desktop_entry(app->window_manager_page)));
    if (!dc_window_manager_rename_desktop(name, &error_message)) {
        dc_app_set_status(app, error_message != NULL ? error_message : "Failed to rename the current desktop.");
        g_free(error_message);
        return;
    }

    dc_app_set_status(app, "PoisonBlade renamed the current desktop.");
}

static void on_window_manager_remove_desktop_clicked(GtkButton *button, gpointer user_data) {
    DcAppController *app = user_data;
    char *error_message = NULL;

    (void) button;
    if (!dc_window_manager_remove_current_desktop(&error_message)) {
        dc_app_set_status(app, error_message != NULL ? error_message : "Failed to remove the current desktop.");
        g_free(error_message);
        return;
    }

    dc_app_set_status(app, "PoisonBlade removed the current desktop.");
}

static void on_window_manager_add_rule_clicked(GtkButton *button, gpointer user_data) {
    DcAppController *app = user_data;
    char *error_message = NULL;
    const char *app_name;
    const char *desktop;
    const char *state;

    (void) button;
    app_name = gtk_entry_get_text(GTK_ENTRY(dc_window_manager_page_get_rule_app_entry(app->window_manager_page)));
    desktop = gtk_entry_get_text(GTK_ENTRY(dc_window_manager_page_get_rule_desktop_entry(app->window_manager_page)));
    state = gtk_combo_box_get_active_id(GTK_COMBO_BOX(dc_window_manager_page_get_rule_state_combo(app->window_manager_page)));
    if (!dc_window_manager_add_rule(app_name, desktop, state, &error_message)) {
        dc_app_set_status(app, error_message != NULL ? error_message : "Failed to add the requested rule.");
        g_free(error_message);
        return;
    }

    refresh_window_manager_rules(app);
    dc_app_set_status(app, "PoisonBlade rule added successfully.");
}

static void on_window_manager_remove_rule_clicked(GtkButton *button, gpointer user_data) {
    DcAppController *app = user_data;
    char *error_message = NULL;
    const char *app_name;

    (void) button;
    app_name = gtk_entry_get_text(GTK_ENTRY(dc_window_manager_page_get_remove_rule_entry(app->window_manager_page)));
    if (!dc_window_manager_remove_rule(app_name, &error_message)) {
        dc_app_set_status(app, error_message != NULL ? error_message : "Failed to remove the requested rule.");
        g_free(error_message);
        return;
    }

    refresh_window_manager_rules(app);
    dc_app_set_status(app, "PoisonBlade rule removed successfully.");
}

static void on_window_manager_refresh_rules_clicked(GtkButton *button, gpointer user_data) {
    (void) button;
    refresh_window_manager_rules(user_data);
}

static gboolean flush_window_manager_autosave(gpointer user_data) {
    DcAppController *app = user_data;

    app->window_manager_autosave_timeout_id = 0;
    on_window_manager_save(app);
    return G_SOURCE_REMOVE;
}

static void queue_window_manager_autosave(gpointer user_data) {
    DcAppController *app = user_data;

    if (app->suppress_window_manager_autosave) {
        return;
    }

    if (app->window_manager_autosave_timeout_id != 0) {
        g_source_remove(app->window_manager_autosave_timeout_id);
    }

    app->window_manager_autosave_timeout_id = g_timeout_add(250, flush_window_manager_autosave, app);
}

static void on_window_manager_widget_changed(GtkWidget *widget, gpointer user_data) {
    (void) widget;
    queue_window_manager_autosave(user_data);
}

static void on_window_manager_switch_active_changed(GObject *object, GParamSpec *pspec, gpointer user_data) {
    (void) object;
    (void) pspec;
    queue_window_manager_autosave(user_data);
}

void dc_app_window_manager_connect_signals(DcAppController *app) {
    g_signal_connect(dc_window_manager_page_get_floating_mode_switch(app->window_manager_page), "notify::active", G_CALLBACK(on_window_manager_switch_active_changed), app);
    g_signal_connect(dc_window_manager_page_get_snap_threshold_scale(app->window_manager_page), "value-changed", G_CALLBACK(on_window_manager_widget_changed), app);
    g_signal_connect(dc_window_manager_page_get_snap_show_preview_switch(app->window_manager_page), "notify::active", G_CALLBACK(on_window_manager_switch_active_changed), app);
    g_signal_connect(dc_window_manager_page_get_layout_combo(app->window_manager_page), "changed", G_CALLBACK(on_window_manager_widget_changed), app);
    g_signal_connect(dc_window_manager_page_get_border_width_scale(app->window_manager_page), "value-changed", G_CALLBACK(on_window_manager_widget_changed), app);
    g_signal_connect(dc_window_manager_page_get_focused_border_color_entry(app->window_manager_page), "changed", G_CALLBACK(on_window_manager_widget_changed), app);
    g_signal_connect(dc_window_manager_page_get_normal_border_color_entry(app->window_manager_page), "changed", G_CALLBACK(on_window_manager_widget_changed), app);
    g_signal_connect(dc_window_manager_page_get_window_gap_scale(app->window_manager_page), "value-changed", G_CALLBACK(on_window_manager_widget_changed), app);
    g_signal_connect(dc_window_manager_page_get_top_padding_scale(app->window_manager_page), "value-changed", G_CALLBACK(on_window_manager_widget_changed), app);
    g_signal_connect(dc_window_manager_page_get_bottom_padding_scale(app->window_manager_page), "value-changed", G_CALLBACK(on_window_manager_widget_changed), app);
    g_signal_connect(dc_window_manager_page_get_focus_opacity_switch(app->window_manager_page), "notify::active", G_CALLBACK(on_window_manager_switch_active_changed), app);
    g_signal_connect(dc_window_manager_page_get_inactive_opacity_scale(app->window_manager_page), "value-changed", G_CALLBACK(on_window_manager_widget_changed), app);
    g_signal_connect(dc_window_manager_page_get_active_opacity_scale(app->window_manager_page), "value-changed", G_CALLBACK(on_window_manager_widget_changed), app);
    g_signal_connect(dc_window_manager_page_get_add_desktop_button(app->window_manager_page), "clicked", G_CALLBACK(on_window_manager_add_desktop_clicked), app);
    g_signal_connect(dc_window_manager_page_get_rename_desktop_button(app->window_manager_page), "clicked", G_CALLBACK(on_window_manager_rename_desktop_clicked), app);
    g_signal_connect(dc_window_manager_page_get_remove_desktop_button(app->window_manager_page), "clicked", G_CALLBACK(on_window_manager_remove_desktop_clicked), app);
    g_signal_connect(dc_window_manager_page_get_add_rule_button(app->window_manager_page), "clicked", G_CALLBACK(on_window_manager_add_rule_clicked), app);
    g_signal_connect(dc_window_manager_page_get_remove_rule_button(app->window_manager_page), "clicked", G_CALLBACK(on_window_manager_remove_rule_clicked), app);
    g_signal_connect(dc_window_manager_page_get_refresh_rules_button(app->window_manager_page), "clicked", G_CALLBACK(on_window_manager_refresh_rules_clicked), app);
}
