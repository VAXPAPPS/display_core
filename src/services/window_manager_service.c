#include "services/window_manager_service.h"

static void set_error(char **error_message, const char *message) {
    if (error_message == NULL) {
        return;
    }

    g_free(*error_message);
    *error_message = g_strdup(message);
}

static char *window_manager_config_path(char **error_message) {
    char *config_dir;
    char *path;

    config_dir = g_build_filename(g_get_user_config_dir(), "display-core", NULL);
    if (g_mkdir_with_parents(config_dir, 0755) != 0) {
        g_free(config_dir);
        set_error(error_message, "Failed to create display-core config directory.");
        return NULL;
    }

    path = g_build_filename(config_dir, "window-manager.ini", NULL);
    g_free(config_dir);
    return path;
}

static gboolean run_vaxp_command(const char *command, char **error_message) {
    gboolean success;
    gint exit_status = 0;
    char *stdout_output = NULL;
    char *stderr_output = NULL;
    GError *error = NULL;

    success = g_spawn_command_line_sync(command, &stdout_output, &stderr_output, &exit_status, &error);
    if (!success) {
        set_error(error_message, error != NULL ? error->message : "Failed to execute vaxp command.");
        g_clear_error(&error);
        g_free(stdout_output);
        g_free(stderr_output);
        return FALSE;
    }

    if (exit_status != 0) {
        if (stderr_output != NULL && *stderr_output != '\0') {
            set_error(error_message, stderr_output);
        } else {
            set_error(error_message, "vaxp command returned a non-zero exit status.");
        }
        g_free(stdout_output);
        g_free(stderr_output);
        return FALSE;
    }

    g_free(stdout_output);
    g_free(stderr_output);
    return TRUE;
}

static gboolean apply_boolean_command(const char *key, gboolean value, char **error_message) {
    char *command;
    gboolean success;

    command = g_strdup_printf("vaxp config %s %s", key, value ? "true" : "false");
    success = run_vaxp_command(command, error_message);
    g_free(command);
    return success;
}

static gboolean apply_int_command(const char *key, int value, char **error_message) {
    char *command;
    gboolean success;

    command = g_strdup_printf("vaxp config %s %d", key, value);
    success = run_vaxp_command(command, error_message);
    g_free(command);
    return success;
}

static gboolean apply_double_command(const char *key, double value, char **error_message) {
    char *command;
    gboolean success;

    command = g_strdup_printf("vaxp config %s %.2f", key, value);
    success = run_vaxp_command(command, error_message);
    g_free(command);
    return success;
}

static gboolean apply_string_command(const char *key, const char *value, char **error_message) {
    char *command;
    char *quoted_value;
    gboolean success;

    quoted_value = g_shell_quote(value != NULL ? value : "");
    command = g_strdup_printf("vaxp config %s %s", key, quoted_value);
    success = run_vaxp_command(command, error_message);
    g_free(quoted_value);
    g_free(command);
    return success;
}

DcWindowManagerConfig *dc_window_manager_config_new(void) {
    DcWindowManagerConfig *config;

    config = g_new0(DcWindowManagerConfig, 1);
    config->floating_mode = FALSE;
    config->snap_threshold = 20;
    config->snap_show_preview = TRUE;
    config->border_width = 2;
    config->focused_border_color = g_strdup("#5e81ac");
    config->normal_border_color = g_strdup("#3b4252");
    config->window_gap = 10;
    config->top_padding = 30;
    config->bottom_padding = 0;
    config->focus_opacity = TRUE;
    config->inactive_opacity = 0.85;
    config->active_opacity = 1.0;
    config->desktop_layout = g_strdup("tiled");
    return config;
}

void dc_window_manager_config_free(DcWindowManagerConfig *config) {
    if (config == NULL) {
        return;
    }

    g_free(config->focused_border_color);
    g_free(config->normal_border_color);
    g_free(config->desktop_layout);
    g_free(config);
}

gboolean dc_window_manager_config_load(DcWindowManagerConfig **config, char **error_message) {
    DcWindowManagerConfig *loaded;
    GKeyFile *key_file;
    char *path;
    GError *error = NULL;
    char *value;

    if (config == NULL) {
        set_error(error_message, "Window manager config output is required.");
        return FALSE;
    }

    path = window_manager_config_path(error_message);
    if (path == NULL) {
        return FALSE;
    }

    loaded = dc_window_manager_config_new();
    key_file = g_key_file_new();

    if (!g_file_test(path, G_FILE_TEST_EXISTS)) {
        g_key_file_unref(key_file);
        g_free(path);
        *config = loaded;
        return TRUE;
    }

    if (!g_key_file_load_from_file(key_file, path, G_KEY_FILE_NONE, &error)) {
        set_error(error_message, error != NULL ? error->message : "Failed to read window manager config.");
        g_clear_error(&error);
        g_key_file_unref(key_file);
        g_free(path);
        dc_window_manager_config_free(loaded);
        return FALSE;
    }

    if (g_key_file_has_key(key_file, "window-manager", "floating_mode", NULL)) {
        loaded->floating_mode = g_key_file_get_boolean(key_file, "window-manager", "floating_mode", NULL);
    }
    if (g_key_file_has_key(key_file, "window-manager", "snap_threshold", NULL)) {
        loaded->snap_threshold = g_key_file_get_integer(key_file, "window-manager", "snap_threshold", NULL);
    }
    if (g_key_file_has_key(key_file, "window-manager", "snap_show_preview", NULL)) {
        loaded->snap_show_preview = g_key_file_get_boolean(key_file, "window-manager", "snap_show_preview", NULL);
    }
    if (g_key_file_has_key(key_file, "window-manager", "border_width", NULL)) {
        loaded->border_width = g_key_file_get_integer(key_file, "window-manager", "border_width", NULL);
    }
    value = g_key_file_get_string(key_file, "window-manager", "focused_border_color", NULL);
    if (value != NULL && *value != '\0') {
        g_free(loaded->focused_border_color);
        loaded->focused_border_color = value;
    } else {
        g_free(value);
    }
    value = g_key_file_get_string(key_file, "window-manager", "normal_border_color", NULL);
    if (value != NULL && *value != '\0') {
        g_free(loaded->normal_border_color);
        loaded->normal_border_color = value;
    } else {
        g_free(value);
    }
    if (g_key_file_has_key(key_file, "window-manager", "window_gap", NULL)) {
        loaded->window_gap = g_key_file_get_integer(key_file, "window-manager", "window_gap", NULL);
    }
    if (g_key_file_has_key(key_file, "window-manager", "top_padding", NULL)) {
        loaded->top_padding = g_key_file_get_integer(key_file, "window-manager", "top_padding", NULL);
    }
    if (g_key_file_has_key(key_file, "window-manager", "bottom_padding", NULL)) {
        loaded->bottom_padding = g_key_file_get_integer(key_file, "window-manager", "bottom_padding", NULL);
    }
    if (g_key_file_has_key(key_file, "window-manager", "focus_opacity", NULL)) {
        loaded->focus_opacity = g_key_file_get_boolean(key_file, "window-manager", "focus_opacity", NULL);
    }
    if (g_key_file_has_key(key_file, "window-manager", "inactive_opacity", NULL)) {
        loaded->inactive_opacity = g_key_file_get_double(key_file, "window-manager", "inactive_opacity", NULL);
    }
    if (g_key_file_has_key(key_file, "window-manager", "active_opacity", NULL)) {
        loaded->active_opacity = g_key_file_get_double(key_file, "window-manager", "active_opacity", NULL);
    }
    value = g_key_file_get_string(key_file, "window-manager", "desktop_layout", NULL);
    if (value != NULL && *value != '\0') {
        g_free(loaded->desktop_layout);
        loaded->desktop_layout = value;
    } else {
        g_free(value);
    }

    g_key_file_unref(key_file);
    g_free(path);
    *config = loaded;
    return TRUE;
}

gboolean dc_window_manager_config_save(const DcWindowManagerConfig *config, char **error_message) {
    GKeyFile *key_file;
    char *path;
    char *data;
    gsize data_len;

    if (config == NULL) {
        set_error(error_message, "Window manager config is required.");
        return FALSE;
    }

    path = window_manager_config_path(error_message);
    if (path == NULL) {
        return FALSE;
    }

    key_file = g_key_file_new();
    g_key_file_set_boolean(key_file, "window-manager", "floating_mode", config->floating_mode);
    g_key_file_set_integer(key_file, "window-manager", "snap_threshold", config->snap_threshold);
    g_key_file_set_boolean(key_file, "window-manager", "snap_show_preview", config->snap_show_preview);
    g_key_file_set_integer(key_file, "window-manager", "border_width", config->border_width);
    g_key_file_set_string(key_file, "window-manager", "focused_border_color", config->focused_border_color != NULL ? config->focused_border_color : "#5e81ac");
    g_key_file_set_string(key_file, "window-manager", "normal_border_color", config->normal_border_color != NULL ? config->normal_border_color : "#3b4252");
    g_key_file_set_integer(key_file, "window-manager", "window_gap", config->window_gap);
    g_key_file_set_integer(key_file, "window-manager", "top_padding", config->top_padding);
    g_key_file_set_integer(key_file, "window-manager", "bottom_padding", config->bottom_padding);
    g_key_file_set_boolean(key_file, "window-manager", "focus_opacity", config->focus_opacity);
    g_key_file_set_double(key_file, "window-manager", "inactive_opacity", config->inactive_opacity);
    g_key_file_set_double(key_file, "window-manager", "active_opacity", config->active_opacity);
    g_key_file_set_string(key_file, "window-manager", "desktop_layout", config->desktop_layout != NULL ? config->desktop_layout : "tiled");

    data = g_key_file_to_data(key_file, &data_len, NULL);
    if (!g_file_set_contents(path, data, data_len, NULL)) {
        set_error(error_message, "Failed to save window manager config.");
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

gboolean dc_window_manager_apply_config(const DcWindowManagerConfig *config, char **error_message) {
    char *local_error = NULL;

    if (config == NULL) {
        set_error(error_message, "Window manager config is required.");
        return FALSE;
    }

    if (!apply_boolean_command("floating_mode", config->floating_mode, &local_error) ||
        !apply_int_command("snap_threshold", config->snap_threshold, &local_error) ||
        !apply_boolean_command("snap_show_preview", config->snap_show_preview, &local_error) ||
        !apply_int_command("border_width", config->border_width, &local_error) ||
        !apply_string_command("focused_border_color", config->focused_border_color, &local_error) ||
        !apply_string_command("normal_border_color", config->normal_border_color, &local_error) ||
        !apply_int_command("window_gap", config->window_gap, &local_error) ||
        !apply_int_command("top_padding", config->top_padding, &local_error) ||
        !apply_int_command("bottom_padding", config->bottom_padding, &local_error) ||
        !apply_boolean_command("focus_opacity", config->focus_opacity, &local_error) ||
        !apply_double_command("inactive_opacity", config->inactive_opacity, &local_error) ||
        !apply_double_command("active_opacity", config->active_opacity, &local_error)) {
        set_error(error_message, local_error != NULL ? local_error : "Failed to apply window manager config.");
        g_free(local_error);
        return FALSE;
    }

    g_free(local_error);
    local_error = NULL;
    if (config->desktop_layout != NULL) {
        char *layout_command = g_strdup_printf("vaxp desktop -l %s", config->desktop_layout);
        gboolean success = run_vaxp_command(layout_command, &local_error);
        g_free(layout_command);
        if (!success) {
            set_error(error_message, local_error != NULL ? local_error : "Failed to apply desktop layout.");
            g_free(local_error);
            return FALSE;
        }
    }

    g_free(local_error);
    return TRUE;
}

static gboolean run_single_arg_command(const char *prefix,
                                       const char *value,
                                       char **error_message) {
    char *quoted_value;
    char *command;
    gboolean success;

    if (value == NULL || *value == '\0') {
        set_error(error_message, "A target value is required.");
        return FALSE;
    }

    quoted_value = g_shell_quote(value);
    command = g_strdup_printf("%s %s", prefix, quoted_value);
    success = run_vaxp_command(command, error_message);
    g_free(command);
    g_free(quoted_value);
    return success;
}

gboolean dc_window_manager_focus_node(const char *direction, char **error_message) {
    return run_single_arg_command("vaxp node -f", direction, error_message);
}

gboolean dc_window_manager_swap_node(const char *direction, char **error_message) {
    return run_single_arg_command("vaxp node -s", direction, error_message);
}

gboolean dc_window_manager_set_node_state(const char *state, char **error_message) {
    return run_single_arg_command("vaxp node -t", state, error_message);
}

gboolean dc_window_manager_move_node_to_desktop(const char *target, gboolean follow, char **error_message) {
    char *quoted_target;
    char *command;
    gboolean success;

    if (target == NULL || *target == '\0') {
        set_error(error_message, "Desktop target is required.");
        return FALSE;
    }

    quoted_target = g_shell_quote(target);
    command = g_strdup_printf("vaxp node -d %s%s", quoted_target, follow ? " --follow" : "");
    success = run_vaxp_command(command, error_message);
    g_free(command);
    g_free(quoted_target);
    return success;
}

gboolean dc_window_manager_move_node_to_monitor(const char *target, char **error_message) {
    return run_single_arg_command("vaxp node -m", target, error_message);
}

gboolean dc_window_manager_focus_desktop(const char *target, char **error_message) {
    return run_single_arg_command("vaxp desktop -f", target, error_message);
}

gboolean dc_window_manager_add_desktop(const char *name, char **error_message) {
    return run_single_arg_command("vaxp monitor -a", name, error_message);
}

gboolean dc_window_manager_rename_desktop(const char *name, char **error_message) {
    return run_single_arg_command("vaxp desktop -n", name, error_message);
}

gboolean dc_window_manager_remove_current_desktop(char **error_message) {
    return run_vaxp_command("vaxp desktop -r", error_message);
}

gboolean dc_window_manager_add_rule(const char *app_name,
                                    const char *desktop_target,
                                    const char *state,
                                    char **error_message) {
    GString *command;
    char *quoted_app;
    char *quoted_target = NULL;
    gboolean success;

    if (app_name == NULL || *app_name == '\0') {
        set_error(error_message, "Application name is required for a rule.");
        return FALSE;
    }

    quoted_app = g_shell_quote(app_name);
    command = g_string_new("vaxp rule -a ");
    g_string_append(command, quoted_app);

    if (desktop_target != NULL && *desktop_target != '\0') {
        quoted_target = g_shell_quote(desktop_target);
        g_string_append_printf(command, " desktop=%s", quoted_target);
    }

    if (state != NULL && *state != '\0') {
        g_string_append_printf(command, " state=%s", state);
    }

    success = run_vaxp_command(command->str, error_message);
    g_string_free(command, TRUE);
    g_free(quoted_app);
    g_free(quoted_target);
    return success;
}

gboolean dc_window_manager_remove_rule(const char *app_name, char **error_message) {
    return run_single_arg_command("vaxp rule -r", app_name, error_message);
}

gboolean dc_window_manager_list_rules(char **rules_output, char **error_message) {
    gboolean success;
    gint exit_status = 0;
    char *stdout_output = NULL;
    char *stderr_output = NULL;
    GError *error = NULL;

    if (rules_output == NULL) {
        set_error(error_message, "Rules output buffer is required.");
        return FALSE;
    }

    success = g_spawn_command_line_sync("vaxp rule -l",
                                        &stdout_output,
                                        &stderr_output,
                                        &exit_status,
                                        &error);
    if (!success) {
        set_error(error_message, error != NULL ? error->message : "Failed to list PoisonBlade rules.");
        g_clear_error(&error);
        g_free(stdout_output);
        g_free(stderr_output);
        return FALSE;
    }

    if (exit_status != 0) {
        set_error(error_message,
                  stderr_output != NULL && *stderr_output != '\0'
                    ? stderr_output
                    : "PoisonBlade rule query returned a non-zero exit status.");
        g_free(stdout_output);
        g_free(stderr_output);
        return FALSE;
    }

    *rules_output = stdout_output != NULL ? stdout_output : g_strdup("");
    g_free(stderr_output);
    return TRUE;
}
