#ifndef DC_WINDOW_MANAGER_SERVICE_H
#define DC_WINDOW_MANAGER_SERVICE_H

#include <gtk/gtk.h>

typedef struct {
    gboolean floating_mode;
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
gboolean dc_window_manager_focus_node(const char *direction, char **error_message);
gboolean dc_window_manager_swap_node(const char *direction, char **error_message);
gboolean dc_window_manager_set_node_state(const char *state, char **error_message);
gboolean dc_window_manager_move_node_to_desktop(const char *target, gboolean follow, char **error_message);
gboolean dc_window_manager_move_node_to_monitor(const char *target, char **error_message);
gboolean dc_window_manager_focus_desktop(const char *target, char **error_message);
gboolean dc_window_manager_add_desktop(const char *name, char **error_message);
gboolean dc_window_manager_rename_desktop(const char *name, char **error_message);
gboolean dc_window_manager_remove_current_desktop(char **error_message);
gboolean dc_window_manager_add_rule(const char *app_name,
                                    const char *desktop_target,
                                    const char *state,
                                    char **error_message);
gboolean dc_window_manager_remove_rule(const char *app_name, char **error_message);
gboolean dc_window_manager_list_rules(char **rules_output, char **error_message);

#endif
