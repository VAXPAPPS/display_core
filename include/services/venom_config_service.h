#ifndef DC_VENOM_CONFIG_SERVICE_H
#define DC_VENOM_CONFIG_SERVICE_H

#include <gtk/gtk.h>

typedef struct {
    gboolean shadow;
    int shadow_radius;
    double shadow_opacity;
    double shadow_red;
    double shadow_green;
    double shadow_blue;
    gboolean fading;
    double active_opacity;
    double inactive_opacity;
    int corner_radius;
    gboolean detect_rounded_corners;
    char *blur_method;
    int blur_strength;
    gboolean blur_background;
    gboolean blur_background_frame;
    char *backend;
    gboolean vsync;
    gboolean use_damage;
} DcVenomConfig;

DcVenomConfig *dc_venom_config_new(void);
void dc_venom_config_free(DcVenomConfig *config);

gboolean dc_venom_config_load(const char *path, DcVenomConfig **config, char **error_message);
gboolean dc_venom_config_save(const char *path, const DcVenomConfig *config, char **error_message);

#endif
