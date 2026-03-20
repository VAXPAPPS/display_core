#include "ui/preview_canvas.h"

#include <math.h>

typedef struct {
    double scale;
    double offset_x;
    double offset_y;
    int min_x;
    int min_y;
    int width;
    int height;
} DcPreviewTransform;

struct _DcPreviewCanvas {
    GtkWidget *area;
    GPtrArray *rows;
    DcOutputRow *drag_row;
    double drag_offset_x;
    double drag_offset_y;
};

static gboolean compute_transform(DcPreviewCanvas *preview, DcPreviewTransform *transform) {
    GtkAllocation allocation;
    guint i;
    gboolean have_enabled = FALSE;
    int min_x = 0;
    int min_y = 0;
    int max_x = 0;
    int max_y = 0;

    gtk_widget_get_allocation(preview->area, &allocation);

    if (preview->rows == NULL) {
        transform->scale = 1.0;
        transform->offset_x = 0.0;
        transform->offset_y = 0.0;
        transform->min_x = 0;
        transform->min_y = 0;
        transform->width = allocation.width;
        transform->height = allocation.height;
        return FALSE;
    }

    for (i = 0; i < preview->rows->len; i++) {
        DcOutputRow *row = g_ptr_array_index(preview->rows, i);
        int x;
        int y;
        int width;
        int height;

        if (!dc_output_row_get_geometry(row, &x, &y, &width, &height)) {
            continue;
        }

        if (!have_enabled) {
            min_x = x;
            min_y = y;
            max_x = x + width;
            max_y = y + height;
            have_enabled = TRUE;
        } else {
            min_x = MIN(min_x, x);
            min_y = MIN(min_y, y);
            max_x = MAX(max_x, x + width);
            max_y = MAX(max_y, y + height);
        }
    }

    if (!have_enabled) {
        transform->scale = 1.0;
        transform->offset_x = 0.0;
        transform->offset_y = 0.0;
        transform->min_x = 0;
        transform->min_y = 0;
        transform->width = allocation.width;
        transform->height = allocation.height;
        return FALSE;
    }

    transform->min_x = min_x;
    transform->min_y = min_y;
    transform->width = MAX(max_x - min_x, 1);
    transform->height = MAX(max_y - min_y, 1);
    transform->scale = MIN((allocation.width - 40.0) / transform->width,
                           (allocation.height - 40.0) / transform->height);
    transform->scale = MAX(transform->scale, 0.05);
    transform->offset_x = (allocation.width - (transform->width * transform->scale)) / 2.0;
    transform->offset_y = (allocation.height - (transform->height * transform->scale)) / 2.0;
    return TRUE;
}

static void draw_preview_label(cairo_t *cr, double x, double y, const char *name, int width, int height) {
    char buffer[128];

    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 14.0);
    cairo_move_to(cr, x + 12.0, y + 24.0);
    cairo_show_text(cr, name);

    g_snprintf(buffer, sizeof(buffer), "%dx%d", width, height);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 12.0);
    cairo_move_to(cr, x + 12.0, y + 44.0);
    cairo_show_text(cr, buffer);
}

static gboolean on_draw(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    DcPreviewCanvas *preview = user_data;
    DcPreviewTransform transform;
    GtkAllocation allocation;
    guint i;

    (void) widget;

    gtk_widget_get_allocation(preview->area, &allocation);

    cairo_set_source_rgba(cr, 0.07, 0.07, 0.07, 0.92);
    cairo_paint(cr);

    cairo_pattern_t *grid = cairo_pattern_create_linear(0, 0, allocation.width, allocation.height);
    cairo_pattern_add_color_stop_rgba(grid, 0.0, 0.14, 0.14, 0.14, 0.90);
    cairo_pattern_add_color_stop_rgba(grid, 1.0, 0.07, 0.07, 0.07, 0.96);
    cairo_set_source(cr, grid);
    cairo_rectangle(cr, 0, 0, allocation.width, allocation.height);
    cairo_fill(cr);
    cairo_pattern_destroy(grid);

    if (!compute_transform(preview, &transform)) {
        cairo_set_source_rgb(cr, 0.85, 0.87, 0.92);
        cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(cr, 15.0);
        cairo_move_to(cr, 24.0, 36.0);
        cairo_show_text(cr, "Enable a display to preview and drag its layout.");
        return FALSE;
    }

    for (i = 0; i < preview->rows->len; i++) {
        DcOutputRow *row = g_ptr_array_index(preview->rows, i);
        int x;
        int y;
        int width;
        int height;
        double draw_x;
        double draw_y;
        double draw_w;
        double draw_h;
        gboolean is_primary;

        if (!dc_output_row_get_geometry(row, &x, &y, &width, &height)) {
            continue;
        }

        draw_x = transform.offset_x + ((double) (x - transform.min_x) * transform.scale);
        draw_y = transform.offset_y + ((double) (y - transform.min_y) * transform.scale);
        draw_w = width * transform.scale;
        draw_h = height * transform.scale;
        is_primary = dc_output_row_is_primary(row);

        if (row == preview->drag_row) {
            cairo_set_source_rgb(cr, 0.93, 0.62, 0.18);
        } else if (is_primary) {
            cairo_set_source_rgb(cr, 0.22, 0.66, 0.86);
        } else {
            cairo_set_source_rgba(cr, 0.88, 0.88, 0.88, 0.18);
        }

        cairo_rectangle(cr, draw_x, draw_y, draw_w, draw_h);
        cairo_fill_preserve(cr);

        cairo_set_source_rgb(cr, 0.97, 0.97, 0.99);
        cairo_set_line_width(cr, row == preview->drag_row ? 3.0 : 2.0);
        cairo_stroke(cr);

        draw_preview_label(cr, draw_x, draw_y, dc_output_row_get_output(row)->name, width, height);

        if (is_primary) {
            cairo_set_source_rgb(cr, 1.0, 0.92, 0.42);
            cairo_arc(cr, draw_x + draw_w - 14.0, draw_y + 14.0, 6.0, 0, 2 * G_PI);
            cairo_fill(cr);
        }
    }

    return FALSE;
}

static DcOutputRow *find_row_at_point(DcPreviewCanvas *preview,
                                      double px,
                                      double py,
                                      DcPreviewTransform *transform_out) {
    DcPreviewTransform transform;
    gint i;

    if (!compute_transform(preview, &transform)) {
        return NULL;
    }

    for (i = (gint) preview->rows->len - 1; i >= 0; i--) {
        DcOutputRow *row = g_ptr_array_index(preview->rows, i);
        int x;
        int y;
        int width;
        int height;
        double draw_x;
        double draw_y;
        double draw_w;
        double draw_h;

        if (!dc_output_row_get_geometry(row, &x, &y, &width, &height)) {
            continue;
        }

        draw_x = transform.offset_x + ((double) (x - transform.min_x) * transform.scale);
        draw_y = transform.offset_y + ((double) (y - transform.min_y) * transform.scale);
        draw_w = width * transform.scale;
        draw_h = height * transform.scale;

        if (px >= draw_x && px <= draw_x + draw_w && py >= draw_y && py <= draw_y + draw_h) {
            if (transform_out != NULL) {
                *transform_out = transform;
            }
            return row;
        }
    }

    return NULL;
}

static gboolean on_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    DcPreviewCanvas *preview = user_data;
    DcPreviewTransform transform;
    DcOutputRow *row;
    int x;
    int y;
    int width;
    int height;
    double draw_x;
    double draw_y;

    (void) widget;

    if (event->button != GDK_BUTTON_PRIMARY) {
        return FALSE;
    }

    row = find_row_at_point(preview, event->x, event->y, &transform);
    if (row == NULL || !dc_output_row_get_geometry(row, &x, &y, &width, &height)) {
        return FALSE;
    }

    draw_x = transform.offset_x + ((double) (x - transform.min_x) * transform.scale);
    draw_y = transform.offset_y + ((double) (y - transform.min_y) * transform.scale);

    preview->drag_row = row;
    preview->drag_offset_x = event->x - draw_x;
    preview->drag_offset_y = event->y - draw_y;
    dc_preview_canvas_queue_draw(preview);
    return TRUE;
}

static gboolean on_button_release(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    DcPreviewCanvas *preview = user_data;

    (void) widget;
    (void) event;

    preview->drag_row = NULL;
    dc_preview_canvas_queue_draw(preview);
    return FALSE;
}

static gboolean on_motion(GtkWidget *widget, GdkEventMotion *event, gpointer user_data) {
    DcPreviewCanvas *preview = user_data;
    DcPreviewTransform transform;
    int current_x;
    int current_y;
    int width;
    int height;
    int new_x;
    int new_y;

    (void) widget;

    if (preview->drag_row == NULL || !compute_transform(preview, &transform)) {
        return FALSE;
    }

    if (!dc_output_row_get_geometry(preview->drag_row, &current_x, &current_y, &width, &height)) {
        return FALSE;
    }

    new_x = transform.min_x +
            (int) lround((event->x - preview->drag_offset_x - transform.offset_x) / transform.scale);
    new_y = transform.min_y +
            (int) lround((event->y - preview->drag_offset_y - transform.offset_y) / transform.scale);

    dc_output_row_set_position(preview->drag_row, new_x, new_y);
    dc_preview_canvas_queue_draw(preview);
    return TRUE;
}

DcPreviewCanvas *dc_preview_canvas_new(void) {
    DcPreviewCanvas *preview = g_new0(DcPreviewCanvas, 1);

    preview->area = gtk_drawing_area_new();
    gtk_widget_set_size_request(preview->area, 720, 260);
    gtk_widget_add_events(preview->area,
                          GDK_BUTTON_PRESS_MASK |
                          GDK_BUTTON_RELEASE_MASK |
                          GDK_POINTER_MOTION_MASK);

    g_signal_connect(preview->area, "draw", G_CALLBACK(on_draw), preview);
    g_signal_connect(preview->area, "button-press-event", G_CALLBACK(on_button_press), preview);
    g_signal_connect(preview->area, "button-release-event", G_CALLBACK(on_button_release), preview);
    g_signal_connect(preview->area, "motion-notify-event", G_CALLBACK(on_motion), preview);
    return preview;
}

void dc_preview_canvas_free(DcPreviewCanvas *preview) {
    g_free(preview);
}

GtkWidget *dc_preview_canvas_get_widget(DcPreviewCanvas *preview) {
    return preview->area;
}

void dc_preview_canvas_set_rows(DcPreviewCanvas *preview, GPtrArray *rows) {
    preview->rows = rows;
    preview->drag_row = NULL;
    dc_preview_canvas_queue_draw(preview);
}

void dc_preview_canvas_queue_draw(DcPreviewCanvas *preview) {
    if (preview->area != NULL) {
        gtk_widget_queue_draw(preview->area);
    }
}
