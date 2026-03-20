#include "ui/output_row.h"

struct _DcOutputRow {
    DcDisplayOutput *output;
    GtkWidget *frame;
    GtkWidget *enabled_check;
    GtkWidget *primary_check;
    GtkWidget *mode_combo;
    GtkWidget *rotation_combo;
    GtkWidget *x_spin;
    GtkWidget *y_spin;
    DcOutputRowChangedFn on_changed;
    DcOutputRowPrimaryFn on_primary_selected;
    gpointer user_data;
};

typedef struct {
    const char *id;
    const char *label;
    Rotation value;
} DcRotationOption;

static const DcRotationOption rotation_items[] = {
    { "normal", "Normal", RR_Rotate_0 },
    { "left", "Left", RR_Rotate_90 },
    { "inverted", "Inverted", RR_Rotate_180 },
    { "right", "Right", RR_Rotate_270 },
};

static GtkWidget *build_labeled_row(const char *label, GtkWidget *widget) {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    GtkWidget *title = gtk_label_new(label);

    gtk_widget_set_halign(title, GTK_ALIGN_START);
    gtk_widget_set_hexpand(title, TRUE);
    gtk_style_context_add_class(gtk_widget_get_style_context(title), "display-muted");
    gtk_box_pack_start(GTK_BOX(box), title, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(box), widget, FALSE, FALSE, 0);

    return box;
}

static void notify_changed(DcOutputRow *row) {
    if (row->on_changed != NULL) {
        row->on_changed(row->user_data);
    }
}

static void update_sensitive_state(DcOutputRow *row) {
    gboolean enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(row->enabled_check));

    gtk_widget_set_sensitive(row->mode_combo, enabled);
    gtk_widget_set_sensitive(row->rotation_combo, enabled);
    gtk_widget_set_sensitive(row->x_spin, enabled);
    gtk_widget_set_sensitive(row->y_spin, enabled);
    gtk_widget_set_sensitive(row->primary_check, enabled);

    if (!enabled) {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(row->primary_check), FALSE);
    }
}

static void on_enabled_toggled(GtkToggleButton *button, gpointer user_data) {
    DcOutputRow *row = user_data;

    (void) button;

    update_sensitive_state(row);
    notify_changed(row);
}

static void on_widget_changed(GtkWidget *widget, gpointer user_data) {
    DcOutputRow *row = user_data;

    (void) widget;
    notify_changed(row);
}

static void on_primary_toggled(GtkToggleButton *button, gpointer user_data) {
    DcOutputRow *row = user_data;

    if (gtk_toggle_button_get_active(button) && row->on_primary_selected != NULL) {
        row->on_primary_selected(row, row->user_data);
        return;
    }

    notify_changed(row);
}

static void on_row_frame_destroy(GtkWidget *widget, gpointer user_data) {
    DcOutputRow *row = user_data;

    (void) widget;

    row->frame = NULL;
    row->enabled_check = NULL;
    row->primary_check = NULL;
    row->mode_combo = NULL;
    row->rotation_combo = NULL;
    row->x_spin = NULL;
    row->y_spin = NULL;
}

DcOutputRow *dc_output_row_new(DcDisplayOutput *output,
                               DcOutputRowChangedFn on_changed,
                               DcOutputRowPrimaryFn on_primary_selected,
                               gpointer user_data) {
    DcOutputRow *row;
    GtkWidget *box;
    guint i;

    row = g_new0(DcOutputRow, 1);
    row->output = output;
    row->on_changed = on_changed;
    row->on_primary_selected = on_primary_selected;
    row->user_data = user_data;

    row->frame = gtk_frame_new(output->name);
    box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    row->enabled_check = gtk_check_button_new_with_label("Enabled");
    row->primary_check = gtk_check_button_new_with_label("Primary");
    row->mode_combo = gtk_combo_box_text_new();
    row->rotation_combo = gtk_combo_box_text_new();
    row->x_spin = gtk_spin_button_new_with_range(-8192, 8192, 1);
    row->y_spin = gtk_spin_button_new_with_range(-8192, 8192, 1);

    gtk_style_context_add_class(gtk_widget_get_style_context(row->frame), "display-card");

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(row->enabled_check), output->enabled);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(row->primary_check), output->primary);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(row->x_spin), output->x);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(row->y_spin), output->y);

    for (i = 0; i < output->modes->len; i++) {
        DcDisplayMode *mode = g_ptr_array_index(output->modes, i);
        char mode_id[32];

        g_snprintf(mode_id, sizeof(mode_id), "%lu", (unsigned long) mode->id);
        gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(row->mode_combo), mode_id, mode->label);

        if (mode->id == output->current_mode) {
            gtk_combo_box_set_active_id(GTK_COMBO_BOX(row->mode_combo), mode_id);
        }
    }

    if (gtk_combo_box_get_active(GTK_COMBO_BOX(row->mode_combo)) < 0 && output->modes->len > 0) {
        DcDisplayMode *mode = g_ptr_array_index(output->modes, 0);
        char mode_id[32];

        g_snprintf(mode_id, sizeof(mode_id), "%lu", (unsigned long) mode->id);
        gtk_combo_box_set_active_id(GTK_COMBO_BOX(row->mode_combo), mode_id);
    }

    for (i = 0; i < G_N_ELEMENTS(rotation_items); i++) {
        gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(row->rotation_combo),
                                  rotation_items[i].id,
                                  rotation_items[i].label);
    }

    gtk_combo_box_set_active_id(GTK_COMBO_BOX(row->rotation_combo),
                                dc_rotation_to_id(output->current_rotation));

    gtk_container_add(GTK_CONTAINER(row->frame), box);
    gtk_box_pack_start(GTK_BOX(box), row->enabled_check, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), build_labeled_row("Mode", row->mode_combo), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), build_labeled_row("Rotation", row->rotation_combo), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), build_labeled_row("Position X", row->x_spin), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), build_labeled_row("Position Y", row->y_spin), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), row->primary_check, FALSE, FALSE, 0);

    g_signal_connect(row->frame, "destroy", G_CALLBACK(on_row_frame_destroy), row);
    g_signal_connect(row->enabled_check, "toggled", G_CALLBACK(on_enabled_toggled), row);
    g_signal_connect(row->mode_combo, "changed", G_CALLBACK(on_widget_changed), row);
    g_signal_connect(row->rotation_combo, "changed", G_CALLBACK(on_widget_changed), row);
    g_signal_connect(row->x_spin, "value-changed", G_CALLBACK(on_widget_changed), row);
    g_signal_connect(row->y_spin, "value-changed", G_CALLBACK(on_widget_changed), row);
    g_signal_connect(row->primary_check, "toggled", G_CALLBACK(on_primary_toggled), row);

    update_sensitive_state(row);
    return row;
}

void dc_output_row_free(DcOutputRow *row) {
    g_free(row);
}

GtkWidget *dc_output_row_get_widget(DcOutputRow *row) {
    return row->frame;
}

DcDisplayOutput *dc_output_row_get_output(DcOutputRow *row) {
    return row->output;
}

gboolean dc_output_row_is_enabled(DcOutputRow *row) {
    return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(row->enabled_check));
}

gboolean dc_output_row_is_primary(DcOutputRow *row) {
    return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(row->primary_check));
}

RRMode dc_output_row_get_selected_mode(DcOutputRow *row) {
    const char *mode_id_text = gtk_combo_box_get_active_id(GTK_COMBO_BOX(row->mode_combo));
    unsigned long parsed_mode = 0;

    if (mode_id_text == NULL) {
        return None;
    }

    sscanf(mode_id_text, "%lu", &parsed_mode);
    return (RRMode) parsed_mode;
}

Rotation dc_output_row_get_selected_rotation(DcOutputRow *row) {
    return dc_rotation_from_id(gtk_combo_box_get_active_id(GTK_COMBO_BOX(row->rotation_combo)));
}

int dc_output_row_get_x(DcOutputRow *row) {
    return gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(row->x_spin));
}

int dc_output_row_get_y(DcOutputRow *row) {
    return gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(row->y_spin));
}

gboolean dc_output_row_get_geometry(DcOutputRow *row, int *x, int *y, int *width, int *height) {
    DcDisplayMode *mode;
    Rotation rotation;
    RRMode mode_id;

    if (!dc_output_row_is_enabled(row)) {
        return FALSE;
    }

    mode_id = dc_output_row_get_selected_mode(row);
    if (mode_id == None) {
        return FALSE;
    }

    mode = dc_display_output_find_mode(row->output, mode_id);
    if (mode == NULL) {
        return FALSE;
    }

    rotation = dc_output_row_get_selected_rotation(row);
    *width = mode->width;
    *height = mode->height;

    if (dc_rotation_swaps_size(rotation)) {
        int temp = *width;
        *width = *height;
        *height = temp;
    }

    *x = dc_output_row_get_x(row);
    *y = dc_output_row_get_y(row);
    return TRUE;
}

void dc_output_row_set_primary(DcOutputRow *row, gboolean primary) {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(row->primary_check), primary);
}

void dc_output_row_set_enabled(DcOutputRow *row, gboolean enabled) {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(row->enabled_check), enabled);
}

void dc_output_row_set_mode(DcOutputRow *row, RRMode mode_id) {
    char mode_text[32];

    g_snprintf(mode_text, sizeof(mode_text), "%lu", (unsigned long) mode_id);
    gtk_combo_box_set_active_id(GTK_COMBO_BOX(row->mode_combo), mode_text);
}

void dc_output_row_set_rotation(DcOutputRow *row, Rotation rotation) {
    gtk_combo_box_set_active_id(GTK_COMBO_BOX(row->rotation_combo), dc_rotation_to_id(rotation));
}

void dc_output_row_set_position(DcOutputRow *row, int x, int y) {
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(row->x_spin), x);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(row->y_spin), y);
}
