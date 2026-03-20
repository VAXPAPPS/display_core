#include "services/theme_service.h"

#include <gio/gio.h>
#include <pango/pangocairo.h>

#define DC_THEME_CONFIG_DIR "vaxp"
#define DC_THEME_CONFIG_FILE "theme-daemon.ini"
#define DC_THEME_DAEMON_DBUS_NAME "org.vaxp.ThemeDaemon"
#define DC_THEME_DAEMON_DBUS_PATH "/org/vaxp/ThemeDaemon"
#define DC_THEME_DAEMON_DBUS_INTERFACE "org.vaxp.ThemeDaemon"

static void set_error(char **error_message, const char *message) {
    if (error_message == NULL) {
        return;
    }

    g_free(*error_message);
    *error_message = g_strdup(message);
}

static char *theme_config_path(void) {
    return g_build_filename(g_get_user_config_dir(),
                            DC_THEME_CONFIG_DIR,
                            DC_THEME_CONFIG_FILE,
                            NULL);
}

DcThemeConfig *dc_theme_config_new(void) {
    DcThemeConfig *config = g_new0(DcThemeConfig, 1);

    config->gtk_theme = g_strdup("Adwaita");
    config->icon_theme = g_strdup("Adwaita");
    config->cursor_theme = g_strdup("Adwaita");
    config->font_name = g_strdup("Sans 11");
    config->monospace_font = g_strdup("Monospace 10");
    config->interface_mode = g_strdup("system");
    config->cursor_size = 24;
    config->text_scale = 1.0;
    return config;
}

void dc_theme_config_free(DcThemeConfig *config) {
    if (config == NULL) {
        return;
    }

    g_free(config->gtk_theme);
    g_free(config->icon_theme);
    g_free(config->cursor_theme);
    g_free(config->font_name);
    g_free(config->monospace_font);
    g_free(config->interface_mode);
    g_free(config);
}

gboolean dc_theme_config_load(DcThemeConfig **config, char **error_message) {
    GKeyFile *key_file;
    DcThemeConfig *loaded;
    char *path;
    GError *error = NULL;
    char *value;

    if (config == NULL) {
        set_error(error_message, "Theme config output is required.");
        return FALSE;
    }

    loaded = dc_theme_config_new();
    path = theme_config_path();
    key_file = g_key_file_new();

    if (!g_file_test(path, G_FILE_TEST_EXISTS)) {
        g_key_file_unref(key_file);
        g_free(path);
        *config = loaded;
        return TRUE;
    }

    if (!g_key_file_load_from_file(key_file, path, G_KEY_FILE_NONE, &error)) {
        set_error(error_message, error != NULL ? error->message : "Failed to read theme-daemon.ini.");
        g_clear_error(&error);
        g_key_file_unref(key_file);
        g_free(path);
        dc_theme_config_free(loaded);
        return FALSE;
    }

    value = g_key_file_get_string(key_file, "interface", "gtk-theme", NULL);
    if (value != NULL && *value != '\0') {
        g_free(loaded->gtk_theme);
        loaded->gtk_theme = value;
    } else {
        g_free(value);
    }

    value = g_key_file_get_string(key_file, "interface", "icon-theme", NULL);
    if (value != NULL && *value != '\0') {
        g_free(loaded->icon_theme);
        loaded->icon_theme = value;
    } else {
        g_free(value);
    }

    value = g_key_file_get_string(key_file, "interface", "cursor-theme", NULL);
    if (value != NULL && *value != '\0') {
        g_free(loaded->cursor_theme);
        loaded->cursor_theme = value;
    } else {
        g_free(value);
    }

    value = g_key_file_get_string(key_file, "interface", "font-name", NULL);
    if (value != NULL && *value != '\0') {
        g_free(loaded->font_name);
        loaded->font_name = value;
    } else {
        g_free(value);
    }

    value = g_key_file_get_string(key_file, "interface", "interface-mode", NULL);
    if (value != NULL && *value != '\0') {
        g_free(loaded->interface_mode);
        loaded->interface_mode = value;
    } else {
        g_free(value);
    }

    if (g_key_file_has_key(key_file, "interface", "cursor-size", NULL)) {
        loaded->cursor_size = g_key_file_get_integer(key_file, "interface", "cursor-size", NULL);
    }

    if (g_key_file_has_key(key_file, "interface", "text-scaling-factor", NULL)) {
        loaded->text_scale = g_key_file_get_double(key_file, "interface", "text-scaling-factor", NULL);
    }

    g_key_file_unref(key_file);
    g_free(path);
    *config = loaded;
    return TRUE;
}

static gboolean call_method(GDBusProxy *proxy,
                            const char *method,
                            GVariant *parameters,
                            char **error_message) {
    GVariant *result;
    GError *error = NULL;

    result = g_dbus_proxy_call_sync(proxy,
                                    method,
                                    parameters,
                                    G_DBUS_CALL_FLAGS_NONE,
                                    -1,
                                    NULL,
                                    &error);
    if (result == NULL) {
        set_error(error_message, error != NULL ? error->message : "Theme daemon call failed.");
        g_clear_error(&error);
        return FALSE;
    }

    g_variant_unref(result);
    return TRUE;
}

gboolean dc_theme_config_apply(const DcThemeConfig *config, char **error_message) {
    GDBusProxy *proxy;
    GError *error = NULL;
    gboolean success = TRUE;

    if (config == NULL) {
        set_error(error_message, "Theme config is required.");
        return FALSE;
    }

    proxy = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SESSION,
                                          G_DBUS_PROXY_FLAGS_NONE,
                                          NULL,
                                          DC_THEME_DAEMON_DBUS_NAME,
                                          DC_THEME_DAEMON_DBUS_PATH,
                                          DC_THEME_DAEMON_DBUS_INTERFACE,
                                          NULL,
                                          &error);
    if (proxy == NULL) {
        set_error(error_message, error != NULL ? error->message : "Failed to connect to theme daemon.");
        g_clear_error(&error);
        return FALSE;
    }

    success = call_method(proxy, "SetGtkTheme", g_variant_new("(s)", config->gtk_theme), error_message) &&
              call_method(proxy, "SetIconTheme", g_variant_new("(s)", config->icon_theme), error_message) &&
              call_method(proxy, "SetCursorTheme", g_variant_new("(s)", config->cursor_theme), error_message) &&
              call_method(proxy, "SetCursorSize", g_variant_new("(i)", config->cursor_size), error_message) &&
              call_method(proxy, "SetInterfaceMode", g_variant_new("(s)", config->interface_mode), error_message) &&
              call_method(proxy, "SetFontName", g_variant_new("(s)", config->font_name), error_message) &&
              call_method(proxy, "SetTextScalingFactor", g_variant_new("(d)", config->text_scale), error_message) &&
              call_method(proxy, "Reload", NULL, error_message);

    g_object_unref(proxy);
    return success;
}

static gboolean directory_exists(const char *path) {
    return g_file_test(path, G_FILE_TEST_IS_DIR);
}

static gboolean file_exists(const char *path) {
    return g_file_test(path, G_FILE_TEST_EXISTS);
}

static gboolean is_gtk_theme_dir(const char *base_dir, const char *name) {
    char *gtk3 = g_build_filename(base_dir, name, "gtk-3.0", NULL);
    char *gtk4 = g_build_filename(base_dir, name, "gtk-4.0", NULL);
    gboolean found = directory_exists(gtk3) || directory_exists(gtk4);

    g_free(gtk3);
    g_free(gtk4);
    return found;
}

static gboolean is_icon_theme_dir(const char *base_dir, const char *name) {
    char *index_theme = g_build_filename(base_dir, name, "index.theme", NULL);
    gboolean found = file_exists(index_theme);

    g_free(index_theme);
    return found;
}

static gboolean is_cursor_theme_dir(const char *base_dir, const char *name) {
    char *cursor_dir = g_build_filename(base_dir, name, "cursors", NULL);
    gboolean found = directory_exists(cursor_dir);

    g_free(cursor_dir);
    return found;
}

static gint compare_strings(gconstpointer a, gconstpointer b) {
    const char *left = *((const char * const *) a);
    const char *right = *((const char * const *) b);
    return g_strcmp0(left, right);
}

static GStrv collect_directory_items(const char * const *roots,
                                     gboolean (*match_func)(const char *, const char *)) {
    GHashTable *seen;
    GPtrArray *items;
    guint i;

    seen = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
    items = g_ptr_array_new_with_free_func(g_free);

    for (i = 0; roots[i] != NULL; i++) {
        GDir *dir;
        const char *name;

        dir = g_dir_open(roots[i], 0, NULL);
        if (dir == NULL) {
            continue;
        }

        while ((name = g_dir_read_name(dir)) != NULL) {
            if (!match_func(roots[i], name) || g_hash_table_contains(seen, name)) {
                continue;
            }

            g_hash_table_add(seen, g_strdup(name));
            g_ptr_array_add(items, g_strdup(name));
        }

        g_dir_close(dir);
    }

    g_hash_table_unref(seen);
    g_ptr_array_sort(items, compare_strings);
    g_ptr_array_add(items, NULL);
    return (GStrv) g_ptr_array_free(items, FALSE);
}

gboolean dc_theme_service_list_gtk_themes(GStrv *themes, char **error_message) {
    char *user_dir;
    const char *roots[3];

    if (themes == NULL) {
        set_error(error_message, "GTK themes output is required.");
        return FALSE;
    }

    user_dir = g_build_filename(g_get_home_dir(), ".themes", NULL);
    roots[0] = user_dir;
    roots[1] = "/usr/share/themes";
    roots[2] = NULL;
    *themes = collect_directory_items(roots, is_gtk_theme_dir);
    g_free(user_dir);
    return TRUE;
}

gboolean dc_theme_service_list_icon_themes(GStrv *themes, char **error_message) {
    char *user_dir;
    const char *roots[3];

    if (themes == NULL) {
        set_error(error_message, "Icon themes output is required.");
        return FALSE;
    }

    user_dir = g_build_filename(g_get_home_dir(), ".icons", NULL);
    roots[0] = user_dir;
    roots[1] = "/usr/share/icons";
    roots[2] = NULL;
    *themes = collect_directory_items(roots, is_icon_theme_dir);
    g_free(user_dir);
    return TRUE;
}

gboolean dc_theme_service_list_cursor_themes(GStrv *themes, char **error_message) {
    char *user_dir;
    const char *roots[3];

    if (themes == NULL) {
        set_error(error_message, "Cursor themes output is required.");
        return FALSE;
    }

    user_dir = g_build_filename(g_get_home_dir(), ".icons", NULL);
    roots[0] = user_dir;
    roots[1] = "/usr/share/icons";
    roots[2] = NULL;
    *themes = collect_directory_items(roots, is_cursor_theme_dir);
    g_free(user_dir);
    return TRUE;
}

gboolean dc_theme_service_list_fonts(GStrv *fonts, char **error_message) {
    PangoFontMap *font_map;
    PangoFontFamily **families = NULL;
    int n_families = 0;
    GPtrArray *items;
    int i;

    if (fonts == NULL) {
        set_error(error_message, "Fonts output is required.");
        return FALSE;
    }

    font_map = pango_cairo_font_map_get_default();
    pango_font_map_list_families(font_map, &families, &n_families);

    items = g_ptr_array_new_with_free_func(g_free);
    for (i = 0; i < n_families; i++) {
        const char *family_name = pango_font_family_get_name(families[i]);
        g_ptr_array_add(items, g_strdup_printf("%s 11", family_name));
    }

    g_ptr_array_sort(items, compare_strings);
    g_ptr_array_add(items, NULL);
    g_free(families);
    *fonts = (GStrv) g_ptr_array_free(items, FALSE);
    return TRUE;
}
