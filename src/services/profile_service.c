#include "services/profile_service.h"

static char *profiles_path(char **error_message) {
    char *config_dir;
    char *app_dir;
    char *path;

    config_dir = g_build_filename(g_get_user_config_dir(), "display-core", NULL);
    if (g_mkdir_with_parents(config_dir, 0755) != 0) {
        g_free(config_dir);
        if (error_message != NULL) {
            *error_message = g_strdup("Failed to create config directory.");
        }
        return NULL;
    }

    app_dir = config_dir;
    path = g_build_filename(app_dir, "profiles.ini", NULL);
    g_free(app_dir);
    return path;
}

static void set_error(char **error_message, const char *message) {
    if (error_message == NULL) {
        return;
    }

    g_free(*error_message);
    *error_message = g_strdup(message);
}

gboolean dc_profile_service_save(const char *profile_name, GPtrArray *configs, char **error_message) {
    GKeyFile *key_file;
    char *path;
    char *data;
    gsize data_len;
    guint i;

    if (profile_name == NULL || *profile_name == '\0') {
        set_error(error_message, "Profile name is required.");
        return FALSE;
    }

    path = profiles_path(error_message);
    if (path == NULL) {
        return FALSE;
    }

    key_file = g_key_file_new();
    g_key_file_load_from_file(key_file, path, G_KEY_FILE_NONE, NULL);

    for (i = 0; i < configs->len; i++) {
        DcDisplayConfig *config = g_ptr_array_index(configs, i);
        char *group = g_strdup_printf("%s:%s",
                                      profile_name,
                                      config->output_name != NULL ? config->output_name : "unknown");

        g_key_file_set_string(key_file, group, "output_name", config->output_name != NULL ? config->output_name : "");
        g_key_file_set_boolean(key_file, group, "enabled", config->enabled);
        g_key_file_set_boolean(key_file, group, "primary", config->primary);
        g_key_file_set_uint64(key_file, group, "mode", (guint64) config->mode);
        g_key_file_set_string(key_file, group, "rotation", dc_rotation_to_id(config->rotation));
        g_key_file_set_integer(key_file, group, "x", config->x);
        g_key_file_set_integer(key_file, group, "y", config->y);
        g_free(group);
    }

    data = g_key_file_to_data(key_file, &data_len, NULL);
    if (!g_file_set_contents(path, data, data_len, NULL)) {
        set_error(error_message, "Failed to save profile file.");
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

gboolean dc_profile_service_load(const char *profile_name, GPtrArray **configs, char **error_message) {
    GKeyFile *key_file;
    char *path;
    gsize length;
    char **groups;
    guint i;
    GPtrArray *loaded_configs;

    if (profile_name == NULL || *profile_name == '\0') {
        set_error(error_message, "Profile name is required.");
        return FALSE;
    }

    path = profiles_path(error_message);
    if (path == NULL) {
        return FALSE;
    }

    key_file = g_key_file_new();
    if (!g_key_file_load_from_file(key_file, path, G_KEY_FILE_NONE, NULL)) {
        set_error(error_message, "No saved profiles were found yet.");
        g_key_file_unref(key_file);
        g_free(path);
        return FALSE;
    }

    loaded_configs = g_ptr_array_new_with_free_func((GDestroyNotify) dc_display_config_free);
    groups = g_key_file_get_groups(key_file, &length);

    for (i = 0; i < length; i++) {
        if (g_str_has_prefix(groups[i], profile_name) && groups[i][strlen(profile_name)] == ':') {
            DcDisplayConfig *config = dc_display_config_new();
            guint64 mode_value;
            char *rotation_id;

            config->output_name = g_key_file_get_string(key_file, groups[i], "output_name", NULL);
            config->enabled = g_key_file_get_boolean(key_file, groups[i], "enabled", NULL);
            config->primary = g_key_file_get_boolean(key_file, groups[i], "primary", NULL);
            mode_value = g_key_file_get_uint64(key_file, groups[i], "mode", NULL);
            config->mode = (RRMode) mode_value;
            rotation_id = g_key_file_get_string(key_file, groups[i], "rotation", NULL);
            config->rotation = dc_rotation_from_id(rotation_id);
            config->x = g_key_file_get_integer(key_file, groups[i], "x", NULL);
            config->y = g_key_file_get_integer(key_file, groups[i], "y", NULL);

            g_free(rotation_id);
            g_ptr_array_add(loaded_configs, config);
        }
    }

    g_strfreev(groups);
    g_key_file_unref(key_file);
    g_free(path);

    if (loaded_configs->len == 0) {
        g_ptr_array_free(loaded_configs, TRUE);
        set_error(error_message, "Profile not found.");
        return FALSE;
    }

    *configs = loaded_configs;
    return TRUE;
}

gboolean dc_profile_service_list(GStrv *profile_names, char **error_message) {
    GKeyFile *key_file;
    char *path;
    gsize group_count;
    char **groups;
    GPtrArray *names;
    guint i;

    path = profiles_path(error_message);
    if (path == NULL) {
        return FALSE;
    }

    key_file = g_key_file_new();
    if (!g_key_file_load_from_file(key_file, path, G_KEY_FILE_NONE, NULL)) {
        *profile_names = g_new0(char *, 1);
        g_key_file_unref(key_file);
        g_free(path);
        return TRUE;
    }

    groups = g_key_file_get_groups(key_file, &group_count);
    names = g_ptr_array_new_with_free_func(g_free);

    for (i = 0; i < group_count; i++) {
        char **parts = g_strsplit(groups[i], ":", 2);
        gboolean exists = FALSE;
        guint j;

        for (j = 0; j < names->len; j++) {
            if (g_strcmp0(g_ptr_array_index(names, j), parts[0]) == 0) {
                exists = TRUE;
                break;
            }
        }

        if (!exists && parts[0] != NULL && *parts[0] != '\0') {
            g_ptr_array_add(names, g_strdup(parts[0]));
        }

        g_strfreev(parts);
    }

    g_ptr_array_add(names, NULL);
    *profile_names = (GStrv) g_ptr_array_free(names, FALSE);

    g_strfreev(groups);
    g_key_file_unref(key_file);
    g_free(path);
    return TRUE;
}
