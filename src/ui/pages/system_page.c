#include "ui/pages/system_page.h"

struct _DcSystemPage {
    GtkWidget *root;

    GtkWidget *combo_locale;
    GtkWidget *combo_timezone;
    GtkWidget *switch_ntp;
};

/* ------------------------------------------------------------------ */
/*  CSS (Dark Glassmorphism)                                          */
/* ------------------------------------------------------------------ */
static const char *SYSTEM_CSS =
    ".sys-scroll-hidden scrollbar { opacity: 0; min-width: 0; min-height: 0; }"
    ".sys-shell { padding: 40px 18px 48px; }"
    ".sys-group-label { color: rgba(255,255,255,0.28); font-size: 10px; font-weight: 700; letter-spacing: 0.09em; padding: 10px 18px 2px; }"
    ".sys-row { padding: 12px 18px; border-bottom: 1px solid rgba(255,255,255,0.05); }"
    ".sys-title { color: rgba(255,255,255,0.92); font-size: 13px; font-weight: 600; }"
    ".sys-desc { color: rgba(255,255,255,0.60); font-size: 11px; margin-top: 4px; }"
    ".sys-card { background-color: rgba(14,14,14,0.72); border: 1px solid rgba(255,255,255,0.10); border-radius: 18px; margin-bottom: 24px; box-shadow: 0 4px 12px rgba(0,0,0,0.1); }"
    
    // Comboboxes
    ".sys-combo { min-width: 250px; background-color: rgba(255,255,255,0.07); border: 1px solid rgba(255,255,255,0.1); border-radius: 8px; color: rgba(255,255,255,0.9); transition: all 0.2s; }"
    ".sys-combo:hover { background-color: rgba(255,255,255,0.12); }"
    
    // Switch
    "switch.sys-switch { border-radius: 14px; outline: none; box-shadow: inset 0 0 2px rgba(0,0,0,0.2); }"
    "switch.sys-switch:checked { background-color: #2196F3; }"
    ;

static void on_realize(GtkWidget *widget, gpointer user_data) {
    (void)user_data;
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider, SYSTEM_CSS, -1, NULL);
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
    add_css_class(label, "sys-group-label");
    return label;
}

static GtkWidget *create_combo_row(const char *title, const char *desc, GtkWidget **out_combo) {
    GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    
    GtkWidget *vbox_text = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    GtkWidget *t_lbl = gtk_label_new(title);
    gtk_widget_set_halign(t_lbl, GTK_ALIGN_START);
    add_css_class(t_lbl, "sys-title");
    gtk_box_pack_start(GTK_BOX(vbox_text), t_lbl, FALSE, FALSE, 0);

    if (desc) {
        GtkWidget *d_lbl = gtk_label_new(desc);
        gtk_widget_set_halign(d_lbl, GTK_ALIGN_START);
        add_css_class(d_lbl, "sys-desc");
        gtk_box_pack_start(GTK_BOX(vbox_text), d_lbl, FALSE, FALSE, 0);
    }
    
    GtkWidget *combo = gtk_combo_box_text_new();
    gtk_widget_set_valign(vbox_text, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(combo, GTK_ALIGN_CENTER);
    gtk_widget_set_hexpand(vbox_text, TRUE);

    add_css_class(combo, "sys-combo");
    add_css_class(row, "sys-row");

    gtk_box_pack_start(GTK_BOX(row), vbox_text, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(row), combo, FALSE, FALSE, 0);

    *out_combo = combo;
    return row;
}

static GtkWidget *create_switch_row(const char *title, const char *desc, GtkWidget **out_switch) {
    GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    
    GtkWidget *vbox_text = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    GtkWidget *t_lbl = gtk_label_new(title);
    gtk_widget_set_halign(t_lbl, GTK_ALIGN_START);
    add_css_class(t_lbl, "sys-title");
    gtk_box_pack_start(GTK_BOX(vbox_text), t_lbl, FALSE, FALSE, 0);

    if (desc) {
        GtkWidget *d_lbl = gtk_label_new(desc);
        gtk_widget_set_halign(d_lbl, GTK_ALIGN_START);
        add_css_class(d_lbl, "sys-desc");
        gtk_box_pack_start(GTK_BOX(vbox_text), d_lbl, FALSE, FALSE, 0);
    }

    GtkWidget *sw = gtk_switch_new();
    gtk_widget_set_valign(vbox_text, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(sw, GTK_ALIGN_CENTER);
    gtk_widget_set_hexpand(vbox_text, TRUE);

    add_css_class(sw, "sys-switch");
    add_css_class(row, "sys-row");

    gtk_box_pack_start(GTK_BOX(row), vbox_text, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(row), sw, FALSE, FALSE, 0);

    *out_switch = sw;
    return row;
}

static GtkWidget *create_card(void) {
    GtkWidget *frame = gtk_frame_new(NULL);
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);
    gtk_container_set_border_width(GTK_CONTAINER(frame), 0);
    add_css_class(frame, "sys-card");

    gtk_container_add(GTK_CONTAINER(frame), vbox);
    return frame;
}

DcSystemPage *dc_system_page_new(void) {
    DcSystemPage *page = g_new0(DcSystemPage, 1);

    GtkWidget *content  = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    GtkWidget *scroller = gtk_scrolled_window_new(NULL, NULL);
    add_css_class(content, "sys-shell");
    add_css_class(scroller, "sys-scroll-hidden");

    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroller),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_overlay_scrolling(GTK_SCROLLED_WINDOW(scroller), TRUE);
    gtk_container_add(GTK_CONTAINER(scroller), content);
    g_signal_connect(scroller, "realize", G_CALLBACK(on_realize), NULL);

    /* ================================================================ */
    /*  Region & Language                                               */
    /* ================================================================ */
    GtkWidget *lang_card = create_card();
    GtkWidget *lang_box = gtk_bin_get_child(GTK_BIN(lang_card));

    gtk_box_pack_start(GTK_BOX(lang_box), create_group_label("REGION & LANGUAGE"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(lang_box), create_combo_row("Language", "Select the system display language", &page->combo_locale), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(content), lang_card, FALSE, FALSE, 0);

    /* ================================================================ */
    /*  Date & Time                                                     */
    /* ================================================================ */
    GtkWidget *time_card = create_card();
    GtkWidget *time_box = gtk_bin_get_child(GTK_BIN(time_card));

    gtk_box_pack_start(GTK_BOX(time_box), create_group_label("DATE & TIME"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(time_box), create_switch_row("Automatic Date & Time", "Require internet access", &page->switch_ntp), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(time_box), create_combo_row("Time Zone", "Select your geographical region", &page->combo_timezone), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(content), time_card, FALSE, FALSE, 0);

    page->root = scroller;
    return page;
}

void dc_system_page_free(DcSystemPage *page) {
    if (page) g_free(page);
}

GtkWidget *dc_system_page_get_widget(DcSystemPage *page) {
    return page ? page->root : NULL;
}

GtkWidget *dc_system_page_get_locale_combo(DcSystemPage *page) {
    return page ? page->combo_locale : NULL;
}

GtkWidget *dc_system_page_get_timezone_combo(DcSystemPage *page) {
    return page ? page->combo_timezone : NULL;
}

GtkWidget *dc_system_page_get_ntp_switch(DcSystemPage *page) {
    return page ? page->switch_ntp : NULL;
}
