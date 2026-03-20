#include "services/venom_config_service.h"

typedef enum {
    DC_VALUE_BOOL,
    DC_VALUE_INT,
    DC_VALUE_DOUBLE,
    DC_VALUE_STRING
} DcValueType;

static void set_error(char **error_message, const char *message) {
    if (error_message == NULL) {
        return;
    }

    g_free(*error_message);
    *error_message = g_strdup(message);
}

DcVenomConfig *dc_venom_config_new(void) {
    DcVenomConfig *config = g_new0(DcVenomConfig, 1);

    config->shadow = TRUE;
    config->shadow_radius = 25;
    config->shadow_opacity = 1.0;
    config->shadow_red = 0.74;
    config->shadow_green = 0.26;
    config->shadow_blue = 0.36;
    config->fading = TRUE;
    config->active_opacity = 1.0;
    config->inactive_opacity = 1.0;
    config->corner_radius = 12;
    config->detect_rounded_corners = TRUE;
    config->blur_method = g_strdup("dual_kawase");
    config->blur_strength = 5;
    config->blur_background = TRUE;
    config->blur_background_frame = FALSE;
    config->backend = g_strdup("glx");
    config->vsync = TRUE;
    config->use_damage = TRUE;
    return config;
}

void dc_venom_config_free(DcVenomConfig *config) {
    if (config == NULL) {
        return;
    }

    g_free(config->blur_method);
    g_free(config->backend);
    g_free(config);
}

static gboolean parse_bool_value(const char *content, const char *key, gboolean *value) {
    GRegex *regex;
    GMatchInfo *match_info = NULL;
    char *pattern;
    char *captured = NULL;
    gboolean found = FALSE;

    pattern = g_strdup_printf("(?m)^\\s*%s\\s*=\\s*(true|false)\\s*;", key);
    regex = g_regex_new(pattern, G_REGEX_CASELESS, 0, NULL);
    if (g_regex_match(regex, content, 0, &match_info) && g_match_info_matches(match_info)) {
        captured = g_match_info_fetch(match_info, 1);
        *value = g_ascii_strcasecmp(captured, "true") == 0;
        found = TRUE;
    }

    g_free(captured);
    if (match_info != NULL) {
        g_match_info_free(match_info);
    }
    g_regex_unref(regex);
    g_free(pattern);
    return found;
}

static gboolean parse_int_value(const char *content, const char *key, int *value) {
    GRegex *regex;
    GMatchInfo *match_info = NULL;
    char *pattern;
    char *captured = NULL;
    gboolean found = FALSE;

    pattern = g_strdup_printf("(?m)^\\s*%s\\s*=\\s*(-?[0-9]+)\\s*;", key);
    regex = g_regex_new(pattern, 0, 0, NULL);
    if (g_regex_match(regex, content, 0, &match_info) && g_match_info_matches(match_info)) {
        captured = g_match_info_fetch(match_info, 1);
        *value = (int) g_ascii_strtoll(captured, NULL, 10);
        found = TRUE;
    }

    g_free(captured);
    if (match_info != NULL) {
        g_match_info_free(match_info);
    }
    g_regex_unref(regex);
    g_free(pattern);
    return found;
}

static gboolean parse_double_value(const char *content, const char *key, double *value) {
    GRegex *regex;
    GMatchInfo *match_info = NULL;
    char *pattern;
    char *captured = NULL;
    gboolean found = FALSE;

    pattern = g_strdup_printf("(?m)^\\s*%s\\s*=\\s*(-?[0-9]+(?:\\.[0-9]+)?)\\s*;", key);
    regex = g_regex_new(pattern, 0, 0, NULL);
    if (g_regex_match(regex, content, 0, &match_info) && g_match_info_matches(match_info)) {
        captured = g_match_info_fetch(match_info, 1);
        *value = g_ascii_strtod(captured, NULL);
        found = TRUE;
    }

    g_free(captured);
    if (match_info != NULL) {
        g_match_info_free(match_info);
    }
    g_regex_unref(regex);
    g_free(pattern);
    return found;
}

static gboolean parse_string_value(const char *content, const char *key, char **value) {
    GRegex *regex;
    GMatchInfo *match_info = NULL;
    char *pattern;
    char *captured = NULL;
    gboolean found = FALSE;

    pattern = g_strdup_printf("(?m)^\\s*%s\\s*=\\s*\"([^\"]+)\"\\s*;", key);
    regex = g_regex_new(pattern, 0, 0, NULL);
    if (g_regex_match(regex, content, 0, &match_info) && g_match_info_matches(match_info)) {
        captured = g_match_info_fetch(match_info, 1);
        g_free(*value);
        *value = g_strdup(captured);
        found = TRUE;
    }

    g_free(captured);
    if (match_info != NULL) {
        g_match_info_free(match_info);
    }
    g_regex_unref(regex);
    g_free(pattern);
    return found;
}

gboolean dc_venom_config_load(const char *path, DcVenomConfig **config, char **error_message) {
    char *content = NULL;
    gsize length = 0;
    DcVenomConfig *loaded;

    if (!g_file_get_contents(path, &content, &length, NULL)) {
        set_error(error_message, "Failed to read venom.conf.");
        return FALSE;
    }

    loaded = dc_venom_config_new();
    parse_bool_value(content, "shadow", &loaded->shadow);
    parse_int_value(content, "shadow-radius", &loaded->shadow_radius);
    parse_double_value(content, "shadow-opacity", &loaded->shadow_opacity);
    parse_double_value(content, "shadow-red", &loaded->shadow_red);
    parse_double_value(content, "shadow-green", &loaded->shadow_green);
    parse_double_value(content, "shadow-blue", &loaded->shadow_blue);
    parse_bool_value(content, "fading", &loaded->fading);
    parse_double_value(content, "active-opacity", &loaded->active_opacity);
    parse_double_value(content, "inactive-opacity", &loaded->inactive_opacity);
    parse_int_value(content, "corner-radius", &loaded->corner_radius);
    parse_bool_value(content, "detect-rounded-corners", &loaded->detect_rounded_corners);
    parse_string_value(content, "blur-method", &loaded->blur_method);
    parse_int_value(content, "blur-strength", &loaded->blur_strength);
    parse_bool_value(content, "blur-background", &loaded->blur_background);
    parse_bool_value(content, "blur-background-frame", &loaded->blur_background_frame);
    parse_string_value(content, "backend", &loaded->backend);
    parse_bool_value(content, "vsync", &loaded->vsync);
    parse_bool_value(content, "use-damage", &loaded->use_damage);

    g_free(content);
    *config = loaded;
    return TRUE;
}

static char *replace_value(const char *content,
                           const char *key,
                           const char *replacement,
                           DcValueType type,
                           gboolean replace_all) {
    GRegex *regex;
    char *pattern;
    char *replaced;
    char *newline;
    GError *error = NULL;

    if (type == DC_VALUE_STRING) {
        pattern = g_strdup_printf("(?m)^\\s*%s\\s*=\\s*\"[^\"]+\"\\s*;", key);
    } else if (type == DC_VALUE_DOUBLE) {
        pattern = g_strdup_printf("(?m)^\\s*%s\\s*=\\s*-?[0-9]+(?:\\.[0-9]+)?\\s*;", key);
    } else if (type == DC_VALUE_INT) {
        pattern = g_strdup_printf("(?m)^\\s*%s\\s*=\\s*-?[0-9]+\\s*;", key);
    } else {
        pattern = g_strdup_printf("(?m)^\\s*%s\\s*=\\s*(true|false)\\s*;", key);
    }

    regex = g_regex_new(pattern, G_REGEX_CASELESS, 0, NULL);
    if (type == DC_VALUE_STRING) {
        newline = g_strdup_printf("%s = \"%s\";", key, replacement);
    } else {
        newline = g_strdup_printf("%s = %s;", key, replacement);
    }

    (void) replace_all;
    replaced = g_regex_replace(regex, content, -1, 0, newline, 0, &error);

    if (error != NULL) {
        g_error_free(error);
        g_regex_unref(regex);
        g_free(pattern);
        g_free(newline);
        return NULL;
    }

    if (g_strcmp0(replaced, content) == 0) {
        char *appended = g_strconcat(content, "\n", newline, "\n", NULL);
        g_free(replaced);
        replaced = appended;
    }

    g_regex_unref(regex);
    g_free(pattern);
    g_free(newline);
    return replaced;
}

gboolean dc_venom_config_save(const char *path, const DcVenomConfig *config, char **error_message) {
    char *content = NULL;
    char *updated = NULL;
    char *next = NULL;
    char buffer[64];

#define DC_REPLACE_OR_FAIL(expr)       \
    do {                               \
        next = (expr);                 \
        g_free(updated);               \
        updated = next;                \
        if (updated == NULL) {         \
            set_error(error_message, "Failed to update venom.conf values."); \
            return FALSE;              \
        }                              \
    } while (0)

    if (!g_file_get_contents(path, &content, NULL, NULL)) {
        set_error(error_message, "Failed to read venom.conf before saving.");
        return FALSE;
    }

    updated = replace_value(content, "shadow", config->shadow ? "true" : "false", DC_VALUE_BOOL, FALSE);
    g_free(content);
    if (updated == NULL) {
        set_error(error_message, "Failed to update venom.conf values.");
        return FALSE;
    }

    g_snprintf(buffer, sizeof(buffer), "%d", config->shadow_radius);
    DC_REPLACE_OR_FAIL(replace_value(updated, "shadow-radius", buffer, DC_VALUE_INT, FALSE));

    g_snprintf(buffer, sizeof(buffer), "%.2f", config->shadow_opacity);
    DC_REPLACE_OR_FAIL(replace_value(updated, "shadow-opacity", buffer, DC_VALUE_DOUBLE, FALSE));

    g_snprintf(buffer, sizeof(buffer), "%.2f", config->shadow_red);
    DC_REPLACE_OR_FAIL(replace_value(updated, "shadow-red", buffer, DC_VALUE_DOUBLE, FALSE));

    g_snprintf(buffer, sizeof(buffer), "%.2f", config->shadow_green);
    DC_REPLACE_OR_FAIL(replace_value(updated, "shadow-green", buffer, DC_VALUE_DOUBLE, FALSE));

    g_snprintf(buffer, sizeof(buffer), "%.2f", config->shadow_blue);
    DC_REPLACE_OR_FAIL(replace_value(updated, "shadow-blue", buffer, DC_VALUE_DOUBLE, FALSE));

    DC_REPLACE_OR_FAIL(replace_value(updated, "fading", config->fading ? "true" : "false", DC_VALUE_BOOL, FALSE));

    g_snprintf(buffer, sizeof(buffer), "%.2f", config->active_opacity);
    DC_REPLACE_OR_FAIL(replace_value(updated, "active-opacity", buffer, DC_VALUE_DOUBLE, FALSE));

    g_snprintf(buffer, sizeof(buffer), "%.2f", config->inactive_opacity);
    DC_REPLACE_OR_FAIL(replace_value(updated, "inactive-opacity", buffer, DC_VALUE_DOUBLE, FALSE));

    g_snprintf(buffer, sizeof(buffer), "%d", config->corner_radius);
    DC_REPLACE_OR_FAIL(replace_value(updated, "corner-radius", buffer, DC_VALUE_INT, TRUE));

    DC_REPLACE_OR_FAIL(replace_value(updated,
                                     "detect-rounded-corners",
                                     config->detect_rounded_corners ? "true" : "false",
                                     DC_VALUE_BOOL,
                                     FALSE));

    DC_REPLACE_OR_FAIL(replace_value(updated, "blur-method", config->blur_method, DC_VALUE_STRING, FALSE));

    g_snprintf(buffer, sizeof(buffer), "%d", config->blur_strength);
    DC_REPLACE_OR_FAIL(replace_value(updated, "blur-strength", buffer, DC_VALUE_INT, FALSE));

    DC_REPLACE_OR_FAIL(replace_value(updated,
                                     "blur-background",
                                     config->blur_background ? "true" : "false",
                                     DC_VALUE_BOOL,
                                     FALSE));

    DC_REPLACE_OR_FAIL(replace_value(updated,
                                     "blur-background-frame",
                                     config->blur_background_frame ? "true" : "false",
                                     DC_VALUE_BOOL,
                                     FALSE));

    DC_REPLACE_OR_FAIL(replace_value(updated, "backend", config->backend, DC_VALUE_STRING, FALSE));

    DC_REPLACE_OR_FAIL(replace_value(updated, "vsync", config->vsync ? "true" : "false", DC_VALUE_BOOL, FALSE));

    DC_REPLACE_OR_FAIL(replace_value(updated, "use-damage", config->use_damage ? "true" : "false", DC_VALUE_BOOL, FALSE));

    if (!g_file_set_contents(path, updated, -1, NULL)) {
        set_error(error_message, "Failed to save venom.conf.");
        g_free(updated);
        return FALSE;
    }

    g_free(updated);
    return TRUE;

#undef DC_REPLACE_OR_FAIL
}
