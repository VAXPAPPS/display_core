#ifndef DC_AUDIO_SERVICE_H
#define DC_AUDIO_SERVICE_H

#include <glib.h>

typedef struct {
    gchar *name;
    gchar *description;
    gint volume;
    gboolean muted;
    gboolean is_default;
} DcAudioDevice;

typedef struct {
    guint32 index;
    gchar *name;
    gchar *icon;
    gint volume;
    gboolean muted;
    gchar *sink_name;
} DcAudioAppStream;

gboolean dc_audio_service_init(char **error_message);
void dc_audio_service_cleanup(void);
gboolean dc_audio_service_is_ready(void);

gint dc_audio_service_get_output_volume(void);
gboolean dc_audio_service_set_output_volume(gint volume, char **error_message);
gboolean dc_audio_service_get_output_muted(void);
gboolean dc_audio_service_set_output_muted(gboolean muted, char **error_message);

gint dc_audio_service_get_input_volume(void);
gboolean dc_audio_service_set_input_volume(gint volume, char **error_message);
gboolean dc_audio_service_get_input_muted(void);
gboolean dc_audio_service_set_input_muted(gboolean muted, char **error_message);

GList *dc_audio_service_list_outputs(char **error_message);
gboolean dc_audio_service_set_default_output(const char *name, char **error_message);

GList *dc_audio_service_list_inputs(char **error_message);
gboolean dc_audio_service_set_default_input(const char *name, char **error_message);

gboolean dc_audio_service_get_overamplification(void);
gboolean dc_audio_service_set_overamplification(gboolean enabled, char **error_message);

GList *dc_audio_service_list_app_streams(char **error_message);
gboolean dc_audio_service_set_app_volume(guint32 index, gint volume, char **error_message);
gboolean dc_audio_service_set_app_muted(guint32 index, gboolean muted, char **error_message);

void dc_audio_service_set_devices_changed_callback(void (*callback)(void *userdata), void *userdata);
void dc_audio_service_set_apps_changed_callback(void (*callback)(void *userdata), void *userdata);
void dc_audio_service_set_overamplification_changed_callback(void (*callback)(void *userdata), void *userdata);

void dc_audio_service_device_free(DcAudioDevice *device);
void dc_audio_service_app_stream_free(DcAudioAppStream *app_stream);

#endif
