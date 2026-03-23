#include "app_controller_internal.h"

#include "services/theme_service.h"

static void fill_combo_with_values(GtkComboBoxText *combo, GStrv values) {
    guint i;

    gtk_combo_box_text_remove_all(combo);
    if (values == NULL) {
        return;
    }

    for (i = 0; values[i] != NULL; i++) {
        gtk_combo_box_text_append(combo, values[i], values[i]);
    }

    if (values[0] != NULL) {
        gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0);
    }
}

static void ensure_combo_has_active_id(GtkComboBoxText *combo, const char *id) {
    if (id == NULL || *id == '\0') {
        return;
    }

    if (!gtk_combo_box_set_active_id(GTK_COMBO_BOX(combo), id)) {
        gtk_combo_box_text_append(combo, id, id);
        gtk_combo_box_set_active_id(GTK_COMBO_BOX(combo), id);
    }
}

static void populate_theme_choices(DcAppController *app) {
    GStrv gtk_themes = NULL;
    GStrv icon_themes = NULL;
    GStrv cursor_themes = NULL;
    GStrv fonts = NULL;
    char *error_message = NULL;

    if (dc_theme_service_list_gtk_themes(&gtk_themes, &error_message)) {
        fill_combo_with_values(GTK_COMBO_BOX_TEXT(dc_themes_page_get_theme_combo(app->themes_page)), gtk_themes);
    }
    g_free(error_message);
    error_message = NULL;

    if (dc_theme_service_list_icon_themes(&icon_themes, &error_message)) {
        fill_combo_with_values(GTK_COMBO_BOX_TEXT(dc_themes_page_get_icons_combo(app->themes_page)), icon_themes);
    }
    g_free(error_message);
    error_message = NULL;

    if (dc_theme_service_list_cursor_themes(&cursor_themes, &error_message)) {
        fill_combo_with_values(GTK_COMBO_BOX_TEXT(dc_themes_page_get_cursor_combo(app->themes_page)), cursor_themes);
    }
    g_free(error_message);
    error_message = NULL;

    if (dc_theme_service_list_fonts(&fonts, &error_message)) {
        fill_combo_with_values(GTK_COMBO_BOX_TEXT(dc_themes_page_get_font_combo(app->themes_page)), fonts);
    }
    g_free(error_message);

    g_strfreev(gtk_themes);
    g_strfreev(icon_themes);
    g_strfreev(cursor_themes);
    g_strfreev(fonts);
}

static void apply_theme_config_to_ui(DcAppController *app, const DcThemeConfig *config) {
    ensure_combo_has_active_id(GTK_COMBO_BOX_TEXT(dc_themes_page_get_theme_combo(app->themes_page)), config->gtk_theme);
    ensure_combo_has_active_id(GTK_COMBO_BOX_TEXT(dc_themes_page_get_icons_combo(app->themes_page)), config->icon_theme);
    ensure_combo_has_active_id(GTK_COMBO_BOX_TEXT(dc_themes_page_get_cursor_combo(app->themes_page)), config->cursor_theme);
    ensure_combo_has_active_id(GTK_COMBO_BOX_TEXT(dc_themes_page_get_font_combo(app->themes_page)), config->font_name);
    ensure_combo_has_active_id(GTK_COMBO_BOX_TEXT(dc_themes_page_get_mode_combo(app->themes_page)), config->interface_mode);
    ensure_combo_has_active_id(GTK_COMBO_BOX_TEXT(dc_themes_page_get_mono_font_combo(app->themes_page)), config->monospace_font);
    gtk_range_set_value(GTK_RANGE(dc_themes_page_get_cursor_size_scale(app->themes_page)), config->cursor_size);
    gtk_range_set_value(GTK_RANGE(dc_themes_page_get_text_scale(app->themes_page)), config->text_scale);
}

static DcThemeConfig *collect_theme_config_from_ui(DcAppController *app) {
    DcThemeConfig *config;
    const char *active_id;

    config = dc_theme_config_new();

    active_id = gtk_combo_box_get_active_id(GTK_COMBO_BOX(dc_themes_page_get_theme_combo(app->themes_page)));
    if (active_id != NULL) {
        g_free(config->gtk_theme);
        config->gtk_theme = g_strdup(active_id);
    }

    active_id = gtk_combo_box_get_active_id(GTK_COMBO_BOX(dc_themes_page_get_icons_combo(app->themes_page)));
    if (active_id != NULL) {
        g_free(config->icon_theme);
        config->icon_theme = g_strdup(active_id);
    }

    active_id = gtk_combo_box_get_active_id(GTK_COMBO_BOX(dc_themes_page_get_cursor_combo(app->themes_page)));
    if (active_id != NULL) {
        g_free(config->cursor_theme);
        config->cursor_theme = g_strdup(active_id);
    }

    active_id = gtk_combo_box_get_active_id(GTK_COMBO_BOX(dc_themes_page_get_font_combo(app->themes_page)));
    if (active_id != NULL) {
        g_free(config->font_name);
        config->font_name = g_strdup(active_id);
    }

    active_id = gtk_combo_box_get_active_id(GTK_COMBO_BOX(dc_themes_page_get_mode_combo(app->themes_page)));
    if (active_id != NULL) {
        g_free(config->interface_mode);
        config->interface_mode = g_strdup(active_id);
    }

    active_id = gtk_combo_box_get_active_id(GTK_COMBO_BOX(dc_themes_page_get_mono_font_combo(app->themes_page)));
    if (active_id != NULL) {
        g_free(config->monospace_font);
        config->monospace_font = g_strdup(active_id);
    }

    config->cursor_size = (int) gtk_range_get_value(GTK_RANGE(dc_themes_page_get_cursor_size_scale(app->themes_page)));
    config->text_scale = gtk_range_get_value(GTK_RANGE(dc_themes_page_get_text_scale(app->themes_page)));
    return config;
}

void dc_app_themes_load(DcAppController *app) {
    DcThemeConfig *config = NULL;
    char *error_message = NULL;

    app->suppress_themes_autosave = TRUE;
    populate_theme_choices(app);

    if (dc_theme_config_load(&config, &error_message)) {
        apply_theme_config_to_ui(app, config);
        dc_theme_config_free(config);
        gtk_widget_set_sensitive(dc_themes_page_get_mono_font_combo(app->themes_page), FALSE);
        gtk_widget_set_tooltip_text(dc_themes_page_get_mono_font_combo(app->themes_page),
                                    "Monospace font wiring is not exposed by the current theme daemon yet.");
        app->suppress_themes_autosave = FALSE;
        return;
    }

    app->suppress_themes_autosave = FALSE;
    g_warning("%s", error_message != NULL ? error_message : "Failed to load theme daemon config.");
    g_free(error_message);
}

static void on_themes_save(DcAppController *app) {
    DcThemeConfig *config;
    char *error_message = NULL;

    config = collect_theme_config_from_ui(app);
    if (!dc_theme_config_apply(config, &error_message)) {
        dc_app_set_status(app, error_message != NULL ? error_message : "Failed to apply theme settings.");
        g_free(error_message);
        dc_theme_config_free(config);
        return;
    }

    dc_app_set_status(app, "Theme settings applied.");
    dc_theme_config_free(config);
}

static gboolean flush_themes_autosave(gpointer user_data) {
    DcAppController *app = user_data;

    app->themes_autosave_timeout_id = 0;
    on_themes_save(app);
    return G_SOURCE_REMOVE;
}

static void queue_themes_autosave(gpointer user_data) {
    DcAppController *app = user_data;

    if (app->suppress_themes_autosave) {
        return;
    }

    if (app->themes_autosave_timeout_id != 0) {
        g_source_remove(app->themes_autosave_timeout_id);
    }

    app->themes_autosave_timeout_id = g_timeout_add(200, flush_themes_autosave, app);
}

static void on_themes_widget_changed(GtkWidget *widget, gpointer user_data) {
    (void) widget;
    queue_themes_autosave(user_data);
}

void dc_app_themes_connect_signals(DcAppController *app) {
    g_signal_connect(dc_themes_page_get_mode_combo(app->themes_page), "changed", G_CALLBACK(on_themes_widget_changed), app);
    g_signal_connect(dc_themes_page_get_theme_combo(app->themes_page), "changed", G_CALLBACK(on_themes_widget_changed), app);
    g_signal_connect(dc_themes_page_get_icons_combo(app->themes_page), "changed", G_CALLBACK(on_themes_widget_changed), app);
    g_signal_connect(dc_themes_page_get_cursor_combo(app->themes_page), "changed", G_CALLBACK(on_themes_widget_changed), app);
    g_signal_connect(dc_themes_page_get_font_combo(app->themes_page), "changed", G_CALLBACK(on_themes_widget_changed), app);
    g_signal_connect(dc_themes_page_get_cursor_size_scale(app->themes_page), "value-changed", G_CALLBACK(on_themes_widget_changed), app);
    g_signal_connect(dc_themes_page_get_text_scale(app->themes_page), "value-changed", G_CALLBACK(on_themes_widget_changed), app);
}
