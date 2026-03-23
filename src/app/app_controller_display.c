#include "app_controller_internal.h"
#include "domain/display_types.h"
#include "services/profile_service.h"

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
        dc_app_set_status(app, error_message != NULL ? error_message : "Failed to load profile list.");
        g_free(error_message);
        return;
    }

    for (i = 0; profile_names[i] != NULL; i++) {
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(dc_display_page_get_profile_combo(app->display_page)),
                                       profile_names[i]);
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

void dc_app_clear_rows(DcAppController *app) {
    guint i;

    if (app->rows == NULL) {
        return;
    }

    for (i = 0; i < app->rows->len; i++) {
        DcOutputRow *row = g_ptr_array_index(app->rows, i);
        GtkWidget *widget = dc_output_row_get_widget(row);

        if (widget != NULL) {
            gtk_widget_destroy(widget);
        }
    }

    free_row_list(&app->rows);
    free_output_models(&app->output_models);
    dc_preview_canvas_set_rows(app->preview, NULL);
}

void dc_app_reload_outputs(DcAppController *app) {
    GPtrArray *loaded_outputs = NULL;
    char *error_message = NULL;
    guint i;

    dc_app_clear_rows(app);
    dc_app_set_status(app, "Refreshing outputs...");

    if (!dc_xrandr_service_load_outputs(app->service, &loaded_outputs, &error_message)) {
        dc_app_set_status(app, error_message != NULL ? error_message : "Failed to load outputs.");
        g_free(error_message);
        return;
    }

    app->output_models = loaded_outputs;
    app->rows = g_ptr_array_new_with_free_func((GDestroyNotify) dc_output_row_free);

    for (i = 0; i < app->output_models->len; i++) {
        DcDisplayOutput *output = g_ptr_array_index(app->output_models, i);
        DcOutputRow *row = dc_output_row_new(output, on_row_changed, on_primary_selected, app);

        g_ptr_array_add(app->rows, row);
        gtk_box_pack_start(GTK_BOX(dc_display_page_get_content_box(app->display_page)),
                           dc_output_row_get_widget(row),
                           FALSE,
                           FALSE,
                           0);
    }

    dc_preview_canvas_set_rows(app->preview, app->rows);
    gtk_widget_show_all(app->window);

    if (app->rows->len == 0) {
        dc_app_set_status(app, "No connected outputs were found.");
    } else {
        dc_app_set_status(app, "Outputs loaded. Drag displays in the preview to reposition them.");
    }

    refresh_profile_list(app);
    g_free(error_message);
}

static void on_refresh_clicked(GtkButton *button, gpointer user_data) {
    (void) button;
    dc_app_reload_outputs(user_data);
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
    dc_app_set_status(app, "Extend mode prepared. Review and press Apply.");
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
    dc_app_set_status(app, "Mirror mode prepared. Review and press Apply.");
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
    dc_app_set_status(app, "Internal-only mode prepared. Review and press Apply.");
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
    dc_app_set_status(app, "External-only mode prepared. Review and press Apply.");
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
        dc_app_set_status(app, "Profile saved successfully.");
        refresh_profile_list(app);
    } else {
        dc_app_set_status(app, error_message != NULL ? error_message : "Failed to save profile.");
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
        dc_app_set_status(app, "Choose a saved profile first.");
        return;
    }

    if (dc_profile_service_load(profile_name, &configs, &error_message)) {
        apply_configs_to_rows(app, configs);
        dc_app_set_status(app, "Profile loaded into the form. Review and press Apply.");
        g_ptr_array_free(configs, TRUE);
    } else {
        dc_app_set_status(app, error_message != NULL ? error_message : "Failed to load profile.");
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
        dc_app_set_status(app, "Display settings applied successfully.");
        return TRUE;
    }

    if (!dc_xrandr_service_apply_configs(app->service, previous_configs, &error_message)) {
        dc_app_set_status(app, error_message != NULL ? error_message : "Failed to revert display settings.");
        g_free(error_message);
        return FALSE;
    }

    dc_app_reload_outputs(app);
    dc_app_set_status(app, "Display settings were reverted.");
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
        dc_app_reload_outputs(app);
        confirm_or_revert(app, previous_configs);
    } else {
        dc_app_set_status(app, error_message != NULL ? error_message : "Failed to apply display settings.");
    }

    g_free(error_message);
    g_ptr_array_free(previous_configs, TRUE);
    g_ptr_array_free(configs, TRUE);
}

void dc_app_connect_display_page_signals(DcAppController *app) {
    g_signal_connect(dc_display_page_get_refresh_button(app->display_page), "clicked", G_CALLBACK(on_refresh_clicked), app);
    g_signal_connect(dc_display_page_get_apply_button(app->display_page), "clicked", G_CALLBACK(on_apply_clicked), app);
    g_signal_connect(dc_display_page_get_extend_button(app->display_page), "clicked", G_CALLBACK(on_extend_clicked), app);
    g_signal_connect(dc_display_page_get_mirror_button(app->display_page), "clicked", G_CALLBACK(on_mirror_clicked), app);
    g_signal_connect(dc_display_page_get_internal_button(app->display_page), "clicked", G_CALLBACK(on_internal_clicked), app);
    g_signal_connect(dc_display_page_get_external_button(app->display_page), "clicked", G_CALLBACK(on_external_clicked), app);
    g_signal_connect(dc_display_page_get_save_profile_button(app->display_page), "clicked", G_CALLBACK(on_save_profile_clicked), app);
    g_signal_connect(dc_display_page_get_load_profile_button(app->display_page), "clicked", G_CALLBACK(on_load_profile_clicked), app);
}
