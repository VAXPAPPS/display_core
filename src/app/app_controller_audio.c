#include "app_controller_internal.h"

#include "services/audio_service.h"

static void free_audio_device_list(GList *devices) {
    g_list_free_full(devices, (GDestroyNotify) dc_audio_service_device_free);
}

static void free_audio_app_stream_list(GList *apps) {
    g_list_free_full(apps, (GDestroyNotify) dc_audio_service_app_stream_free);
}

static gboolean load_audio_page(DcAppController *app);

static gboolean refresh_audio_page_idle(gpointer user_data) {
    DcAppController *app = user_data;

    app->audio_refresh_idle_id = 0;
    load_audio_page(app);
    return G_SOURCE_REMOVE;
}

static void queue_audio_page_refresh(DcAppController *app) {
    if (app->audio_refresh_idle_id != 0) {
        return;
    }

    app->audio_refresh_idle_id = g_idle_add(refresh_audio_page_idle, app);
}

static void on_audio_devices_changed(void *userdata) {
    queue_audio_page_refresh(userdata);
}

static void on_audio_apps_changed(void *userdata) {
    queue_audio_page_refresh(userdata);
}

static void on_audio_overamplification_changed(void *userdata) {
    queue_audio_page_refresh(userdata);
}

static void populate_device_combo(GtkComboBoxText *combo, GList *devices) {
    GList *iter;

    gtk_combo_box_text_remove_all(combo);

    for (iter = devices; iter != NULL; iter = iter->next) {
        DcAudioDevice *device = iter->data;

        gtk_combo_box_text_append(combo,
                                  device->name,
                                  device->description != NULL ? device->description : device->name);
        if (device->is_default) {
            gtk_combo_box_set_active_id(GTK_COMBO_BOX(combo), device->name);
        }
    }

    if (gtk_combo_box_get_active(GTK_COMBO_BOX(combo)) < 0 && devices != NULL) {
        DcAudioDevice *first = devices->data;
        gtk_combo_box_set_active_id(GTK_COMBO_BOX(combo), first->name);
    }
}

static void clear_app_rows(DcAppController *app) {
    GList *children;
    GList *iter;

    children = gtk_container_get_children(GTK_CONTAINER(dc_audio_page_get_apps_box(app->audio_page)));
    for (iter = children; iter != NULL; iter = iter->next) {
        gtk_widget_destroy(GTK_WIDGET(iter->data));
    }
    g_list_free(children);
}

static GtkWidget *create_app_stream_row(const DcAudioAppStream *app_stream) {
    GtkWidget *row;
    GtkWidget *text_box;
    GtkWidget *title_label;
    GtkWidget *description_label;
    GtkWidget *controls;
    GtkWidget *volume_scale;
    GtkWidget *mute_switch;

    row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    text_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 3);
    title_label = gtk_label_new(app_stream->name != NULL ? app_stream->name : "Unknown");
    description_label = gtk_label_new(app_stream->sink_name != NULL ? app_stream->sink_name : "Current output stream");
    controls = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    volume_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.0, 150.0, 1.0);
    mute_switch = gtk_switch_new();

    gtk_widget_set_halign(title_label, GTK_ALIGN_START);
    gtk_widget_set_halign(description_label, GTK_ALIGN_START);
    gtk_widget_set_hexpand(text_box, TRUE);
    gtk_widget_set_hexpand(volume_scale, FALSE);
    gtk_widget_set_size_request(volume_scale, 160, -1);
    gtk_widget_set_valign(volume_scale, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(mute_switch, GTK_ALIGN_CENTER);
    gtk_widget_set_halign(mute_switch, GTK_ALIGN_END);
    gtk_widget_set_size_request(mute_switch, 42, 22);
    gtk_label_set_line_wrap(GTK_LABEL(description_label), TRUE);
    gtk_scale_set_draw_value(GTK_SCALE(volume_scale), TRUE);
    gtk_range_set_value(GTK_RANGE(volume_scale), app_stream->volume);
    gtk_switch_set_active(GTK_SWITCH(mute_switch), app_stream->muted);

    dc_app_add_css_class(title_label, "audio-setting-title");
    dc_app_add_css_class(description_label, "audio-setting-desc");
    dc_app_add_css_class(row, "audio-row");

    g_object_set_data(G_OBJECT(volume_scale), "app-stream-index", GUINT_TO_POINTER(app_stream->index));
    g_object_set_data(G_OBJECT(mute_switch), "app-stream-index", GUINT_TO_POINTER(app_stream->index));
    g_object_set_data(G_OBJECT(row), "app-stream-volume-scale", volume_scale);
    g_object_set_data(G_OBJECT(row), "app-stream-mute-switch", mute_switch);

    gtk_box_pack_start(GTK_BOX(text_box), title_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(text_box), description_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(controls), volume_scale, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(controls), mute_switch, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(row), text_box, TRUE, TRUE, 0);
    gtk_box_pack_end(GTK_BOX(row), controls, FALSE, FALSE, 0);
    return row;
}

static void on_app_volume_changed(GtkRange *range, gpointer user_data);
static void on_app_mute_changed(GObject *object, GParamSpec *pspec, gpointer user_data);

static void populate_app_streams(DcAppController *app, GList *apps) {
    GList *iter;
    GtkWidget *apps_box;

    apps_box = dc_audio_page_get_apps_box(app->audio_page);

    clear_app_rows(app);

    if (apps == NULL) {
        GtkWidget *empty_label = gtk_label_new("No active application streams were found.");
        gtk_widget_set_halign(empty_label, GTK_ALIGN_START);
        dc_app_add_css_class(empty_label, "audio-setting-desc");
        gtk_box_pack_start(GTK_BOX(apps_box), empty_label, FALSE, FALSE, 18);
        gtk_widget_show_all(apps_box);
        return;
    }

    for (iter = apps; iter != NULL; iter = iter->next) {
        DcAudioAppStream *app_stream = iter->data;
        GtkWidget *row = create_app_stream_row(app_stream);
        GtkWidget *volume_scale;
        GtkWidget *mute_switch;

        gtk_box_pack_start(GTK_BOX(apps_box), row, FALSE, FALSE, 0);
        volume_scale = g_object_get_data(G_OBJECT(row), "app-stream-volume-scale");
        mute_switch = g_object_get_data(G_OBJECT(row), "app-stream-mute-switch");

        g_signal_connect(volume_scale, "value-changed", G_CALLBACK(on_app_volume_changed), app);
        g_signal_connect(mute_switch, "notify::active", G_CALLBACK(on_app_mute_changed), app);
    }

    gtk_widget_show_all(apps_box);
}

static void update_output_scale_range(DcAppController *app) {
    gboolean overamplification = gtk_switch_get_active(GTK_SWITCH(dc_audio_page_get_overamplification_switch(app->audio_page)));
    GtkRange *scale = GTK_RANGE(dc_audio_page_get_output_volume_scale(app->audio_page));
    gdouble current_value = gtk_range_get_value(scale);
    gdouble max_value = overamplification ? 150.0 : 100.0;

    gtk_range_set_range(scale, 0.0, max_value);
    if (current_value > max_value) {
        gtk_range_set_value(scale, max_value);
    }
}

static gboolean load_audio_page(DcAppController *app) {
    GList *outputs = NULL;
    GList *inputs = NULL;
    GList *apps = NULL;
    char *error_message = NULL;
    gboolean success = TRUE;

    if (!dc_audio_service_init(&error_message)) {
        dc_app_set_status(app, error_message != NULL ? error_message : "Audio backend is unavailable.");
        g_free(error_message);
        gtk_widget_set_sensitive(dc_audio_page_get_widget(app->audio_page), FALSE);
        return FALSE;
    }

    dc_audio_service_set_devices_changed_callback(on_audio_devices_changed, app);
    dc_audio_service_set_apps_changed_callback(on_audio_apps_changed, app);
    dc_audio_service_set_overamplification_changed_callback(on_audio_overamplification_changed, app);

    outputs = dc_audio_service_list_outputs(&error_message);
    if (outputs == NULL && error_message != NULL) {
        dc_app_set_status(app, error_message);
        g_clear_pointer(&error_message, g_free);
        success = FALSE;
    }

    inputs = dc_audio_service_list_inputs(&error_message);
    if (inputs == NULL && error_message != NULL) {
        dc_app_set_status(app, error_message);
        g_clear_pointer(&error_message, g_free);
        success = FALSE;
    }

    apps = dc_audio_service_list_app_streams(&error_message);
    if (apps == NULL && error_message != NULL) {
        dc_app_set_status(app, error_message);
        g_clear_pointer(&error_message, g_free);
    }

    app->suppress_audio_updates = TRUE;
    populate_device_combo(GTK_COMBO_BOX_TEXT(dc_audio_page_get_output_combo(app->audio_page)), outputs);
    populate_device_combo(GTK_COMBO_BOX_TEXT(dc_audio_page_get_input_combo(app->audio_page)), inputs);
    gtk_switch_set_active(GTK_SWITCH(dc_audio_page_get_overamplification_switch(app->audio_page)),
                          dc_audio_service_get_overamplification());
    update_output_scale_range(app);
    gtk_range_set_value(GTK_RANGE(dc_audio_page_get_output_volume_scale(app->audio_page)),
                        dc_audio_service_get_output_volume());
    gtk_switch_set_active(GTK_SWITCH(dc_audio_page_get_output_mute_switch(app->audio_page)),
                          dc_audio_service_get_output_muted());
    gtk_range_set_value(GTK_RANGE(dc_audio_page_get_input_volume_scale(app->audio_page)),
                        dc_audio_service_get_input_volume());
    gtk_switch_set_active(GTK_SWITCH(dc_audio_page_get_input_mute_switch(app->audio_page)),
                          dc_audio_service_get_input_muted());
    populate_app_streams(app, apps);
    app->suppress_audio_updates = FALSE;

    free_audio_device_list(outputs);
    free_audio_device_list(inputs);
    free_audio_app_stream_list(apps);
    g_free(error_message);

    gtk_widget_set_sensitive(dc_audio_page_get_widget(app->audio_page), success);
    if (success) {
        dc_app_set_status(app, "Audio controls loaded.");
    }

    return success;
}

void dc_app_audio_load(DcAppController *app) {
    load_audio_page(app);
}

static void on_output_device_changed(GtkWidget *widget, gpointer user_data) {
    DcAppController *app = user_data;
    const char *active_id;
    char *error_message = NULL;

    if (app->suppress_audio_updates) {
        return;
    }

    active_id = gtk_combo_box_get_active_id(GTK_COMBO_BOX(widget));
    if (active_id == NULL) {
        return;
    }

    if (!dc_audio_service_set_default_output(active_id, &error_message)) {
        dc_app_set_status(app, error_message != NULL ? error_message : "Failed to change the output device.");
        g_free(error_message);
        return;
    }

    load_audio_page(app);
}

static void on_output_volume_changed(GtkRange *range, gpointer user_data) {
    DcAppController *app = user_data;
    char *error_message = NULL;

    if (app->suppress_audio_updates) {
        return;
    }

    if (!dc_audio_service_set_output_volume((gint) gtk_range_get_value(range), &error_message)) {
        dc_app_set_status(app, error_message != NULL ? error_message : "Failed to update output volume.");
        g_free(error_message);
        return;
    }

    dc_app_set_status(app, "Output volume updated.");
}

static void on_output_mute_changed(GObject *object, GParamSpec *pspec, gpointer user_data) {
    DcAppController *app = user_data;
    char *error_message = NULL;

    (void) pspec;

    if (app->suppress_audio_updates) {
        return;
    }

    if (!dc_audio_service_set_output_muted(gtk_switch_get_active(GTK_SWITCH(object)), &error_message)) {
        dc_app_set_status(app, error_message != NULL ? error_message : "Failed to update output mute state.");
        g_free(error_message);
        return;
    }

    dc_app_set_status(app, "Output mute state updated.");
}

static void on_input_device_changed(GtkWidget *widget, gpointer user_data) {
    DcAppController *app = user_data;
    const char *active_id;
    char *error_message = NULL;

    if (app->suppress_audio_updates) {
        return;
    }

    active_id = gtk_combo_box_get_active_id(GTK_COMBO_BOX(widget));
    if (active_id == NULL) {
        return;
    }

    if (!dc_audio_service_set_default_input(active_id, &error_message)) {
        dc_app_set_status(app, error_message != NULL ? error_message : "Failed to change the input device.");
        g_free(error_message);
        return;
    }

    load_audio_page(app);
}

static void on_input_volume_changed(GtkRange *range, gpointer user_data) {
    DcAppController *app = user_data;
    char *error_message = NULL;

    if (app->suppress_audio_updates) {
        return;
    }

    if (!dc_audio_service_set_input_volume((gint) gtk_range_get_value(range), &error_message)) {
        dc_app_set_status(app, error_message != NULL ? error_message : "Failed to update input volume.");
        g_free(error_message);
        return;
    }

    dc_app_set_status(app, "Input volume updated.");
}

static void on_input_mute_changed(GObject *object, GParamSpec *pspec, gpointer user_data) {
    DcAppController *app = user_data;
    char *error_message = NULL;

    (void) pspec;

    if (app->suppress_audio_updates) {
        return;
    }

    if (!dc_audio_service_set_input_muted(gtk_switch_get_active(GTK_SWITCH(object)), &error_message)) {
        dc_app_set_status(app, error_message != NULL ? error_message : "Failed to update input mute state.");
        g_free(error_message);
        return;
    }

    dc_app_set_status(app, "Input mute state updated.");
}

static void on_app_volume_changed(GtkRange *range, gpointer user_data) {
    DcAppController *app = user_data;
    guint32 index;
    char *error_message = NULL;

    if (app->suppress_audio_updates) {
        return;
    }

    index = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(range), "app-stream-index"));
    if (!dc_audio_service_set_app_volume(index, (gint) gtk_range_get_value(range), &error_message)) {
        dc_app_set_status(app, error_message != NULL ? error_message : "Failed to update application volume.");
        g_free(error_message);
        return;
    }

    dc_app_set_status(app, "Application volume updated.");
}

static void on_app_mute_changed(GObject *object, GParamSpec *pspec, gpointer user_data) {
    DcAppController *app = user_data;
    guint32 index;
    char *error_message = NULL;

    (void) pspec;

    if (app->suppress_audio_updates) {
        return;
    }

    index = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(object), "app-stream-index"));
    if (!dc_audio_service_set_app_muted(index, gtk_switch_get_active(GTK_SWITCH(object)), &error_message)) {
        dc_app_set_status(app, error_message != NULL ? error_message : "Failed to update application mute state.");
        g_free(error_message);
        return;
    }

    dc_app_set_status(app, "Application mute state updated.");
}

static void on_overamplification_changed(GObject *object, GParamSpec *pspec, gpointer user_data) {
    DcAppController *app = user_data;
    char *error_message = NULL;

    (void) pspec;

    if (app->suppress_audio_updates) {
        return;
    }

    if (!dc_audio_service_set_overamplification(gtk_switch_get_active(GTK_SWITCH(object)), &error_message)) {
        dc_app_set_status(app, error_message != NULL ? error_message : "Failed to update over-amplification.");
        g_free(error_message);
        return;
    }

    update_output_scale_range(app);
    load_audio_page(app);
}

void dc_app_audio_connect_signals(DcAppController *app) {
    g_signal_connect(dc_audio_page_get_output_combo(app->audio_page), "changed", G_CALLBACK(on_output_device_changed), app);
    g_signal_connect(dc_audio_page_get_output_volume_scale(app->audio_page), "value-changed", G_CALLBACK(on_output_volume_changed), app);
    g_signal_connect(dc_audio_page_get_output_mute_switch(app->audio_page), "notify::active", G_CALLBACK(on_output_mute_changed), app);
    g_signal_connect(dc_audio_page_get_input_combo(app->audio_page), "changed", G_CALLBACK(on_input_device_changed), app);
    g_signal_connect(dc_audio_page_get_input_volume_scale(app->audio_page), "value-changed", G_CALLBACK(on_input_volume_changed), app);
    g_signal_connect(dc_audio_page_get_input_mute_switch(app->audio_page), "notify::active", G_CALLBACK(on_input_mute_changed), app);
    g_signal_connect(dc_audio_page_get_overamplification_switch(app->audio_page), "notify::active", G_CALLBACK(on_overamplification_changed), app);
}
