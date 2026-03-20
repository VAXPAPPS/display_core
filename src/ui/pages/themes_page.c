#include "ui/pages/themes_page.h"

struct _DcThemesPage {
    GtkWidget *root;
};

static const char *THEMES_PAGE_CSS =
    ".themes-scroll-hidden scrollbar {"
    "  opacity: 0;"
    "  min-width: 0;"
    "  min-height: 0;"
    "}"
    ".themes-scroll-hidden scrollbar slider {"
    "  min-width: 0;"
    "  min-height: 0;"
    "}"
    ".themes-shell {"
    "  padding: 20px 18px 48px;"
    "}"
    ".themes-card {"
    "  background-color: rgba(14,14,14,0.72);"
    "  border: 1px solid rgba(255,255,255,0.10);"
    "  border-radius: 18px;"
    "}"
    ".themes-group-label {"
    "  color: rgba(255,255,255,0.28);"
    "  font-size: 10px;"
    "  font-weight: 700;"
    "  letter-spacing: 0.09em;"
    "  padding: 10px 18px 2px;"
    "}"
    ".themes-row {"
    "  padding: 10px 18px;"
    "  border-bottom: 1px solid rgba(255,255,255,0.05);"
    "}"
    ".themes-setting-title {"
    "  color: rgba(255,255,255,0.92);"
    "  font-size: 13px;"
    "  font-weight: 600;"
    "}"
    ".themes-setting-desc {"
    "  color: rgba(255,255,255,0.45);"
    "  font-size: 11.5px;"
    "}"
    ".themes-note {"
    "  color: rgba(255,255,255,0.60);"
    "  font-size: 11px;"
    "  padding: 10px 18px 16px;"
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
    "}";

static void on_realize(GtkWidget *widget, gpointer user_data) {
    GtkCssProvider *provider;

    (void) user_data;

    provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider, THEMES_PAGE_CSS, -1, NULL);
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
    add_css_class(label, "themes-group-label");
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

    add_css_class(title_label, "themes-setting-title");
    add_css_class(description_label, "themes-setting-desc");
    add_css_class(row, "themes-row");

    gtk_box_pack_start(GTK_BOX(text_box), title_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(text_box), description_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(row), text_box, TRUE, TRUE, 0);
    gtk_box_pack_end(GTK_BOX(row), control, FALSE, FALSE, 0);
    return row;
}

static GtkWidget *create_card(void) {
    GtkWidget *frame;
    GtkWidget *box;

    frame = gtk_frame_new(NULL);
    box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);
    gtk_container_set_border_width(GTK_CONTAINER(frame), 0);
    add_css_class(frame, "themes-card");
    gtk_container_add(GTK_CONTAINER(frame), box);
    return frame;
}

static GtkWidget *create_combo(const char *first_id,
                               const char *first_label,
                               const char *second_id,
                               const char *second_label,
                               const char *third_id,
                               const char *third_label) {
    GtkWidget *combo;

    combo = gtk_combo_box_text_new();
    if (first_id != NULL) {
        gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(combo), first_id, first_label);
    }
    if (second_id != NULL) {
        gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(combo), second_id, second_label);
    }
    if (third_id != NULL) {
        gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(combo), third_id, third_label);
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0);
    return combo;
}

DcThemesPage *dc_themes_page_new(void) {
    DcThemesPage *page;
    GtkWidget *scroller;
    GtkWidget *content;
    GtkWidget *appearance_card;
    GtkWidget *appearance_box;
    GtkWidget *assets_card;
    GtkWidget *assets_box;
    GtkWidget *typography_card;
    GtkWidget *typography_box;
    GtkWidget *mode_combo;
    GtkWidget *theme_combo;
    GtkWidget *icons_combo;
    GtkWidget *cursor_combo;
    GtkWidget *font_combo;
    GtkWidget *mono_font_combo;
    GtkWidget *cursor_size_scale;
    GtkWidget *text_scale;

    page = g_new0(DcThemesPage, 1);
    scroller = gtk_scrolled_window_new(NULL, NULL);
    content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);

    add_css_class(scroller, "themes-scroll-hidden");
    add_css_class(content, "themes-shell");
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroller),
                                   GTK_POLICY_NEVER,
                                   GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_overlay_scrolling(GTK_SCROLLED_WINDOW(scroller), TRUE);
    gtk_container_add(GTK_CONTAINER(scroller), content);
    g_signal_connect(scroller, "realize", G_CALLBACK(on_realize), NULL);

    appearance_card = create_card();
    appearance_box = gtk_bin_get_child(GTK_BIN(appearance_card));
    mode_combo = create_combo("system", "Follow System",
                              "light", "Light Mode",
                              "dark", "Dark Mode");
    theme_combo = create_combo("adwaita", "Adwaita",
                               "vaxp-night", "VAXP Night",
                               "glass-midnight", "Glass Midnight");

    assets_card = create_card();
    assets_box = gtk_bin_get_child(GTK_BIN(assets_card));
    icons_combo = create_combo("adwaita", "Adwaita",
                               "papirus-dark", "Papirus Dark",
                               "vaxp-icons", "VAXP Icons");
    cursor_combo = create_combo("adwaita", "Adwaita",
                                "bibata-ice", "Bibata Ice",
                                "vaxp-cursor", "VAXP Cursor");
    cursor_size_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 16.0, 64.0, 1.0);
    gtk_widget_set_size_request(cursor_size_scale, 160, -1);
    gtk_scale_set_draw_value(GTK_SCALE(cursor_size_scale), TRUE);
    gtk_range_set_value(GTK_RANGE(cursor_size_scale), 24.0);

    typography_card = create_card();
    typography_box = gtk_bin_get_child(GTK_BIN(typography_card));
    font_combo = create_combo("sans-10", "Sans 10",
                              "cantarell-11", "Cantarell 11",
                              "ibm-plex-sans-10", "IBM Plex Sans 10");
    mono_font_combo = create_combo("monospace-10", "Monospace 10",
                                   "jetbrains-mono-10", "JetBrains Mono 10",
                                   "iosevka-11", "Iosevka 11");
    text_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.80, 1.60, 0.05);
    gtk_widget_set_size_request(text_scale, 160, -1);
    gtk_scale_set_draw_value(GTK_SCALE(text_scale), TRUE);
    gtk_range_set_value(GTK_RANGE(text_scale), 1.00);

    gtk_box_pack_start(GTK_BOX(appearance_box), create_group_label("APPEARANCE"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(appearance_box), create_setting_row("Interface Mode",
                                                                   "Switch between light, dark, or following the system preference.",
                                                                   mode_combo), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(appearance_box), create_setting_row("GTK Theme",
                                                                   "Choose the visual shell theme used by applications and panels.",
                                                                   theme_combo), FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(assets_box), create_group_label("ASSETS"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(assets_box), create_setting_row("Icon Theme",
                                                               "Select the icon pack exposed across the desktop.",
                                                               icons_combo), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(assets_box), create_setting_row("Cursor Theme",
                                                               "Choose the mouse pointer theme used by the session.",
                                                               cursor_combo), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(assets_box), create_setting_row("Cursor Size",
                                                               "Set the visual scale used by the pointer assets.",
                                                               cursor_size_scale), FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(typography_box), create_group_label("TYPOGRAPHY"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(typography_box), create_setting_row("Interface Font",
                                                                   "Pick the default UI font family and size for the desktop.",
                                                                   font_combo), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(typography_box), create_setting_row("Monospace Font",
                                                                   "Choose the monospace font used by terminals and code-oriented apps.",
                                                                   mono_font_combo), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(typography_box), create_setting_row("Text Scale",
                                                                   "Preview the global UI text scaling direction before wiring the logic.",
                                                                   text_scale), FALSE, FALSE, 0);

    {
        GtkWidget *note;

        note = gtk_label_new("Themes is currently UI-only. Theme application logic, persistence, and backend integration will be planned before implementation.");
        gtk_widget_set_halign(note, GTK_ALIGN_START);
        gtk_label_set_line_wrap(GTK_LABEL(note), TRUE);
        add_css_class(note, "themes-note");
        gtk_box_pack_start(GTK_BOX(typography_box), note, FALSE, FALSE, 0);
    }

    gtk_box_pack_start(GTK_BOX(content), appearance_card, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(content), assets_card, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(content), typography_card, FALSE, FALSE, 0);

    page->root = scroller;
    return page;
}

void dc_themes_page_free(DcThemesPage *page) {
    g_free(page);
}

GtkWidget *dc_themes_page_get_widget(DcThemesPage *page) {
    return page->root;
}
