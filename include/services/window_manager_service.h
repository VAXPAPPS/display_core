#ifndef DC_WINDOW_MANAGER_SERVICE_H
#define DC_WINDOW_MANAGER_SERVICE_H

#include <gtk/gtk.h>

typedef struct {
    gboolean floating_mode;
    gboolean snap_to_edge;
    int snap_threshold;
    gboolean snap_show_preview;
    int border_width;
    char *focused_border_color;
    char *normal_border_color;
    int window_gap;
    int top_padding;
    int bottom_padding;
    gboolean focus_opacity;
    double inactive_opacity;
    double active_opacity;
    char *desktop_layout;
} DcWindowManagerConfig;

DcWindowManagerConfig *dc_window_manager_config_new(void);
void dc_window_manager_config_free(DcWindowManagerConfig *config);

gboolean dc_window_manager_config_load(DcWindowManagerConfig **config, char **error_message);
gboolean dc_window_manager_config_save(const DcWindowManagerConfig *config, char **error_message);
gboolean dc_window_manager_apply_config(const DcWindowManagerConfig *config, char **error_message);

#endif
