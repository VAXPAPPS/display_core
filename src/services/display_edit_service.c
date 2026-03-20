#include "services/display_edit_service.h"

static void set_error(char **error_message, const char *message) {
    if (error_message == NULL) {
        return;
    }

    g_free(*error_message);
    *error_message = g_strdup(message);
}

static char *display_edit_config_path(char **error_message) {
    char *config_dir;
    char *path;

    config_dir = g_build_filename(g_get_user_config_dir(), "display-core", NULL);
    if (g_mkdir_with_parents(config_dir, 0755) != 0) {
        g_free(config_dir);
        set_error(error_message, "Failed to create display-core config directory.");
        return NULL;
    }

    path = g_build_filename(config_dir, "display-edit.ini", NULL);
    g_free(config_dir);
    return path;
}

DcDisplayEditConfig *dc_display_edit_config_new(void) {
    DcDisplayEditConfig *config;

    config = g_new0(DcDisplayEditConfig, 1);
    config->night_light_enabled = FALSE;
    config->night_light_temperature = 4200;
    config->night_light_schedule = g_strdup("sunset");
    config->night_light_custom_start_hour = 21;
    config->night_light_custom_end_hour = 6;
    config->vrr_enabled = FALSE;
    config->adaptive_brightness = FALSE;
    config->gamma = 1.0;
    config->vibrance = 18;
    return config;
}

void dc_display_edit_config_free(DcDisplayEditConfig *config) {
    if (config == NULL) {
        return;
    }

    g_free(config->night_light_schedule);
    g_free(config);
}

gboolean dc_display_edit_config_load(DcDisplayEditConfig **config, char **error_message) {
    DcDisplayEditConfig *loaded;
    GKeyFile *key_file;
    char *path;
    GError *error = NULL;
    char *schedule;

    if (config == NULL) {
        set_error(error_message, "Display Edit config output is required.");
        return FALSE;
    }

    path = display_edit_config_path(error_message);
    if (path == NULL) {
        return FALSE;
    }

    loaded = dc_display_edit_config_new();
    key_file = g_key_file_new();

    if (!g_file_test(path, G_FILE_TEST_EXISTS)) {
        g_key_file_unref(key_file);
        g_free(path);
        *config = loaded;
        return TRUE;
    }

    if (!g_key_file_load_from_file(key_file, path, G_KEY_FILE_NONE, &error)) {
        set_error(error_message, error != NULL ? error->message : "Failed to read display edit config.");
        g_clear_error(&error);
        g_key_file_unref(key_file);
        g_free(path);
        dc_display_edit_config_free(loaded);
        return FALSE;
    }

    if (g_key_file_has_key(key_file, "display-edit", "night_light_enabled", NULL)) {
        loaded->night_light_enabled = g_key_file_get_boolean(key_file, "display-edit", "night_light_enabled", NULL);
    }
    if (g_key_file_has_key(key_file, "display-edit", "night_light_temperature", NULL)) {
        loaded->night_light_temperature = g_key_file_get_integer(key_file, "display-edit", "night_light_temperature", NULL);
    }
    schedule = NULL;
    if (g_key_file_has_key(key_file, "display-edit", "night_light_schedule", NULL)) {
        schedule = g_key_file_get_string(key_file, "display-edit", "night_light_schedule", NULL);
    }
    if (schedule != NULL && *schedule != '\0') {
        g_free(loaded->night_light_schedule);
        loaded->night_light_schedule = schedule;
    } else {
        g_free(schedule);
    }
    if (g_key_file_has_key(key_file, "display-edit", "adaptive_brightness", NULL)) {
        loaded->adaptive_brightness = g_key_file_get_boolean(key_file, "display-edit", "adaptive_brightness", NULL);
    }
    if (g_key_file_has_key(key_file, "display-edit", "night_light_custom_start_hour", NULL)) {
        loaded->night_light_custom_start_hour = g_key_file_get_integer(key_file, "display-edit", "night_light_custom_start_hour", NULL);
    }
    if (g_key_file_has_key(key_file, "display-edit", "night_light_custom_end_hour", NULL)) {
        loaded->night_light_custom_end_hour = g_key_file_get_integer(key_file, "display-edit", "night_light_custom_end_hour", NULL);
    }
    if (g_key_file_has_key(key_file, "display-edit", "vrr_enabled", NULL)) {
        loaded->vrr_enabled = g_key_file_get_boolean(key_file, "display-edit", "vrr_enabled", NULL);
    }
    if (g_key_file_has_key(key_file, "display-edit", "gamma", NULL)) {
        loaded->gamma = g_key_file_get_double(key_file, "display-edit", "gamma", NULL);
    }
    if (g_key_file_has_key(key_file, "display-edit", "vibrance", NULL)) {
        loaded->vibrance = g_key_file_get_integer(key_file, "display-edit", "vibrance", NULL);
    }

    g_key_file_unref(key_file);
    g_free(path);
    *config = loaded;
    return TRUE;
}

gboolean dc_display_edit_config_save(const DcDisplayEditConfig *config, char **error_message) {
    GKeyFile *key_file;
    char *path;
    char *data;
    gsize data_len;

    if (config == NULL) {
        set_error(error_message, "Display Edit config is required.");
        return FALSE;
    }

    path = display_edit_config_path(error_message);
    if (path == NULL) {
        return FALSE;
    }

    key_file = g_key_file_new();
    g_key_file_set_boolean(key_file, "display-edit", "night_light_enabled", config->night_light_enabled);
    g_key_file_set_integer(key_file, "display-edit", "night_light_temperature", config->night_light_temperature);
    g_key_file_set_string(key_file, "display-edit", "night_light_schedule", config->night_light_schedule != NULL ? config->night_light_schedule : "sunset");
    g_key_file_set_integer(key_file, "display-edit", "night_light_custom_start_hour", config->night_light_custom_start_hour);
    g_key_file_set_integer(key_file, "display-edit", "night_light_custom_end_hour", config->night_light_custom_end_hour);
    g_key_file_set_boolean(key_file, "display-edit", "vrr_enabled", config->vrr_enabled);
    g_key_file_set_boolean(key_file, "display-edit", "adaptive_brightness", config->adaptive_brightness);
    g_key_file_set_double(key_file, "display-edit", "gamma", config->gamma);
    g_key_file_set_integer(key_file, "display-edit", "vibrance", config->vibrance);

    data = g_key_file_to_data(key_file, &data_len, NULL);
    if (!g_file_set_contents(path, data, data_len, NULL)) {
        set_error(error_message, "Failed to save display edit config.");
        g_free(data);
        g_key_file_unref(key_file);
        g_free(path);
        return FALSE;
    }

    g_free(data);
    g_key_file_unref(key_file);
    g_free(path);
    return TRUE;
}
