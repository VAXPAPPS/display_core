#ifndef DC_THEME_SERVICE_H
#define DC_THEME_SERVICE_H

#include <gtk/gtk.h>

typedef struct {
    char *gtk_theme;
    char *icon_theme;
    char *cursor_theme;
    char *font_name;
    char *monospace_font;
    char *interface_mode;
    int cursor_size;
    double text_scale;
} DcThemeConfig;

DcThemeConfig *dc_theme_config_new(void);
void dc_theme_config_free(DcThemeConfig *config);

gboolean dc_theme_config_load(DcThemeConfig **config, char **error_message);
gboolean dc_theme_config_apply(const DcThemeConfig *config, char **error_message);

gboolean dc_theme_service_list_gtk_themes(GStrv *themes, char **error_message);
gboolean dc_theme_service_list_icon_themes(GStrv *themes, char **error_message);
gboolean dc_theme_service_list_cursor_themes(GStrv *themes, char **error_message);
gboolean dc_theme_service_list_fonts(GStrv *fonts, char **error_message);

#endif
