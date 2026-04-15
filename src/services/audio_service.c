#include "services/audio_service.h"
#include <pulse/pulseaudio.h>
#include <string.h>

static pa_threaded_mainloop *pa_ml = NULL;
static pa_context *pa_ctx = NULL;

static void (*devices_changed_callback)(void *userdata) = NULL;
static void *devices_changed_user_data = NULL;
static void (*apps_changed_callback)(void *userdata) = NULL;
static void *apps_changed_user_data = NULL;
static void (*overamplification_changed_callback)(void *userdata) = NULL;
static void *overamplification_changed_user_data = NULL;

static gboolean overamplification_enabled = FALSE;
static guint idle_dispatch_devices_id = 0;
static guint idle_dispatch_apps_id = 0;

static char *default_sink_name = NULL;
static char *default_source_name = NULL;

static void success_cb(pa_context *c, int success, void *userdata);

static void set_error(char **error_message, const char *message) {
    if (error_message) {
        g_free(*error_message);
        *error_message = g_strdup(message);
    }
}

static gboolean dispatch_devices_idle(gpointer user_data) {
    (void)user_data;
    idle_dispatch_devices_id = 0;
    if (devices_changed_callback) {
        devices_changed_callback(devices_changed_user_data);
    }
    return G_SOURCE_REMOVE;
}

static void schedule_devices_changed(void) {
    if (idle_dispatch_devices_id == 0) {
        idle_dispatch_devices_id = g_idle_add(dispatch_devices_idle, NULL);
    }
}

static gboolean dispatch_apps_idle(gpointer user_data) {
    (void)user_data;
    idle_dispatch_apps_id = 0;
    if (apps_changed_callback) {
        apps_changed_callback(apps_changed_user_data);
    }
    return G_SOURCE_REMOVE;
}

static void schedule_apps_changed(void) {
    if (idle_dispatch_apps_id == 0) {
        idle_dispatch_apps_id = g_idle_add(dispatch_apps_idle, NULL);
    }
}

static void server_info_cb(pa_context *c, const pa_server_info *i, void *userdata) {
    (void)c;
    (void)userdata;
    if (i) {
        g_free(default_sink_name);
        default_sink_name = g_strdup(i->default_sink_name);
        g_free(default_source_name);
        default_source_name = g_strdup(i->default_source_name);
    }
    pa_threaded_mainloop_signal(pa_ml, 0);
}

static void subscription_cb(pa_context *c, pa_subscription_event_type_t t, uint32_t index, void *userdata) {
    (void)c;
    (void)index;
    (void)userdata;

    int facility = t & PA_SUBSCRIPTION_EVENT_FACILITY_MASK;

    if (facility == PA_SUBSCRIPTION_EVENT_SINK ||
        facility == PA_SUBSCRIPTION_EVENT_SOURCE ||
        facility == PA_SUBSCRIPTION_EVENT_SERVER) {
        
        if (facility == PA_SUBSCRIPTION_EVENT_SERVER) {
            pa_operation *o = pa_context_get_server_info(c, server_info_cb, NULL);
            if (o) pa_operation_unref(o);
        }
        
        schedule_devices_changed();
    } else if (facility == PA_SUBSCRIPTION_EVENT_SINK_INPUT) {
        schedule_apps_changed();
    }
}

static void context_state_cb(pa_context *c, void *userdata) {
    (void)userdata;
    pa_context_state_t state = pa_context_get_state(c);
    if (state == PA_CONTEXT_READY || state == PA_CONTEXT_FAILED || state == PA_CONTEXT_TERMINATED) {
        pa_threaded_mainloop_signal(pa_ml, 0);
    }
}

static gboolean wait_for_operation(pa_operation *o) {
    if (!o) return FALSE;
    while (pa_operation_get_state(o) == PA_OPERATION_RUNNING) {
        pa_threaded_mainloop_wait(pa_ml);
    }
    pa_operation_unref(o);
    return TRUE;
}

gboolean dc_audio_service_init(char **error_message) {
    if (pa_ml != NULL) return TRUE;

    pa_ml = pa_threaded_mainloop_new();
    if (!pa_ml) {
        set_error(error_message, "Failed to create PulseAudio mainloop");
        return FALSE;
    }

    if (pa_threaded_mainloop_start(pa_ml) < 0) {
        set_error(error_message, "Failed to start PulseAudio mainloop");
        pa_threaded_mainloop_free(pa_ml);
        pa_ml = NULL;
        return FALSE;
    }

    pa_threaded_mainloop_lock(pa_ml);
    pa_ctx = pa_context_new(pa_threaded_mainloop_get_api(pa_ml), "display-settings");
    if (!pa_ctx) {
        pa_threaded_mainloop_unlock(pa_ml);
        set_error(error_message, "Failed to create PulseAudio context");
        goto fail;
    }

    pa_context_set_state_callback(pa_ctx, context_state_cb, NULL);
    if (pa_context_connect(pa_ctx, NULL, PA_CONTEXT_NOFLAGS, NULL) < 0) {
        pa_threaded_mainloop_unlock(pa_ml);
        set_error(error_message, "Failed to connect to PulseAudio service");
        goto fail;
    }

    while (TRUE) {
        pa_context_state_t state = pa_context_get_state(pa_ctx);
        if (state == PA_CONTEXT_READY || state == PA_CONTEXT_FAILED || state == PA_CONTEXT_TERMINATED) {
            break;
        }
        pa_threaded_mainloop_wait(pa_ml);
    }

    if (pa_context_get_state(pa_ctx) != PA_CONTEXT_READY) {
        pa_threaded_mainloop_unlock(pa_ml);
        set_error(error_message, "Failed to connect to PulseAudio service (not ready)");
        goto fail;
    }

    pa_context_set_subscribe_callback(pa_ctx, subscription_cb, NULL);
    pa_operation *o = pa_context_subscribe(pa_ctx, PA_SUBSCRIPTION_MASK_SINK | PA_SUBSCRIPTION_MASK_SOURCE | PA_SUBSCRIPTION_MASK_SINK_INPUT | PA_SUBSCRIPTION_MASK_SERVER, success_cb, NULL);
    wait_for_operation(o);

    o = pa_context_get_server_info(pa_ctx, server_info_cb, NULL);
    wait_for_operation(o);

    pa_threaded_mainloop_unlock(pa_ml);
    return TRUE;

fail:
    if (pa_ctx) {
        pa_context_unref(pa_ctx);
        pa_ctx = NULL;
    }
    if (pa_ml) {
        pa_threaded_mainloop_stop(pa_ml);
        pa_threaded_mainloop_free(pa_ml);
        pa_ml = NULL;
    }
    return FALSE;
}

void dc_audio_service_cleanup(void) {
    if (pa_ml) {
        pa_threaded_mainloop_lock(pa_ml);
        if (pa_ctx) {
            pa_context_disconnect(pa_ctx);
            pa_context_unref(pa_ctx);
            pa_ctx = NULL;
        }
        pa_threaded_mainloop_unlock(pa_ml);
        pa_threaded_mainloop_stop(pa_ml);
        pa_threaded_mainloop_free(pa_ml);
        pa_ml = NULL;
    }

    if (idle_dispatch_devices_id) {
        g_source_remove(idle_dispatch_devices_id);
        idle_dispatch_devices_id = 0;
    }
    if (idle_dispatch_apps_id) {
        g_source_remove(idle_dispatch_apps_id);
        idle_dispatch_apps_id = 0;
    }

    g_free(default_sink_name);
    default_sink_name = NULL;
    g_free(default_source_name);
    default_source_name = NULL;
}

gboolean dc_audio_service_is_ready(void) {
    return pa_ctx != NULL && pa_context_get_state(pa_ctx) == PA_CONTEXT_READY;
}

static gint pa_volume_to_gint(const pa_cvolume *cv) {
    pa_volume_t v = pa_cvolume_avg(cv);
    return (gint)((v * 100 + PA_VOLUME_NORM / 2) / PA_VOLUME_NORM);
}

static pa_cvolume gint_to_pa_volume(const pa_cvolume *orig_cv, gint vol) {
    pa_cvolume out = *orig_cv;
    pa_volume_t target_v = (pa_volume_t)((vol * PA_VOLUME_NORM + 50) / 100);
    if (out.channels == 0) {
        pa_cvolume_set(&out, 2, target_v);
    } else {
        pa_cvolume_scale(&out, target_v);
    }
    return out;
}

typedef struct {
    gint param_int;
    gboolean param_bool;
    gboolean success;
    char **error_message;
} OpState;

static void success_cb(pa_context *c, int success, void *userdata) {
    (void)c;
    if (userdata) {
        OpState *state = userdata;
        state->success = (success != 0);
        if (!success && state->error_message) {
            set_error(state->error_message, "PulseAudio operation rejected.");
        }
    }
    pa_threaded_mainloop_signal(pa_ml, 0);
}

typedef struct {
    gint vol;
    gboolean muted;
} SingleState;

static void single_sink_info_cb(pa_context *c, const pa_sink_info *i, int eol, void *userdata) {
    (void)c;
    if (eol > 0) {
        pa_threaded_mainloop_signal(pa_ml, 0);
        return;
    }
    SingleState *state = userdata;
    state->vol = pa_volume_to_gint(&i->volume);
    state->muted = (i->mute != 0);
}

static void single_source_info_cb(pa_context *c, const pa_source_info *i, int eol, void *userdata) {
    (void)c;
    if (eol > 0) {
        pa_threaded_mainloop_signal(pa_ml, 0);
        return;
    }
    SingleState *state = userdata;
    state->vol = pa_volume_to_gint(&i->volume);
    state->muted = (i->mute != 0);
}

gint dc_audio_service_get_output_volume(void) {
    if (!dc_audio_service_is_ready() || !default_sink_name) return 0;
    SingleState state = {0, FALSE};
    pa_threaded_mainloop_lock(pa_ml);
    pa_operation *o = pa_context_get_sink_info_by_name(pa_ctx, default_sink_name, single_sink_info_cb, &state);
    wait_for_operation(o);
    pa_threaded_mainloop_unlock(pa_ml);
    return state.vol;
}

gboolean dc_audio_service_get_output_muted(void) {
    if (!dc_audio_service_is_ready() || !default_sink_name) return FALSE;
    SingleState state = {0, FALSE};
    pa_threaded_mainloop_lock(pa_ml);
    pa_operation *o = pa_context_get_sink_info_by_name(pa_ctx, default_sink_name, single_sink_info_cb, &state);
    wait_for_operation(o);
    pa_threaded_mainloop_unlock(pa_ml);
    return state.muted;
}

gint dc_audio_service_get_input_volume(void) {
    if (!dc_audio_service_is_ready() || !default_source_name) return 0;
    SingleState state = {0, FALSE};
    pa_threaded_mainloop_lock(pa_ml);
    pa_operation *o = pa_context_get_source_info_by_name(pa_ctx, default_source_name, single_source_info_cb, &state);
    wait_for_operation(o);
    pa_threaded_mainloop_unlock(pa_ml);
    return state.vol;
}

gboolean dc_audio_service_get_input_muted(void) {
    if (!dc_audio_service_is_ready() || !default_source_name) return FALSE;
    SingleState state = {0, FALSE};
    pa_threaded_mainloop_lock(pa_ml);
    pa_operation *o = pa_context_get_source_info_by_name(pa_ctx, default_source_name, single_source_info_cb, &state);
    wait_for_operation(o);
    pa_threaded_mainloop_unlock(pa_ml);
    return state.muted;
}

// Flat getters for volume modification
static void output_volume_set_cb(pa_context *c, const pa_sink_info *i, int eol, void *userdata) {
    if (eol > 0) {
        pa_threaded_mainloop_signal(pa_ml, 0);
        return;
    }
    OpState *state = userdata;
    pa_cvolume modified = gint_to_pa_volume(&i->volume, state->param_int);
    pa_operation *o = pa_context_set_sink_volume_by_name(c, i->name, &modified, NULL, NULL);
    if (o) pa_operation_unref(o);
    state->success = TRUE;
}

gboolean dc_audio_service_set_output_volume(gint volume, char **error_message) {
    if (!dc_audio_service_is_ready() || !default_sink_name) return FALSE;
    OpState state = {volume, FALSE, FALSE, error_message};
    pa_threaded_mainloop_lock(pa_ml);
    pa_operation *o = pa_context_get_sink_info_by_name(pa_ctx, default_sink_name, output_volume_set_cb, &state);
    wait_for_operation(o);
    pa_threaded_mainloop_unlock(pa_ml);
    return state.success;
}

gboolean dc_audio_service_set_output_muted(gboolean muted, char **error_message) {
    if (!dc_audio_service_is_ready() || !default_sink_name) return FALSE;
    OpState state = {0, muted, FALSE, error_message};
    pa_threaded_mainloop_lock(pa_ml);
    pa_operation *o = pa_context_set_sink_mute_by_name(pa_ctx, default_sink_name, muted, success_cb, &state);
    wait_for_operation(o);
    pa_threaded_mainloop_unlock(pa_ml);
    return state.success;
}

static void input_volume_set_cb(pa_context *c, const pa_source_info *i, int eol, void *userdata) {
    if (eol > 0) {
        pa_threaded_mainloop_signal(pa_ml, 0);
        return;
    }
    OpState *state = userdata;
    pa_cvolume modified = gint_to_pa_volume(&i->volume, state->param_int);
    pa_operation *o = pa_context_set_source_volume_by_name(c, i->name, &modified, NULL, NULL);
    if (o) pa_operation_unref(o);
    state->success = TRUE;
}

gboolean dc_audio_service_set_input_volume(gint volume, char **error_message) {
    if (!dc_audio_service_is_ready() || !default_source_name) return FALSE;
    OpState state = {volume, FALSE, FALSE, error_message};
    pa_threaded_mainloop_lock(pa_ml);
    pa_operation *o = pa_context_get_source_info_by_name(pa_ctx, default_source_name, input_volume_set_cb, &state);
    wait_for_operation(o);
    pa_threaded_mainloop_unlock(pa_ml);
    return state.success;
}

gboolean dc_audio_service_set_input_muted(gboolean muted, char **error_message) {
    if (!dc_audio_service_is_ready() || !default_source_name) return FALSE;
    OpState state = {0, muted, FALSE, error_message};
    pa_threaded_mainloop_lock(pa_ml);
    pa_operation *o = pa_context_set_source_mute_by_name(pa_ctx, default_source_name, muted, success_cb, &state);
    wait_for_operation(o);
    pa_threaded_mainloop_unlock(pa_ml);
    return state.success;
}

// Lists
typedef struct {
    GList *list;
    const char *default_name;
} ListState;

static void sink_info_list_cb(pa_context *c, const pa_sink_info *i, int eol, void *userdata) {
    (void)c;
    if (eol > 0) {
        pa_threaded_mainloop_signal(pa_ml, 0);
        return;
    }
    
    ListState *state = userdata;
    DcAudioDevice *dev = g_new0(DcAudioDevice, 1);
    dev->name = g_strdup(i->name);
    dev->description = g_strdup(i->description);
    dev->volume = pa_volume_to_gint(&i->volume);
    dev->muted = (i->mute != 0);
    dev->is_default = (g_strcmp0(i->name, state->default_name) == 0);
    
    state->list = g_list_append(state->list, dev);
}

GList *dc_audio_service_list_outputs(char **error_message) {
    (void)error_message;
    if (!dc_audio_service_is_ready()) return NULL;
    
    ListState state = {NULL, default_sink_name};
    pa_threaded_mainloop_lock(pa_ml);
    pa_operation *o = pa_context_get_sink_info_list(pa_ctx, sink_info_list_cb, &state);
    wait_for_operation(o);
    pa_threaded_mainloop_unlock(pa_ml);
    
    return state.list;
}

gboolean dc_audio_service_set_default_output(const char *name, char **error_message) {
    if (!dc_audio_service_is_ready()) return FALSE;
    OpState state = {0, FALSE, FALSE, error_message};
    pa_threaded_mainloop_lock(pa_ml);
    pa_operation *o = pa_context_set_default_sink(pa_ctx, name, success_cb, &state);
    wait_for_operation(o);
    pa_threaded_mainloop_unlock(pa_ml);
    return state.success;
}

static void source_info_list_cb(pa_context *c, const pa_source_info *i, int eol, void *userdata) {
    (void)c;
    if (eol > 0) {
        pa_threaded_mainloop_signal(pa_ml, 0);
        return;
    }
    
    if (i->monitor_of_sink != PA_INVALID_INDEX) {
        return;
    }

    ListState *state = userdata;
    DcAudioDevice *dev = g_new0(DcAudioDevice, 1);
    dev->name = g_strdup(i->name);
    dev->description = g_strdup(i->description);
    dev->volume = pa_volume_to_gint(&i->volume);
    dev->muted = (i->mute != 0);
    dev->is_default = (g_strcmp0(i->name, state->default_name) == 0);
    
    state->list = g_list_append(state->list, dev);
}

GList *dc_audio_service_list_inputs(char **error_message) {
    (void)error_message;
    if (!dc_audio_service_is_ready()) return NULL;
    
    ListState state = {NULL, default_source_name};
    pa_threaded_mainloop_lock(pa_ml);
    pa_operation *o = pa_context_get_source_info_list(pa_ctx, source_info_list_cb, &state);
    wait_for_operation(o);
    pa_threaded_mainloop_unlock(pa_ml);
    
    return state.list;
}

gboolean dc_audio_service_set_default_input(const char *name, char **error_message) {
    if (!dc_audio_service_is_ready()) return FALSE;
    OpState state = {0, FALSE, FALSE, error_message};
    pa_threaded_mainloop_lock(pa_ml);
    pa_operation *o = pa_context_set_default_source(pa_ctx, name, success_cb, &state);
    wait_for_operation(o);
    pa_threaded_mainloop_unlock(pa_ml);
    return state.success;
}

gboolean dc_audio_service_get_overamplification(void) {
    return overamplification_enabled;
}

gboolean dc_audio_service_set_overamplification(gboolean enabled, char **error_message) {
    (void)error_message;
    overamplification_enabled = enabled;
    return TRUE;
}

static void sink_input_info_list_cb(pa_context *c, const pa_sink_input_info *i, int eol, void *userdata) {
    (void)c;
    if (eol > 0) {
        pa_threaded_mainloop_signal(pa_ml, 0);
        return;
    }
    
    GList **list = userdata;
    DcAudioAppStream *app = g_new0(DcAudioAppStream, 1);
    app->index = i->index;
    
    const char *name = pa_proplist_gets(i->proplist, PA_PROP_APPLICATION_NAME);
    const char *icon = pa_proplist_gets(i->proplist, PA_PROP_APPLICATION_ICON_NAME);
    
    app->name = g_strdup(name ? name : i->name);
    app->icon = g_strdup(icon);
    app->volume = pa_volume_to_gint(&i->volume);
    app->muted = (i->mute != 0);
    app->sink_name = NULL; // We skip mapping the exact sink for simplicity

    *list = g_list_append(*list, app);
}

GList *dc_audio_service_list_app_streams(char **error_message) {
    (void)error_message;
    if (!dc_audio_service_is_ready()) return NULL;
    
    GList *list = NULL;
    pa_threaded_mainloop_lock(pa_ml);
    pa_operation *o = pa_context_get_sink_input_info_list(pa_ctx, sink_input_info_list_cb, &list);
    wait_for_operation(o);
    pa_threaded_mainloop_unlock(pa_ml);
    
    return list;
}

static void app_volume_set_cb(pa_context *c, const pa_sink_input_info *i, int eol, void *userdata) {
    if (eol > 0) {
        pa_threaded_mainloop_signal(pa_ml, 0);
        return;
    }
    OpState *state = userdata;
    pa_cvolume modified = gint_to_pa_volume(&i->volume, state->param_int);
    pa_operation *o = pa_context_set_sink_input_volume(c, i->index, &modified, NULL, NULL);
    if (o) pa_operation_unref(o);
    state->success = TRUE;
}

gboolean dc_audio_service_set_app_volume(guint32 index, gint volume, char **error_message) {
    if (!dc_audio_service_is_ready()) return FALSE;
    OpState state = {volume, FALSE, FALSE, error_message};
    pa_threaded_mainloop_lock(pa_ml);
    pa_operation *o = pa_context_get_sink_input_info(pa_ctx, index, app_volume_set_cb, &state);
    wait_for_operation(o);
    pa_threaded_mainloop_unlock(pa_ml);
    return state.success;
}

gboolean dc_audio_service_set_app_muted(guint32 index, gboolean muted, char **error_message) {
    if (!dc_audio_service_is_ready()) return FALSE;
    OpState state = {0, muted, FALSE, error_message};
    pa_threaded_mainloop_lock(pa_ml);
    pa_operation *o = pa_context_set_sink_input_mute(pa_ctx, index, muted, success_cb, &state);
    wait_for_operation(o);
    pa_threaded_mainloop_unlock(pa_ml);
    return state.success;
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
    if (device) {
        g_free(device->name);
        g_free(device->description);
        g_free(device);
    }
}

void dc_audio_service_app_stream_free(DcAudioAppStream *app_stream) {
    if (app_stream) {
        g_free(app_stream->name);
        g_free(app_stream->icon);
        g_free(app_stream->sink_name);
        g_free(app_stream);
    }
}
