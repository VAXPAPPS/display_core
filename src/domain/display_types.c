#include "domain/display_types.h"

#include <string.h>

typedef struct {
    const char *id;
    Rotation value;
} DcRotationItem;

static const DcRotationItem rotation_items[] = {
    { "normal", RR_Rotate_0 },
    { "left", RR_Rotate_90 },
    { "inverted", RR_Rotate_180 },
    { "right", RR_Rotate_270 },
};

DcDisplayMode *dc_display_mode_new(RRMode id, int width, int height, double refresh_rate, const char *label) {
    DcDisplayMode *mode = g_new0(DcDisplayMode, 1);

    mode->id = id;
    mode->width = width;
    mode->height = height;
    mode->refresh_rate = refresh_rate;
    mode->label = g_strdup(label);
    return mode;
}

void dc_display_mode_free(DcDisplayMode *mode) {
    if (mode == NULL) {
        return;
    }

    g_free(mode->label);
    g_free(mode);
}

DcDisplayOutput *dc_display_output_new(void) {
    DcDisplayOutput *output = g_new0(DcDisplayOutput, 1);

    output->modes = g_ptr_array_new_with_free_func((GDestroyNotify) dc_display_mode_free);
    return output;
}

void dc_display_output_free(DcDisplayOutput *output) {
    if (output == NULL) {
        return;
    }

    g_free(output->name);
    if (output->modes != NULL) {
        g_ptr_array_free(output->modes, TRUE);
    }
    g_free(output);
}

DcDisplayMode *dc_display_output_find_mode(const DcDisplayOutput *output, RRMode mode_id) {
    guint i;

    if (output == NULL || output->modes == NULL) {
        return NULL;
    }

    for (i = 0; i < output->modes->len; i++) {
        DcDisplayMode *mode = g_ptr_array_index(output->modes, i);
        if (mode->id == mode_id) {
            return mode;
        }
    }

    return NULL;
}

DcDisplayConfig *dc_display_config_new(void) {
    return g_new0(DcDisplayConfig, 1);
}

void dc_display_config_free(DcDisplayConfig *config) {
    if (config == NULL) {
        return;
    }

    g_free(config->output_name);
    g_free(config);
}

const char *dc_rotation_to_id(Rotation rotation) {
    guint i;

    for (i = 0; i < G_N_ELEMENTS(rotation_items); i++) {
        if (rotation_items[i].value == rotation) {
            return rotation_items[i].id;
        }
    }

    return "normal";
}

Rotation dc_rotation_from_id(const char *id) {
    guint i;

    if (id == NULL) {
        return RR_Rotate_0;
    }

    for (i = 0; i < G_N_ELEMENTS(rotation_items); i++) {
        if (g_strcmp0(rotation_items[i].id, id) == 0) {
            return rotation_items[i].value;
        }
    }

    return RR_Rotate_0;
}

gboolean dc_rotation_swaps_size(Rotation rotation) {
    return rotation == RR_Rotate_90 || rotation == RR_Rotate_270;
}
