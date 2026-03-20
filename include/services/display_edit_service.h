#ifndef DC_DISPLAY_EDIT_SERVICE_H
#define DC_DISPLAY_EDIT_SERVICE_H

#include <gtk/gtk.h>

typedef struct {
    gboolean night_light_enabled;
    int night_light_temperature;
    char *night_light_schedule;
    int night_light_custom_start_hour;
    int night_light_custom_end_hour;
    gboolean adaptive_brightness;
    double gamma;
    int vibrance;
} DcDisplayEditConfig;

DcDisplayEditConfig *dc_display_edit_config_new(void);
void dc_display_edit_config_free(DcDisplayEditConfig *config);

gboolean dc_display_edit_config_load(DcDisplayEditConfig **config, char **error_message);
gboolean dc_display_edit_config_save(const DcDisplayEditConfig *config, char **error_message);

#endif
