#include "ui/pages/display_edit_page.h"

struct _DcDisplayEditPage {
    GtkWidget *root;
    GtkWidget *night_light_switch;
    GtkWidget *night_light_temperature_scale;
    GtkWidget *night_light_schedule_combo;
    GtkWidget *adaptive_brightness_switch;
    GtkWidget *gamma_scale;
    GtkWidget *vibrance_scale;
};

static const char *DISPLAY_EDIT_CSS =
    ".dedit-scroll-hidden scrollbar {"
    "  opacity: 0;"
    "  min-width: 0;"
    "  min-height: 0;"
    "}"
    ".dedit-scroll-hidden scrollbar slider {"
    "  min-width: 0;"
    "  min-height: 0;"
    "}"
    ".dedit-shell {"
    "  padding: 20px 18px 48px;"
    "}"
    ".dedit-card {"
    "  background-color: rgba(14,14,14,0.72);"
    "  border: 1px solid rgba(255,255,255,0.10);"
    "  border-radius: 18px;"
    "}"
    ".dedit-group-label {"
    "  color: rgba(255,255,255,0.28);"
    "  font-size: 10px;"
    "  font-weight: 700;"
    "  letter-spacing: 0.09em;"
    "  padding: 10px 18px 2px;"
    "}"
    ".dedit-row {"
    "  padding: 10px 18px;"
    "  border-bottom: 1px solid rgba(255,255,255,0.05);"
    "}"
    ".dedit-setting-title {"
    "  color: rgba(255,255,255,0.92);"
    "  font-size: 13px;"
    "  font-weight: 600;"
    "}"
    ".dedit-setting-desc {"
    "  color: rgba(255,255,255,0.45);"
    "  font-size: 11.5px;"
    "}"
    "switch {"
    "  background-color: rgba(255,255,255,0.12);"
    "  border: 1px solid rgba(255,255,255,0.08);"
    "  border-radius: 999px;"
    "  min-width: 42px;"
    "  min-height: 22px;"
    "}"
    "switch:checked {"
    "  background-color: rgba(255,255,255,0.32);"
    "  border-color: rgba(255,255,255,0.22);"
    "}"
    "switch slider {"
    "  min-width: 16px;"
    "  min-height: 16px;"
    "  border-radius: 50%;"
    "  background-color: rgba(255,255,255,0.95);"
    "  margin: 2px;"
    "}"
    "scale trough {"
    "  min-height: 4px;"
    "  border-radius: 2px;"
    "  background-color: rgba(255,255,255,0.13);"
    "}"
    "scale trough highlight {"
    "  background-color: rgba(255,255,255,0.50);"
    "  border-radius: 2px;"
    "}"
    "scale trough slider {"
    "  min-width: 16px;"
    "  min-height: 16px;"
    "  border-radius: 50%;"
    "  background-color: rgba(255,255,255,0.92);"
    "  border: none;"
    "}"
    "scale value {"
    "  color: rgba(255,255,255,0.38);"
    "  font-size: 11px;"
    "  min-width: 26px;"
    "}"
    "combobox button {"
    "  background-color: rgba(255,255,255,0.08);"
    "  background-image: none;"
    "  border: 1px solid rgba(255,255,255,0.12);"
    "  border-radius: 10px;"
    "  padding: 6px 10px;"
    "  color: rgba(255,255,255,0.90);"
    "  font-size: 12px;"
    "}"
    "combobox button:hover {"
    "  background-color: rgba(255,255,255,0.14);"
    "}";

static void on_realize(GtkWidget *widget, gpointer user_data) {
    GtkCssProvider *provider;

    (void) user_data;

    provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider, DISPLAY_EDIT_CSS, -1, NULL);
    gtk_style_context_add_provider_for_screen(gtk_widget_get_screen(widget),
                                              GTK_STYLE_PROVIDER(provider),
                                              GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);
}

static void add_css_class(GtkWidget *widget, const char *class_name) {
    gtk_style_context_add_class(gtk_widget_get_style_context(widget), class_name);
}

static GtkWidget *create_group_label(const char *text) {
    GtkWidget *label = gtk_label_new(text);

    gtk_widget_set_halign(label, GTK_ALIGN_START);
    add_css_class(label, "dedit-group-label");
    return label;
}

static GtkWidget *create_setting_row(const char *title,
                                     const char *description,
                                     GtkWidget *control) {
    GtkWidget *row;
    GtkWidget *text_box;
    GtkWidget *title_label;
    GtkWidget *description_label;

    row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    text_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 3);
    title_label = gtk_label_new(title);
    description_label = gtk_label_new(description);

    gtk_widget_set_halign(title_label, GTK_ALIGN_START);
    gtk_widget_set_halign(description_label, GTK_ALIGN_START);
    gtk_widget_set_hexpand(text_box, TRUE);
    gtk_label_set_line_wrap(GTK_LABEL(description_label), TRUE);
    gtk_widget_set_valign(control, GTK_ALIGN_CENTER);

    add_css_class(title_label, "dedit-setting-title");
    add_css_class(description_label, "dedit-setting-desc");
    add_css_class(row, "dedit-row");

    gtk_box_pack_start(GTK_BOX(text_box), title_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(text_box), description_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(row), text_box, TRUE, TRUE, 0);
    gtk_box_pack_end(GTK_BOX(row), control, FALSE, FALSE, 0);
    return row;
}

static GtkWidget *create_card(void) {
    GtkWidget *frame = gtk_frame_new(NULL);
    GtkWidget *outer_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);
    gtk_container_set_border_width(GTK_CONTAINER(frame), 0);
    add_css_class(frame, "dedit-card");
    gtk_container_add(GTK_CONTAINER(frame), outer_box);
    return frame;
}

DcDisplayEditPage *dc_display_edit_page_new(void) {
    DcDisplayEditPage *page;
    GtkWidget *content;
    GtkWidget *scroller;
    GtkWidget *night_card;
    GtkWidget *night_box;
    GtkWidget *display_card;
    GtkWidget *display_box;
    GtkWidget *scales[3];
    int i;

    page = g_new0(DcDisplayEditPage, 1);
    content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);
    scroller = gtk_scrolled_window_new(NULL, NULL);

    add_css_class(content, "dedit-shell");
    add_css_class(scroller, "dedit-scroll-hidden");

    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroller),
                                   GTK_POLICY_NEVER,
                                   GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_overlay_scrolling(GTK_SCROLLED_WINDOW(scroller), TRUE);
    gtk_container_add(GTK_CONTAINER(scroller), content);
    g_signal_connect(scroller, "realize", G_CALLBACK(on_realize), NULL);

    night_card = create_card();
    night_box = gtk_bin_get_child(GTK_BIN(night_card));
    page->night_light_switch = gtk_switch_new();
    page->night_light_temperature_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 2500.0, 6500.0, 100.0);
    page->night_light_schedule_combo = gtk_combo_box_text_new();

    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(page->night_light_schedule_combo), "sunset", "Sunset to Sunrise");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(page->night_light_schedule_combo), "custom", "Custom Schedule");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(page->night_light_schedule_combo), "always", "Always On");
    gtk_combo_box_set_active(GTK_COMBO_BOX(page->night_light_schedule_combo), 0);

    display_card = create_card();
    display_box = gtk_bin_get_child(GTK_BIN(display_card));
    page->adaptive_brightness_switch = gtk_switch_new();
    page->gamma_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.6, 1.6, 0.01);
    page->vibrance_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.0, 100.0, 1.0);

    scales[0] = page->night_light_temperature_scale;
    scales[1] = page->gamma_scale;
    scales[2] = page->vibrance_scale;
    for (i = 0; i < 3; i++) {
        gtk_widget_set_size_request(scales[i], 160, -1);
        gtk_widget_set_hexpand(scales[i], FALSE);
        gtk_scale_set_draw_value(GTK_SCALE(scales[i]), TRUE);
    }

    gtk_range_set_value(GTK_RANGE(page->night_light_temperature_scale), 4200.0);
    gtk_range_set_value(GTK_RANGE(page->gamma_scale), 1.00);
    gtk_range_set_value(GTK_RANGE(page->vibrance_scale), 18.0);

    gtk_box_pack_start(GTK_BOX(night_box), create_group_label("NIGHT LIGHT"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(night_box), create_setting_row("Enable Night Light",
                                                              "Shift the display toward warmer tones after dark.",
                                                              page->night_light_switch),
                       FALSE,
                       FALSE,
                       0);
    gtk_box_pack_start(GTK_BOX(night_box), create_setting_row("Color Temperature",
                                                              "Tune the warmth target used when night light is active.",
                                                              page->night_light_temperature_scale),
                       FALSE,
                       FALSE,
                       0);
    gtk_box_pack_start(GTK_BOX(night_box), create_setting_row("Schedule",
                                                              "Choose when the warm color profile should turn on automatically.",
                                                              page->night_light_schedule_combo),
                       FALSE,
                       FALSE,
                       0);

    gtk_box_pack_start(GTK_BOX(display_box), create_group_label("DISPLAY TUNING"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(display_box), create_setting_row("Adaptive Brightness",
                                                                "Prepare ambient brightness control hooks for later integration.",
                                                                page->adaptive_brightness_switch),
                       FALSE,
                       FALSE,
                       0);
    gtk_box_pack_start(GTK_BOX(display_box), create_setting_row("Gamma",
                                                                "Stage a base gamma profile for per-output calibration work.",
                                                                page->gamma_scale),
                       FALSE,
                       FALSE,
                       0);
    gtk_box_pack_start(GTK_BOX(display_box), create_setting_row("Vibrance",
                                                                "Prototype a saturation control for future display enhancement features.",
                                                                page->vibrance_scale),
                       FALSE,
                       FALSE,
                       0);

    gtk_box_pack_start(GTK_BOX(content), night_card, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(content), display_card, FALSE, FALSE, 0);

    page->root = scroller;
    return page;
}

void dc_display_edit_page_free(DcDisplayEditPage *page) {
    g_free(page);
}

GtkWidget *dc_display_edit_page_get_widget(DcDisplayEditPage *page) {
    return page->root;
}

GtkWidget *dc_display_edit_page_get_night_light_switch(DcDisplayEditPage *page) {
    return page->night_light_switch;
}

GtkWidget *dc_display_edit_page_get_night_light_temperature_scale(DcDisplayEditPage *page) {
    return page->night_light_temperature_scale;
}

GtkWidget *dc_display_edit_page_get_night_light_schedule_combo(DcDisplayEditPage *page) {
    return page->night_light_schedule_combo;
}

GtkWidget *dc_display_edit_page_get_adaptive_brightness_switch(DcDisplayEditPage *page) {
    return page->adaptive_brightness_switch;
}

GtkWidget *dc_display_edit_page_get_gamma_scale(DcDisplayEditPage *page) {
    return page->gamma_scale;
}

GtkWidget *dc_display_edit_page_get_vibrance_scale(DcDisplayEditPage *page) {
    return page->vibrance_scale;
}
