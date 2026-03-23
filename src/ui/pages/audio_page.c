#include "ui/pages/audio_page.h"

struct _DcAudioPage {
    GtkWidget *root;
    GtkWidget *output_combo;
    GtkWidget *output_volume_scale;
    GtkWidget *output_mute_switch;
    GtkWidget *input_combo;
    GtkWidget *input_volume_scale;
    GtkWidget *input_mute_switch;
    GtkWidget *overamplification_switch;
    GtkWidget *apps_box;
};

static const char *AUDIO_CSS =
    ".audio-scroll-hidden scrollbar {"
    "  opacity: 0;"
    "  min-width: 0;"
    "  min-height: 0;"
    "}"
    ".audio-scroll-hidden scrollbar slider {"
    "  min-width: 0;"
    "  min-height: 0;"
    "}"
    ".audio-shell {"
    "  padding: 20px 18px 48px;"
    "}"
    ".audio-card {"
    "  background-color: rgba(14,14,14,0.72);"
    "  border: 1px solid rgba(255,255,255,0.10);"
    "  border-radius: 18px;"
    "}"
    ".audio-group-label {"
    "  color: rgba(255,255,255,0.28);"
    "  font-size: 10px;"
    "  font-weight: 700;"
    "  letter-spacing: 0.09em;"
    "  padding: 10px 18px 2px;"
    "}"
    ".audio-row {"
    "  padding: 10px 18px;"
    "  border-bottom: 1px solid rgba(255,255,255,0.05);"
    "}"
    ".audio-setting-title {"
    "  color: rgba(255,255,255,0.92);"
    "  font-size: 13px;"
    "  font-weight: 600;"
    "}"
    ".audio-setting-desc {"
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
    gtk_css_provider_load_from_data(provider, AUDIO_CSS, -1, NULL);
    gtk_style_context_add_provider_for_screen(gtk_widget_get_screen(widget),
                                              GTK_STYLE_PROVIDER(provider),
                                              GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);
}

static void add_css_class(GtkWidget *widget, const char *class_name) {
    gtk_style_context_add_class(gtk_widget_get_style_context(widget), class_name);
}

static GtkWidget *create_group_label(const char *text) {
    GtkWidget *label;

    label = gtk_label_new(text);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    add_css_class(label, "audio-group-label");
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

    add_css_class(title_label, "audio-setting-title");
    add_css_class(description_label, "audio-setting-desc");
    add_css_class(row, "audio-row");

    gtk_box_pack_start(GTK_BOX(text_box), title_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(text_box), description_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(row), text_box, TRUE, TRUE, 0);
    gtk_box_pack_end(GTK_BOX(row), control, FALSE, FALSE, 0);
    return row;
}

static GtkWidget *create_card(void) {
    GtkWidget *frame;
    GtkWidget *outer_box;

    frame = gtk_frame_new(NULL);
    outer_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);
    gtk_container_set_border_width(GTK_CONTAINER(frame), 0);
    add_css_class(frame, "audio-card");
    gtk_container_add(GTK_CONTAINER(frame), outer_box);
    return frame;
}

DcAudioPage *dc_audio_page_new(void) {
    DcAudioPage *page;
    GtkWidget *content;
    GtkWidget *scroller;
    GtkWidget *output_card;
    GtkWidget *output_box;
    GtkWidget *input_card;
    GtkWidget *input_box;
    GtkWidget *behavior_card;
    GtkWidget *behavior_box;
    GtkWidget *apps_card;
    GtkWidget *apps_card_box;

    page = g_new0(DcAudioPage, 1);
    content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);
    scroller = gtk_scrolled_window_new(NULL, NULL);

    add_css_class(content, "audio-shell");
    add_css_class(scroller, "audio-scroll-hidden");
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroller), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_overlay_scrolling(GTK_SCROLLED_WINDOW(scroller), TRUE);
    gtk_container_add(GTK_CONTAINER(scroller), content);
    g_signal_connect(scroller, "realize", G_CALLBACK(on_realize), NULL);

    output_card = create_card();
    output_box = gtk_bin_get_child(GTK_BIN(output_card));
    page->output_combo = gtk_combo_box_text_new();
    page->output_volume_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.0, 150.0, 1.0);
    page->output_mute_switch = gtk_switch_new();

    input_card = create_card();
    input_box = gtk_bin_get_child(GTK_BIN(input_card));
    page->input_combo = gtk_combo_box_text_new();
    page->input_volume_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.0, 100.0, 1.0);
    page->input_mute_switch = gtk_switch_new();

    behavior_card = create_card();
    behavior_box = gtk_bin_get_child(GTK_BIN(behavior_card));
    page->overamplification_switch = gtk_switch_new();

    apps_card = create_card();
    apps_card_box = gtk_bin_get_child(GTK_BIN(apps_card));
    page->apps_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    gtk_widget_set_size_request(page->output_volume_scale, 160, -1);
    gtk_widget_set_size_request(page->input_volume_scale, 160, -1);
    gtk_scale_set_draw_value(GTK_SCALE(page->output_volume_scale), TRUE);
    gtk_scale_set_draw_value(GTK_SCALE(page->input_volume_scale), TRUE);

    gtk_box_pack_start(GTK_BOX(output_box), create_group_label("OUTPUT"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(output_box), create_setting_row("Output Device",
                                                               "Choose the active sink used for speakers or headphones.",
                                                               page->output_combo), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(output_box), create_setting_row("Output Volume",
                                                               "Adjust the current output volume and optional boost range.",
                                                               page->output_volume_scale), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(output_box), create_setting_row("Mute Output",
                                                               "Silence the current output without changing its saved level.",
                                                               page->output_mute_switch), FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(input_box), create_group_label("INPUT"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(input_box), create_setting_row("Input Device",
                                                              "Choose the active microphone or capture source.",
                                                              page->input_combo), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(input_box), create_setting_row("Input Volume",
                                                              "Control how strongly the active source is captured.",
                                                              page->input_volume_scale), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(input_box), create_setting_row("Mute Input",
                                                              "Disable microphone capture without losing the selected device.",
                                                              page->input_mute_switch), FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(behavior_box), create_group_label("BEHAVIOR"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(behavior_box), create_setting_row("Over-amplification",
                                                                 "Allow the output slider to boost beyond 100 percent when needed.",
                                                                 page->overamplification_switch), FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(apps_card_box), create_group_label("APPLICATIONS"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(apps_card_box), page->apps_box, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(content), output_card, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(content), input_card, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(content), behavior_card, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(content), apps_card, FALSE, FALSE, 0);

    page->root = scroller;
    return page;
}

void dc_audio_page_free(DcAudioPage *page) { g_free(page); }
GtkWidget *dc_audio_page_get_widget(DcAudioPage *page) { return page->root; }
GtkWidget *dc_audio_page_get_output_combo(DcAudioPage *page) { return page->output_combo; }
GtkWidget *dc_audio_page_get_output_volume_scale(DcAudioPage *page) { return page->output_volume_scale; }
GtkWidget *dc_audio_page_get_output_mute_switch(DcAudioPage *page) { return page->output_mute_switch; }
GtkWidget *dc_audio_page_get_input_combo(DcAudioPage *page) { return page->input_combo; }
GtkWidget *dc_audio_page_get_input_volume_scale(DcAudioPage *page) { return page->input_volume_scale; }
GtkWidget *dc_audio_page_get_input_mute_switch(DcAudioPage *page) { return page->input_mute_switch; }
GtkWidget *dc_audio_page_get_overamplification_switch(DcAudioPage *page) { return page->overamplification_switch; }
GtkWidget *dc_audio_page_get_apps_box(DcAudioPage *page) { return page->apps_box; }
