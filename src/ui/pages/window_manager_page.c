#include "ui/pages/window_manager_page.h"

struct _DcWindowManagerPage {
    GtkWidget *root;
};

static void add_css_class(GtkWidget *widget, const char *class_name) {
    gtk_style_context_add_class(gtk_widget_get_style_context(widget), class_name);
}

static GtkWidget *create_section_heading(const char *title, const char *subtitle) {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    GtkWidget *title_label = gtk_label_new(title);
    GtkWidget *subtitle_label = gtk_label_new(subtitle);

    gtk_widget_set_halign(title_label, GTK_ALIGN_START);
    gtk_widget_set_halign(subtitle_label, GTK_ALIGN_START);
    add_css_class(title_label, "page-title");
    add_css_class(subtitle_label, "page-subtitle");

    gtk_box_pack_start(GTK_BOX(box), title_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), subtitle_label, FALSE, FALSE, 0);
    return box;
}

static GtkWidget *create_setting_row(const char *title, const char *description, GtkWidget *control) {
    GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    GtkWidget *text_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    GtkWidget *title_label = gtk_label_new(title);
    GtkWidget *description_label = gtk_label_new(description);

    gtk_widget_set_halign(title_label, GTK_ALIGN_START);
    gtk_widget_set_halign(description_label, GTK_ALIGN_START);
    gtk_widget_set_hexpand(text_box, TRUE);
    gtk_label_set_line_wrap(GTK_LABEL(description_label), TRUE);
    add_css_class(title_label, "setting-title");
    add_css_class(description_label, "setting-description");

    gtk_box_pack_start(GTK_BOX(text_box), title_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(text_box), description_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(row), text_box, TRUE, TRUE, 0);
    gtk_box_pack_end(GTK_BOX(row), control, FALSE, FALSE, 0);
    return row;
}

static GtkWidget *create_card(const char *title, const char *subtitle) {
    GtkWidget *frame = gtk_frame_new(NULL);
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);

    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);
    gtk_container_set_border_width(GTK_CONTAINER(frame), 0);
    add_css_class(frame, "panel-card");
    gtk_container_add(GTK_CONTAINER(frame), box);
    gtk_box_pack_start(GTK_BOX(box), create_section_heading(title, subtitle), FALSE, FALSE, 0);
    return frame;
}

DcWindowManagerPage *dc_window_manager_page_new(void) {
    DcWindowManagerPage *page = g_new0(DcWindowManagerPage, 1);
    GtkWidget *content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 16);
    GtkWidget *scroller = gtk_scrolled_window_new(NULL, NULL);
    GtkWidget *card = create_card("Core Behavior", "These controls are placeholders for the next implementation pass.");
    GtkWidget *card_box = gtk_bin_get_child(GTK_BIN(card));
    GtkWidget *smart_focus = gtk_switch_new();
    GtkWidget *edge_snap = gtk_switch_new();
    GtkWidget *workspace_combo = gtk_combo_box_text_new();

    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(workspace_combo), "Static Workspaces");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(workspace_combo), "Dynamic Workspaces");
    gtk_combo_box_set_active(GTK_COMBO_BOX(workspace_combo), 0);

    add_css_class(content, "page-shell");
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroller), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_overlay_scrolling(GTK_SCROLLED_WINDOW(scroller), TRUE);
    gtk_container_add(GTK_CONTAINER(scroller), content);

    gtk_box_pack_start(GTK_BOX(content),
                       create_section_heading("Window Manager",
                                              "Prepare tiling, focus, borders, and workspace behavior from one place."),
                       FALSE,
                       FALSE,
                       0);
    gtk_box_pack_start(GTK_BOX(card_box),
                       create_setting_row("Smart Focus",
                                          "Focus the next relevant window automatically when the active window closes or moves.",
                                          smart_focus),
                       FALSE,
                       FALSE,
                       0);
    gtk_box_pack_start(GTK_BOX(card_box),
                       create_setting_row("Edge Snap",
                                          "Snap windows cleanly against screen edges and neighboring windows.",
                                          edge_snap),
                       FALSE,
                       FALSE,
                       0);
    gtk_box_pack_start(GTK_BOX(card_box),
                       create_setting_row("Workspace Model",
                                          "Choose how workspaces should be created and managed.",
                                          workspace_combo),
                       FALSE,
                       FALSE,
                       0);
    gtk_box_pack_start(GTK_BOX(content), card, FALSE, FALSE, 0);

    page->root = scroller;
    return page;
}

void dc_window_manager_page_free(DcWindowManagerPage *page) { g_free(page); }
GtkWidget *dc_window_manager_page_get_widget(DcWindowManagerPage *page) { return page->root; }
