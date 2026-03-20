#ifndef DC_DISPLAY_TYPES_H
#define DC_DISPLAY_TYPES_H

#include <gtk/gtk.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>

typedef struct {
    RRMode id;
    int width;
    int height;
    double refresh_rate;
    char *label;
} DcDisplayMode;

typedef struct {
    RROutput output_id;
    char *name;
    gboolean connected;
    gboolean enabled;
    gboolean primary;
    RRCrtc crtc_id;
    RRMode current_mode;
    Rotation current_rotation;
    int x;
    int y;
    GPtrArray *modes;
} DcDisplayOutput;

typedef struct {
    RROutput output_id;
    char *output_name;
    gboolean enabled;
    gboolean primary;
    RRMode mode;
    Rotation rotation;
    int x;
    int y;
} DcDisplayConfig;

DcDisplayMode *dc_display_mode_new(RRMode id, int width, int height, double refresh_rate, const char *label);
void dc_display_mode_free(DcDisplayMode *mode);

DcDisplayOutput *dc_display_output_new(void);
void dc_display_output_free(DcDisplayOutput *output);
DcDisplayMode *dc_display_output_find_mode(const DcDisplayOutput *output, RRMode mode_id);

DcDisplayConfig *dc_display_config_new(void);
void dc_display_config_free(DcDisplayConfig *config);

const char *dc_rotation_to_id(Rotation rotation);
Rotation dc_rotation_from_id(const char *id);
gboolean dc_rotation_swaps_size(Rotation rotation);

#endif
