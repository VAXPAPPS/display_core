#include "ui/pages/default_apps_page.h"
#include <string.h>

struct _DcDefaultAppsPage {
    GtkWidget *root;

    GtkWidget *combo_web;
    GtkWidget *combo_mail;
    GtkWidget *combo_file;
    GtkWidget *combo_text;
    GtkWidget *combo_video;
    GtkWidget *combo_audio;
    GtkWidget *combo_image;
};

/* ------------------------------------------------------------------ */
/*  CSS (Dark Glassmorphism)                                          */
/* ------------------------------------------------------------------ */
static const char *DEFAULT_APPS_CSS =
    ".dap-scroll-hidden scrollbar { opacity: 0; min-width: 0; min-height: 0; }"
    ".dap-shell { padding: 40px 18px 48px; }"
    ".dap-group-label { color: rgba(255,255,255,0.28); font-size: 10px; font-weight: 700; letter-spacing: 0.09em; padding: 10px 18px 2px; }"
    ".dap-row { padding: 12px 18px; border-bottom: 1px solid rgba(255,255,255,0.05); }"
    ".dap-title { color: rgba(255,255,255,0.92); font-size: 13px; font-weight: 600; }"
    ".dap-card { background-color: rgba(14,14,14,0.72); border: 1px solid rgba(255,255,255,0.10); border-radius: 18px; margin-bottom: 24px; box-shadow: 0 4px 12px rgba(0,0,0,0.1); }"
    
    // Customizing the combobox buttons for that clean look
    ".dap-combo { min-width: 250px; background-color: rgba(255,255,255,0.07); border: 1px solid rgba(255,255,255,0.1); border-radius: 8px; color: rgba(255,255,255,0.9); transition: all 0.2s; }"
    ".dap-combo:hover { background-color: rgba(255,255,255,0.12); }"
    ;

static void on_realize(GtkWidget *widget, gpointer user_data) {
    (void)user_data;
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider, DEFAULT_APPS_CSS, -1, NULL);
    gtk_style_context_add_provider_for_screen(
        gtk_widget_get_screen(widget),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
    );
    g_object_unref(provider);
}

static void add_css_class(GtkWidget *widget, const char *class_name) {
    gtk_style_context_add_class(gtk_widget_get_style_context(widget), class_name);
}

static GtkWidget *create_group_label(const char *text) {
    GtkWidget *label = gtk_label_new(text);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    add_css_class(label, "dap-group-label");
    return label;
}

static GtkWidget *create_combo_row(const char *title, GtkWidget **out_combo) {
    GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    GtkWidget *t_lbl = gtk_label_new(title);
    GtkWidget *combo = gtk_combo_box_text_new();

    gtk_widget_set_halign(t_lbl, GTK_ALIGN_START);
    gtk_widget_set_valign(t_lbl, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(combo, GTK_ALIGN_CENTER);
    gtk_widget_set_hexpand(t_lbl, TRUE);

    add_css_class(t_lbl, "dap-title");
    add_css_class(combo, "dap-combo");
    add_css_class(row, "dap-row");

    gtk_box_pack_start(GTK_BOX(row), t_lbl, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(row), combo, FALSE, FALSE, 0);

    *out_combo = combo;
    return row;
}

static GtkWidget *create_card(void) {
    GtkWidget *frame = gtk_frame_new(NULL);
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);
    gtk_container_set_border_width(GTK_CONTAINER(frame), 0);
    add_css_class(frame, "dap-card");

    gtk_container_add(GTK_CONTAINER(frame), vbox);
    return frame;
}

DcDefaultAppsPage *dc_default_apps_page_new(void) {
    DcDefaultAppsPage *page = g_new0(DcDefaultAppsPage, 1);

    GtkWidget *content  = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    GtkWidget *scroller = gtk_scrolled_window_new(NULL, NULL);
    add_css_class(content, "dap-shell");
    add_css_class(scroller, "dap-scroll-hidden");

    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroller),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_overlay_scrolling(GTK_SCROLLED_WINDOW(scroller), TRUE);
    gtk_container_add(GTK_CONTAINER(scroller), content);
    g_signal_connect(scroller, "realize", G_CALLBACK(on_realize), NULL);

    /* ================================================================ */
    /*  Internet                                                        */
    /* ================================================================ */
    GtkWidget *internet_card = create_card();
    GtkWidget *internet_box = gtk_bin_get_child(GTK_BIN(internet_card));

    gtk_box_pack_start(GTK_BOX(internet_box), create_group_label("INTERNET"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(internet_box), create_combo_row("Web Browser", &page->combo_web), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(internet_box), create_combo_row("Email Client", &page->combo_mail), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(content), internet_card, FALSE, FALSE, 0);

    /* ================================================================ */
    /*  System                                                          */
    /* ================================================================ */
    GtkWidget *sys_card = create_card();
    GtkWidget *sys_box = gtk_bin_get_child(GTK_BIN(sys_card));

    gtk_box_pack_start(GTK_BOX(sys_box), create_group_label("SYSTEM"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(sys_box), create_combo_row("File Manager", &page->combo_file), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(content), sys_card, FALSE, FALSE, 0);

    /* ================================================================ */
    /*  Media                                                           */
    /* ================================================================ */
    GtkWidget *media_card = create_card();
    GtkWidget *media_box = gtk_bin_get_child(GTK_BIN(media_card));

    gtk_box_pack_start(GTK_BOX(media_box), create_group_label("MEDIA"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(media_box), create_combo_row("Video Player", &page->combo_video), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(media_box), create_combo_row("Audio Player", &page->combo_audio), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(media_box), create_combo_row("Image Viewer", &page->combo_image), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(content), media_card, FALSE, FALSE, 0);

    /* ================================================================ */
    /*  Documents                                                       */
    /* ================================================================ */
    GtkWidget *doc_card = create_card();
    GtkWidget *doc_box = gtk_bin_get_child(GTK_BIN(doc_card));

    gtk_box_pack_start(GTK_BOX(doc_box), create_group_label("DOCUMENTS"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(doc_box), create_combo_row("Text Editor", &page->combo_text), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(content), doc_card, FALSE, FALSE, 0);


    page->root = scroller;
    return page;
}

void dc_default_apps_page_free(DcDefaultAppsPage *page) {
    if (!page) return;
    g_free(page);
}

GtkWidget *dc_default_apps_page_get_widget(DcDefaultAppsPage *page) {
    return page->root;
}

GtkWidget *dc_default_apps_page_get_combo(DcDefaultAppsPage *page, const char *category) {
    if (!page || !category) return NULL;
    
    if (strcmp(category, "web") == 0) return page->combo_web;
    if (strcmp(category, "mail") == 0) return page->combo_mail;
    if (strcmp(category, "file") == 0) return page->combo_file;
    if (strcmp(category, "text") == 0) return page->combo_text;
    if (strcmp(category, "video") == 0) return page->combo_video;
    if (strcmp(category, "audio") == 0) return page->combo_audio;
    if (strcmp(category, "image") == 0) return page->combo_image;
    
    return NULL;
}
