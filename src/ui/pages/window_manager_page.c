#include "ui/pages/window_manager_page.h"

struct _DcWindowManagerPage {
    GtkWidget *root;
    GtkWidget *floating_mode_switch;
    GtkWidget *snap_threshold_scale;
    GtkWidget *snap_show_preview_switch;
    GtkWidget *layout_combo;
    GtkWidget *border_width_scale;
    GtkWidget *focused_border_color_entry;
    GtkWidget *normal_border_color_entry;
    GtkWidget *window_gap_scale;
    GtkWidget *top_padding_scale;
    GtkWidget *bottom_padding_scale;
    GtkWidget *focus_opacity_switch;
    GtkWidget *inactive_opacity_scale;
    GtkWidget *active_opacity_scale;
    GtkWidget *add_desktop_entry;
    GtkWidget *add_desktop_button;
    GtkWidget *rename_desktop_entry;
    GtkWidget *rename_desktop_button;
    GtkWidget *remove_desktop_button;
    GtkWidget *rule_app_entry;
    GtkWidget *rule_desktop_entry;
    GtkWidget *rule_state_combo;
    GtkWidget *add_rule_button;
    GtkWidget *remove_rule_entry;
    GtkWidget *remove_rule_button;
    GtkWidget *refresh_rules_button;
    GtkWidget *rules_text_view;
};

static const char *WINDOW_MANAGER_CSS =
    ".wm-scroll-hidden scrollbar {"
    "  opacity: 0;"
    "  min-width: 0;"
    "  min-height: 0;"
    "}"
    ".wm-scroll-hidden scrollbar slider {"
    "  min-width: 0;"
    "  min-height: 0;"
    "}"
    ".wm-shell {"
    "  padding: 20px 18px 48px;"
    "}"
    ".wm-card {"
    "  background-color: rgba(14,14,14,0.72);"
    "  border: 1px solid rgba(255,255,255,0.10);"
    "  border-radius: 18px;"
    "}"
    ".wm-group-label {"
    "  color: rgba(255,255,255,0.28);"
    "  font-size: 10px;"
    "  font-weight: 700;"
    "  letter-spacing: 0.09em;"
    "  padding: 10px 18px 2px;"
    "}"
    ".wm-row {"
    "  padding: 10px 18px;"
    "  border-bottom: 1px solid rgba(255,255,255,0.05);"
    "}"
    ".wm-setting-title {"
    "  color: rgba(255,255,255,0.92);"
    "  font-size: 13px;"
    "  font-weight: 600;"
    "}"
    ".wm-setting-desc {"
    "  color: rgba(255,255,255,0.45);"
    "  font-size: 11.5px;"
    "}"
    ".wm-action-button {"
    "  background-color: rgba(255,255,255,0.12);"
    "  background-image: none;"
    "  border: 1px solid rgba(255,255,255,0.14);"
    "  border-radius: 10px;"
    "  color: rgba(255,255,255,0.94);"
    "  padding: 8px 12px;"
    "  font-size: 12px;"
    "  font-weight: 600;"
    "}"
    ".wm-action-button:hover {"
    "  background-color: rgba(255,255,255,0.18);"
    "}"
    ".wm-action-button:disabled {"
    "  color: rgba(255,255,255,0.35);"
    "}"
    ".wm-rules-view {"
    "  min-height: 140px;"
    "  background-color: rgba(255,255,255,0.04);"
    "  border: 1px solid rgba(255,255,255,0.08);"
    "  border-radius: 12px;"
    "  padding: 10px;"
    "  color: rgba(255,255,255,0.88);"
    "  font-family: monospace;"
    "  font-size: 11px;"
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
    "}"
    "entry, textview {"
    "  background-color: rgba(255,255,255,0.08);"
    "  background-image: none;"
    "  border: 1px solid rgba(255,255,255,0.12);"
    "  border-radius: 10px;"
    "  padding: 7px 12px;"
    "  color: rgba(255,255,255,0.92);"
    "  font-size: 12px;"
    "}"
    "entry:focus, textview:focus {"
    "  border-color: rgba(255,255,255,0.30);"
    "  background-color: rgba(255,255,255,0.11);"
    "}";

static void on_realize(GtkWidget *widget, gpointer user_data) {
    GtkCssProvider *provider;

    (void) user_data;

    provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider, WINDOW_MANAGER_CSS, -1, NULL);
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
    add_css_class(label, "wm-group-label");
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

    add_css_class(title_label, "wm-setting-title");
    add_css_class(description_label, "wm-setting-desc");
    add_css_class(row, "wm-row");

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
    add_css_class(frame, "wm-card");
    gtk_container_add(GTK_CONTAINER(frame), outer_box);
    return frame;
}

static GtkWidget *create_action_button(const char *label) {
    GtkWidget *button;

    button = gtk_button_new_with_label(label);
    add_css_class(button, "wm-action-button");
    return button;
}

static GtkWidget *create_inline_control(GtkWidget *first,
                                        GtkWidget *second,
                                        GtkWidget *third) {
    GtkWidget *box;

    box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    add_css_class(box, "wm-inline-control");
    if (first != NULL) {
        gtk_box_pack_start(GTK_BOX(box), first, FALSE, FALSE, 0);
    }
    if (second != NULL) {
        gtk_box_pack_start(GTK_BOX(box), second, FALSE, FALSE, 0);
    }
    if (third != NULL) {
        gtk_box_pack_start(GTK_BOX(box), third, FALSE, FALSE, 0);
    }
    return box;
}

DcWindowManagerPage *dc_window_manager_page_new(void) {
    DcWindowManagerPage *page;
    GtkWidget *content;
    GtkWidget *scroller;
    GtkWidget *behavior_card;
    GtkWidget *behavior_box;
    GtkWidget *style_card;
    GtkWidget *style_box;
    GtkWidget *rules_card;
    GtkWidget *rules_box;
    GtkWidget *scales[7];
    GtkWidget *rules_scroller;
    int i;

    page = g_new0(DcWindowManagerPage, 1);
    content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);
    scroller = gtk_scrolled_window_new(NULL, NULL);

    add_css_class(content, "wm-shell");
    add_css_class(scroller, "wm-scroll-hidden");
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroller),
                                   GTK_POLICY_NEVER,
                                   GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_overlay_scrolling(GTK_SCROLLED_WINDOW(scroller), TRUE);
    gtk_container_add(GTK_CONTAINER(scroller), content);
    g_signal_connect(scroller, "realize", G_CALLBACK(on_realize), NULL);

    behavior_card = create_card();
    behavior_box = gtk_bin_get_child(GTK_BIN(behavior_card));
    page->floating_mode_switch = gtk_switch_new();
    page->snap_threshold_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.0, 80.0, 1.0);
    page->snap_show_preview_switch = gtk_switch_new();
    page->layout_combo = gtk_combo_box_text_new();
    page->window_gap_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.0, 64.0, 1.0);
    page->top_padding_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.0, 128.0, 1.0);
    page->bottom_padding_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.0, 128.0, 1.0);

    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(page->layout_combo), "tiled", "tiled");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(page->layout_combo), "monocle", "monocle");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(page->layout_combo), "tall", "tall");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(page->layout_combo), "rtall", "rtall");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(page->layout_combo), "wide", "wide");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(page->layout_combo), "rwide", "rwide");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(page->layout_combo), "grid", "grid");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(page->layout_combo), "rgrid", "rgrid");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(page->layout_combo), "even", "even");
    gtk_combo_box_set_active_id(GTK_COMBO_BOX(page->layout_combo), "tiled");

    style_card = create_card();
    style_box = gtk_bin_get_child(GTK_BIN(style_card));
    page->border_width_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.0, 12.0, 1.0);
    page->focused_border_color_entry = gtk_entry_new();
    page->normal_border_color_entry = gtk_entry_new();
    page->focus_opacity_switch = gtk_switch_new();
    page->inactive_opacity_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.10, 1.00, 0.01);
    page->active_opacity_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.10, 1.00, 0.01);

    gtk_entry_set_placeholder_text(GTK_ENTRY(page->focused_border_color_entry), "#5e81ac");
    gtk_entry_set_placeholder_text(GTK_ENTRY(page->normal_border_color_entry), "#3b4252");

    rules_card = create_card();
    rules_box = gtk_bin_get_child(GTK_BIN(rules_card));
    page->add_desktop_entry = gtk_entry_new();
    page->add_desktop_button = create_action_button("Create");
    page->rename_desktop_entry = gtk_entry_new();
    page->rename_desktop_button = create_action_button("Rename");
    page->remove_desktop_button = create_action_button("Remove Current");
    page->rule_app_entry = gtk_entry_new();
    page->rule_desktop_entry = gtk_entry_new();
    page->rule_state_combo = gtk_combo_box_text_new();
    page->add_rule_button = create_action_button("Add Rule");
    page->remove_rule_entry = gtk_entry_new();
    page->remove_rule_button = create_action_button("Remove Rule");
    page->refresh_rules_button = create_action_button("Refresh Rules");
    page->rules_text_view = gtk_text_view_new();

    gtk_entry_set_placeholder_text(GTK_ENTRY(page->add_desktop_entry), "Work");
    gtk_entry_set_placeholder_text(GTK_ENTRY(page->rename_desktop_entry), "Workspace");
    gtk_entry_set_placeholder_text(GTK_ENTRY(page->rule_app_entry), "Firefox");
    gtk_entry_set_placeholder_text(GTK_ENTRY(page->rule_desktop_entry), "^2");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(page->rule_state_combo), "floating", "floating");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(page->rule_state_combo), "tiled", "tiled");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(page->rule_state_combo), "fullscreen", "fullscreen");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(page->rule_state_combo), "maximized", "maximized");
    gtk_combo_box_set_active_id(GTK_COMBO_BOX(page->rule_state_combo), "floating");
    gtk_entry_set_placeholder_text(GTK_ENTRY(page->remove_rule_entry), "Firefox");

    gtk_text_view_set_editable(GTK_TEXT_VIEW(page->rules_text_view), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(page->rules_text_view), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(page->rules_text_view), GTK_WRAP_WORD_CHAR);
    add_css_class(page->rules_text_view, "wm-rules-view");

    scales[0] = page->snap_threshold_scale;
    scales[1] = page->window_gap_scale;
    scales[2] = page->top_padding_scale;
    scales[3] = page->bottom_padding_scale;
    scales[4] = page->border_width_scale;
    scales[5] = page->inactive_opacity_scale;
    scales[6] = page->active_opacity_scale;
    for (i = 0; i < 7; i++) {
        gtk_widget_set_size_request(scales[i], 160, -1);
        gtk_widget_set_hexpand(scales[i], FALSE);
        gtk_scale_set_draw_value(GTK_SCALE(scales[i]), TRUE);
    }

    gtk_widget_set_size_request(page->add_desktop_entry, 160, -1);
    gtk_widget_set_size_request(page->rename_desktop_entry, 160, -1);
    gtk_widget_set_size_request(page->rule_app_entry, 120, -1);
    gtk_widget_set_size_request(page->rule_desktop_entry, 80, -1);
    gtk_widget_set_size_request(page->remove_rule_entry, 120, -1);

    gtk_range_set_value(GTK_RANGE(page->snap_threshold_scale), 20.0);
    gtk_range_set_value(GTK_RANGE(page->window_gap_scale), 10.0);
    gtk_range_set_value(GTK_RANGE(page->top_padding_scale), 30.0);
    gtk_range_set_value(GTK_RANGE(page->bottom_padding_scale), 0.0);
    gtk_range_set_value(GTK_RANGE(page->border_width_scale), 2.0);
    gtk_range_set_value(GTK_RANGE(page->inactive_opacity_scale), 0.85);
    gtk_range_set_value(GTK_RANGE(page->active_opacity_scale), 1.0);
    gtk_entry_set_text(GTK_ENTRY(page->focused_border_color_entry), "#5e81ac");
    gtk_entry_set_text(GTK_ENTRY(page->normal_border_color_entry), "#3b4252");

    gtk_box_pack_start(GTK_BOX(behavior_box), create_group_label("BEHAVIOR"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(behavior_box), create_setting_row("Floating Mode",
                                                                 "Enable PoisonBlade floating mode for free window movement.",
                                                                 page->floating_mode_switch), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(behavior_box), create_setting_row("Snap Threshold",
                                                                 "Control edge snap sensitivity in pixels.",
                                                                 page->snap_threshold_scale), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(behavior_box), create_setting_row("Snap Preview",
                                                                 "Show preview feedback while snapping floating windows.",
                                                                 page->snap_show_preview_switch), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(behavior_box), create_setting_row("Desktop Layout",
                                                                 "Set the current PoisonBlade desktop layout.",
                                                                 page->layout_combo), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(behavior_box), create_setting_row("Window Gap",
                                                                 "Set the gap between managed windows.",
                                                                 page->window_gap_scale), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(behavior_box), create_setting_row("Top Padding",
                                                                 "Reserve space for the top panel or shell chrome.",
                                                                 page->top_padding_scale), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(behavior_box), create_setting_row("Bottom Padding",
                                                                 "Reserve space at the bottom of the work area.",
                                                                 page->bottom_padding_scale), FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(style_box), create_group_label("BORDERS & OPACITY"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(style_box), create_setting_row("Border Width",
                                                              "Set the border width used by managed windows.",
                                                              page->border_width_scale), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(style_box), create_setting_row("Focused Border Color",
                                                              "Hex color applied to the focused window border.",
                                                              page->focused_border_color_entry), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(style_box), create_setting_row("Normal Border Color",
                                                              "Hex color applied to unfocused window borders.",
                                                              page->normal_border_color_entry), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(style_box), create_setting_row("Focus Opacity Mode",
                                                              "Use active and inactive opacity values for focus transitions.",
                                                              page->focus_opacity_switch), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(style_box), create_setting_row("Inactive Opacity",
                                                              "Opacity used for unfocused windows.",
                                                              page->inactive_opacity_scale), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(style_box), create_setting_row("Active Opacity",
                                                              "Opacity used for the focused window.",
                                                              page->active_opacity_scale), FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(rules_box), create_group_label("DESKTOPS & RULES"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(rules_box), create_setting_row("Create Desktop",
                                                              "Add a new desktop on the current monitor.",
                                                              create_inline_control(page->add_desktop_entry,
                                                                                    page->add_desktop_button,
                                                                                    NULL)), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(rules_box), create_setting_row("Rename Desktop",
                                                              "Rename the current desktop without leaving the page.",
                                                              create_inline_control(page->rename_desktop_entry,
                                                                                    page->rename_desktop_button,
                                                                                    NULL)), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(rules_box), create_setting_row("Remove Current Desktop",
                                                              "Remove the currently focused desktop.",
                                                              create_inline_control(page->remove_desktop_button,
                                                                                    NULL,
                                                                                    NULL)), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(rules_box), create_setting_row("Add Rule",
                                                              "Create a PoisonBlade rule for an app, desktop, and preferred state.",
                                                              create_inline_control(page->rule_app_entry,
                                                                                    page->rule_desktop_entry,
                                                                                    page->rule_state_combo)), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(rules_box), create_setting_row("Commit Rule",
                                                              "Write the rule to PoisonBlade immediately.",
                                                              create_inline_control(page->add_rule_button,
                                                                                    NULL,
                                                                                    NULL)), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(rules_box), create_setting_row("Remove Rule",
                                                              "Delete an existing rule by application name.",
                                                              create_inline_control(page->remove_rule_entry,
                                                                                    page->remove_rule_button,
                                                                                    NULL)), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(rules_box), create_setting_row("Rule Inventory",
                                                              "Refresh and inspect the current PoisonBlade rules.",
                                                              create_inline_control(page->refresh_rules_button,
                                                                                    NULL,
                                                                                    NULL)), FALSE, FALSE, 0);

    rules_scroller = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(rules_scroller),
                                   GTK_POLICY_NEVER,
                                   GTK_POLICY_AUTOMATIC);
    gtk_widget_set_margin_start(rules_scroller, 18);
    gtk_widget_set_margin_end(rules_scroller, 18);
    gtk_widget_set_margin_top(rules_scroller, 4);
    gtk_widget_set_margin_bottom(rules_scroller, 18);
    gtk_container_add(GTK_CONTAINER(rules_scroller), page->rules_text_view);
    gtk_box_pack_start(GTK_BOX(rules_box), rules_scroller, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(content), behavior_card, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(content), style_card, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(content), rules_card, FALSE, FALSE, 0);

    page->root = scroller;
    return page;
}

void dc_window_manager_page_free(DcWindowManagerPage *page) {
    g_free(page);
}

GtkWidget *dc_window_manager_page_get_widget(DcWindowManagerPage *page) { return page->root; }
GtkWidget *dc_window_manager_page_get_floating_mode_switch(DcWindowManagerPage *page) { return page->floating_mode_switch; }
GtkWidget *dc_window_manager_page_get_snap_threshold_scale(DcWindowManagerPage *page) { return page->snap_threshold_scale; }
GtkWidget *dc_window_manager_page_get_snap_show_preview_switch(DcWindowManagerPage *page) { return page->snap_show_preview_switch; }
GtkWidget *dc_window_manager_page_get_layout_combo(DcWindowManagerPage *page) { return page->layout_combo; }
GtkWidget *dc_window_manager_page_get_border_width_scale(DcWindowManagerPage *page) { return page->border_width_scale; }
GtkWidget *dc_window_manager_page_get_focused_border_color_entry(DcWindowManagerPage *page) { return page->focused_border_color_entry; }
GtkWidget *dc_window_manager_page_get_normal_border_color_entry(DcWindowManagerPage *page) { return page->normal_border_color_entry; }
GtkWidget *dc_window_manager_page_get_window_gap_scale(DcWindowManagerPage *page) { return page->window_gap_scale; }
GtkWidget *dc_window_manager_page_get_top_padding_scale(DcWindowManagerPage *page) { return page->top_padding_scale; }
GtkWidget *dc_window_manager_page_get_bottom_padding_scale(DcWindowManagerPage *page) { return page->bottom_padding_scale; }
GtkWidget *dc_window_manager_page_get_focus_opacity_switch(DcWindowManagerPage *page) { return page->focus_opacity_switch; }
GtkWidget *dc_window_manager_page_get_inactive_opacity_scale(DcWindowManagerPage *page) { return page->inactive_opacity_scale; }
GtkWidget *dc_window_manager_page_get_active_opacity_scale(DcWindowManagerPage *page) { return page->active_opacity_scale; }
GtkWidget *dc_window_manager_page_get_add_desktop_entry(DcWindowManagerPage *page) { return page->add_desktop_entry; }
GtkWidget *dc_window_manager_page_get_add_desktop_button(DcWindowManagerPage *page) { return page->add_desktop_button; }
GtkWidget *dc_window_manager_page_get_rename_desktop_entry(DcWindowManagerPage *page) { return page->rename_desktop_entry; }
GtkWidget *dc_window_manager_page_get_rename_desktop_button(DcWindowManagerPage *page) { return page->rename_desktop_button; }
GtkWidget *dc_window_manager_page_get_remove_desktop_button(DcWindowManagerPage *page) { return page->remove_desktop_button; }
GtkWidget *dc_window_manager_page_get_rule_app_entry(DcWindowManagerPage *page) { return page->rule_app_entry; }
GtkWidget *dc_window_manager_page_get_rule_desktop_entry(DcWindowManagerPage *page) { return page->rule_desktop_entry; }
GtkWidget *dc_window_manager_page_get_rule_state_combo(DcWindowManagerPage *page) { return page->rule_state_combo; }
GtkWidget *dc_window_manager_page_get_add_rule_button(DcWindowManagerPage *page) { return page->add_rule_button; }
GtkWidget *dc_window_manager_page_get_remove_rule_entry(DcWindowManagerPage *page) { return page->remove_rule_entry; }
GtkWidget *dc_window_manager_page_get_remove_rule_button(DcWindowManagerPage *page) { return page->remove_rule_button; }
GtkWidget *dc_window_manager_page_get_refresh_rules_button(DcWindowManagerPage *page) { return page->refresh_rules_button; }
GtkWidget *dc_window_manager_page_get_rules_text_view(DcWindowManagerPage *page) { return page->rules_text_view; }
