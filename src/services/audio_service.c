#include "services/audio_service.h"

#include <gio/gio.h>

#define DC_AUDIO_DBUS_NAME "org.venom.Audio"
#define DC_AUDIO_DBUS_PATH "/org/venom/Audio"
#define DC_AUDIO_DBUS_INTERFACE "org.venom.Audio"

static GDBusConnection *audio_connection = NULL;
static guint devices_signal_id = 0;
static guint apps_signal_id = 0;
static guint overamplification_signal_id = 0;
static guint volume_signal_id = 0;
static guint mute_signal_id = 0;

static void (*devices_changed_callback)(void *userdata) = NULL;
static void *devices_changed_user_data = NULL;
static void (*apps_changed_callback)(void *userdata) = NULL;
static void *apps_changed_user_data = NULL;
static void (*overamplification_changed_callback)(void *userdata) = NULL;
static void *overamplification_changed_user_data = NULL;

static void set_error(char **error_message, const char *message) {
    if (error_message == NULL) {
        return;
    }

    g_free(*error_message);
    *error_message = g_strdup(message);
}

static void set_gerror_message(char **error_message, const char *fallback, GError *error) {
    if (error != NULL && error->message != NULL) {
        set_error(error_message, error->message);
        return;
    }

    set_error(error_message, fallback);
}

static void dispatch_devices_changed(void) {
    if (devices_changed_callback != NULL) {
        devices_changed_callback(devices_changed_user_data);
    }
}

static void dispatch_apps_changed(void) {
    if (apps_changed_callback != NULL) {
        apps_changed_callback(apps_changed_user_data);
    }
}

static void dispatch_overamplification_changed(void) {
    if (overamplification_changed_callback != NULL) {
        overamplification_changed_callback(overamplification_changed_user_data);
    }
}

static void on_dbus_signal(GDBusConnection *connection,
                           const gchar *sender_name,
                           const gchar *object_path,
                           const gchar *interface_name,
                           const gchar *signal_name,
                           GVariant *parameters,
                           gpointer user_data) {
    (void) connection;
    (void) sender_name;
    (void) object_path;
    (void) interface_name;
    (void) parameters;
    (void) user_data;

    if (g_strcmp0(signal_name, "DevicesChanged") == 0 ||
        g_strcmp0(signal_name, "VolumeChanged") == 0 ||
        g_strcmp0(signal_name, "MuteChanged") == 0) {
        dispatch_devices_changed();
        return;
    }

    if (g_strcmp0(signal_name, "AppsChanged") == 0) {
        dispatch_apps_changed();
        return;
    }

    if (g_strcmp0(signal_name, "OveramplificationChanged") == 0) {
        dispatch_overamplification_changed();
    }
}

static gboolean ensure_connection(char **error_message) {
    GError *error = NULL;

    if (audio_connection != NULL) {
        return TRUE;
    }

    audio_connection = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);
    if (audio_connection == NULL) {
        set_gerror_message(error_message, "Failed to connect to the session D-Bus.", error);
        g_clear_error(&error);
        return FALSE;
    }

    return TRUE;
}

static GVariant *call_method(const gchar *method_name,
                             GVariant *parameters,
                             const GVariantType *reply_type,
                             char **error_message) {
    GError *error = NULL;
    GVariant *result;

    if (!ensure_connection(error_message)) {
        return NULL;
    }

    result = g_dbus_connection_call_sync(audio_connection,
                                         DC_AUDIO_DBUS_NAME,
                                         DC_AUDIO_DBUS_PATH,
                                         DC_AUDIO_DBUS_INTERFACE,
                                         method_name,
                                         parameters,
                                         reply_type,
                                         G_DBUS_CALL_FLAGS_NONE,
                                         -1,
                                         NULL,
                                         &error);
    if (result == NULL) {
        set_gerror_message(error_message, "Audio daemon request failed.", error);
        g_clear_error(&error);
        return NULL;
    }

    return result;
}

static gboolean call_boolean_method(const gchar *method_name,
                                    GVariant *parameters,
                                    char **error_message) {
    GVariant *result;
    gboolean success = FALSE;

    result = call_method(method_name, parameters, G_VARIANT_TYPE("(b)"), error_message);
    if (result == NULL) {
        return FALSE;
    }

    g_variant_get(result, "(b)", &success);
    g_variant_unref(result);

    if (!success && error_message != NULL && *error_message == NULL) {
        set_error(error_message, "Audio daemon rejected the request.");
    }

    return success;
}

static gint call_int_method(const gchar *method_name, gint fallback_value) {
    GVariant *result;
    gint value = fallback_value;

    result = call_method(method_name, NULL, G_VARIANT_TYPE("(i)"), NULL);
    if (result == NULL) {
        return fallback_value;
    }

    g_variant_get(result, "(i)", &value);
    g_variant_unref(result);
    return value;
}

static gboolean call_bool_method(const gchar *method_name, gboolean fallback_value) {
    GVariant *result;
    gboolean value = fallback_value;

    result = call_method(method_name, NULL, G_VARIANT_TYPE("(b)"), NULL);
    if (result == NULL) {
        return fallback_value;
    }

    g_variant_get(result, "(b)", &value);
    g_variant_unref(result);
    return value;
}

static void subscribe_signal(guint *subscription_id, const gchar *signal_name) {
    if (audio_connection == NULL || *subscription_id != 0) {
        return;
    }

    *subscription_id = g_dbus_connection_signal_subscribe(audio_connection,
                                                          DC_AUDIO_DBUS_NAME,
                                                          DC_AUDIO_DBUS_INTERFACE,
                                                          signal_name,
                                                          DC_AUDIO_DBUS_PATH,
                                                          NULL,
                                                          G_DBUS_SIGNAL_FLAGS_NONE,
                                                          on_dbus_signal,
                                                          NULL,
                                                          NULL);
}

gboolean dc_audio_service_init(char **error_message) {
    GVariant *result;

    if (!ensure_connection(error_message)) {
        return FALSE;
    }

    result = call_method("GetVolume", NULL, G_VARIANT_TYPE("(i)"), error_message);
    if (result == NULL) {
        return FALSE;
    }
    g_variant_unref(result);

    subscribe_signal(&devices_signal_id, "DevicesChanged");
    subscribe_signal(&apps_signal_id, "AppsChanged");
    subscribe_signal(&overamplification_signal_id, "OveramplificationChanged");
    subscribe_signal(&volume_signal_id, "VolumeChanged");
    subscribe_signal(&mute_signal_id, "MuteChanged");

    return TRUE;
}

void dc_audio_service_cleanup(void) {
    if (audio_connection != NULL) {
        if (devices_signal_id != 0) {
            g_dbus_connection_signal_unsubscribe(audio_connection, devices_signal_id);
            devices_signal_id = 0;
        }
        if (apps_signal_id != 0) {
            g_dbus_connection_signal_unsubscribe(audio_connection, apps_signal_id);
            apps_signal_id = 0;
        }
        if (overamplification_signal_id != 0) {
            g_dbus_connection_signal_unsubscribe(audio_connection, overamplification_signal_id);
            overamplification_signal_id = 0;
        }
        if (volume_signal_id != 0) {
            g_dbus_connection_signal_unsubscribe(audio_connection, volume_signal_id);
            volume_signal_id = 0;
        }
        if (mute_signal_id != 0) {
            g_dbus_connection_signal_unsubscribe(audio_connection, mute_signal_id);
            mute_signal_id = 0;
        }

        g_clear_object(&audio_connection);
    }
}

gboolean dc_audio_service_is_ready(void) {
    return audio_connection != NULL;
}

gint dc_audio_service_get_output_volume(void) {
    return call_int_method("GetVolume", 0);
}

gboolean dc_audio_service_set_output_volume(gint volume, char **error_message) {
    return call_boolean_method("SetVolume", g_variant_new("(i)", volume), error_message);
}

gboolean dc_audio_service_get_output_muted(void) {
    return call_bool_method("GetMuted", FALSE);
}

gboolean dc_audio_service_set_output_muted(gboolean muted, char **error_message) {
    return call_boolean_method("SetMuted", g_variant_new("(b)", muted), error_message);
}

gint dc_audio_service_get_input_volume(void) {
    return call_int_method("GetMicVolume", 0);
}

gboolean dc_audio_service_set_input_volume(gint volume, char **error_message) {
    return call_boolean_method("SetMicVolume", g_variant_new("(i)", volume), error_message);
}

gboolean dc_audio_service_get_input_muted(void) {
    return call_bool_method("GetMicMuted", FALSE);
}

gboolean dc_audio_service_set_input_muted(gboolean muted, char **error_message) {
    return call_boolean_method("SetMicMuted", g_variant_new("(b)", muted), error_message);
}

GList *dc_audio_service_list_outputs(char **error_message) {
    GVariant *result;
    GVariant *array;
    GVariantIter iter;
    GList *devices = NULL;
    const gchar *name;
    const gchar *description;
    gint volume;
    gboolean is_default;

    result = call_method("GetSinks", NULL, G_VARIANT_TYPE("(a(ssib))"), error_message);
    if (result == NULL) {
        return NULL;
    }

    array = g_variant_get_child_value(result, 0);
    g_variant_iter_init(&iter, array);
    while (g_variant_iter_loop(&iter, "(&s&sib)", &name, &description, &volume, &is_default)) {
        DcAudioDevice *device = g_new0(DcAudioDevice, 1);

        device->name = g_strdup(name);
        device->description = g_strdup(description);
        device->volume = volume;
        device->is_default = is_default;
        devices = g_list_append(devices, device);
    }

    g_variant_unref(array);
    g_variant_unref(result);
    return devices;
}

gboolean dc_audio_service_set_default_output(const char *name, char **error_message) {
    return call_boolean_method("SetDefaultSink", g_variant_new("(s)", name), error_message);
}

GList *dc_audio_service_list_inputs(char **error_message) {
    GVariant *result;
    GVariant *array;
    GVariantIter iter;
    GList *devices = NULL;
    const gchar *name;
    const gchar *description;
    gint volume;
    gboolean is_default;

    result = call_method("GetSources", NULL, G_VARIANT_TYPE("(a(ssib))"), error_message);
    if (result == NULL) {
        return NULL;
    }

    array = g_variant_get_child_value(result, 0);
    g_variant_iter_init(&iter, array);
    while (g_variant_iter_loop(&iter, "(&s&sib)", &name, &description, &volume, &is_default)) {
        DcAudioDevice *device = g_new0(DcAudioDevice, 1);

        device->name = g_strdup(name);
        device->description = g_strdup(description);
        device->volume = volume;
        device->is_default = is_default;
        devices = g_list_append(devices, device);
    }

    g_variant_unref(array);
    g_variant_unref(result);
    return devices;
}

gboolean dc_audio_service_set_default_input(const char *name, char **error_message) {
    return call_boolean_method("SetDefaultSource", g_variant_new("(s)", name), error_message);
}

gboolean dc_audio_service_get_overamplification(void) {
    return call_bool_method("GetOveramplification", FALSE);
}

gboolean dc_audio_service_set_overamplification(gboolean enabled, char **error_message) {
    return call_boolean_method("SetOveramplification", g_variant_new("(b)", enabled), error_message);
}

GList *dc_audio_service_list_app_streams(char **error_message) {
    GVariant *result;
    GVariant *array;
    GVariantIter iter;
    GList *apps = NULL;
    guint32 index;
    const gchar *name;
    const gchar *icon;
    gint volume;
    gboolean muted;

    result = call_method("GetAppStreams", NULL, G_VARIANT_TYPE("(a(ussib))"), error_message);
    if (result == NULL) {
        return NULL;
    }

    array = g_variant_get_child_value(result, 0);
    g_variant_iter_init(&iter, array);
    while (g_variant_iter_loop(&iter, "(u&s&sib)", &index, &name, &icon, &volume, &muted)) {
        DcAudioAppStream *app_stream = g_new0(DcAudioAppStream, 1);

        app_stream->index = index;
        app_stream->name = g_strdup(name);
        app_stream->icon = g_strdup(icon);
        app_stream->volume = volume;
        app_stream->muted = muted;
        app_stream->sink_name = NULL;
        apps = g_list_append(apps, app_stream);
    }

    g_variant_unref(array);
    g_variant_unref(result);
    return apps;
}

gboolean dc_audio_service_set_app_volume(guint32 index, gint volume, char **error_message) {
    return call_boolean_method("SetAppVolume", g_variant_new("(ui)", index, volume), error_message);
}

gboolean dc_audio_service_set_app_muted(guint32 index, gboolean muted, char **error_message) {
    return call_boolean_method("SetAppMuted", g_variant_new("(ub)", index, muted), error_message);
}

void dc_audio_service_set_devices_changed_callback(void (*callback)(void *userdata), void *userdata) {
    devices_changed_callback = callback;
    devices_changed_user_data = userdata;
}

void dc_audio_service_set_apps_changed_callback(void (*callback)(void *userdata), void *userdata) {
    apps_changed_callback = callback;
    apps_changed_user_data = userdata;
}

void dc_audio_service_set_overamplification_changed_callback(void (*callback)(void *userdata), void *userdata) {
    overamplification_changed_callback = callback;
    overamplification_changed_user_data = userdata;
}

void dc_audio_service_device_free(DcAudioDevice *device) {
    if (device == NULL) {
        return;
    }

    g_free(device->name);
    g_free(device->description);
    g_free(device);
}

void dc_audio_service_app_stream_free(DcAudioAppStream *app_stream) {
    if (app_stream == NULL) {
        return;
    }

    g_free(app_stream->name);
    g_free(app_stream->icon);
    g_free(app_stream->sink_name);
    g_free(app_stream);
}
