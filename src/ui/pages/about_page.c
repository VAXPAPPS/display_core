#include "ui/pages/about_page.h"
#include <unistd.h>

struct _DcAboutPage {
    GtkWidget *root;

    GtkWidget *lbl_distro;
    GtkWidget *lbl_de;
    GtkWidget *lbl_kernel;
    GtkWidget *lbl_os_type;
    GtkWidget *lbl_model;
    GtkWidget *lbl_shell;
    
    GtkWidget *lbl_ram;
    GtkWidget *lbl_disk;
    
    GtkWidget *lbl_graphics;
};

/* ------------------------------------------------------------------ */
/*  CSS (Dark Glassmorphism)                                          */
/* ------------------------------------------------------------------ */
static const char *ABOUT_CSS =
    ".abt-scroll-hidden scrollbar { opacity: 0; min-width: 0; min-height: 0; }"
    ".abt-shell { padding: 40px 18px 48px; }"
    ".abt-logo { margin-bottom: 24px; opacity: 0.9; }"
    ".abt-group-label { color: rgba(255,255,255,0.28); font-size: 10px; font-weight: 700; letter-spacing: 0.09em; padding: 10px 18px 2px; }"
    ".abt-row { padding: 12px 18px; border-bottom: 1px solid rgba(255,255,255,0.05); }"
    ".abt-title { color: rgba(255,255,255,0.92); font-size: 13px; font-weight: 700; }"
    ".abt-val { color: rgba(255,255,255,0.70); font-size: 13px; }"
    ".abt-card { background-color: rgba(14,14,14,0.72); border: 1px solid rgba(255,255,255,0.10); border-radius: 18px; margin-bottom: 24px; }";

static void on_realize(GtkWidget *widget, gpointer user_data) {
    (void)user_data;
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider, ABOUT_CSS, -1, NULL);
    gtk_style_context_add_provider_for_screen(
        gtk_widget_get_screen(widget),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
    );
    g_object_unref(provider);
}

/* ------------------------------------------------------------------ */
/*  Helpers                                                           */
/* ------------------------------------------------------------------ */
static void add_css_class(GtkWidget *widget, const char *class_name) {
    gtk_style_context_add_class(gtk_widget_get_style_context(widget), class_name);
}

static GtkWidget *create_group_label(const char *text) {
    GtkWidget *label = gtk_label_new(text);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    add_css_class(label, "abt-group-label");
    return label;
}

static GtkWidget *create_info_row(const char *title, GtkWidget **out_value_lbl) {
    GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    GtkWidget *t_lbl = gtk_label_new(title);
    GtkWidget *v_lbl = gtk_label_new("");

    gtk_widget_set_halign(t_lbl, GTK_ALIGN_START);
    gtk_widget_set_halign(v_lbl, GTK_ALIGN_END);
    gtk_widget_set_hexpand(t_lbl, TRUE);
    gtk_label_set_selectable(GTK_LABEL(v_lbl), TRUE);
    gtk_label_set_line_wrap(GTK_LABEL(v_lbl), TRUE);
    gtk_label_set_max_width_chars(GTK_LABEL(v_lbl), 60);

    add_css_class(t_lbl, "abt-title");
    add_css_class(v_lbl, "abt-val");
    add_css_class(row, "abt-row");

    gtk_box_pack_start(GTK_BOX(row), t_lbl, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(row), v_lbl, FALSE, FALSE, 0);

    *out_value_lbl = v_lbl;
    return row;
}

static GtkWidget *create_card(void) {
    GtkWidget *frame = gtk_frame_new(NULL);
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);
    gtk_container_set_border_width(GTK_CONTAINER(frame), 0);
    add_css_class(frame, "abt-card");

    gtk_container_add(GTK_CONTAINER(frame), vbox);
    return frame;
}

static const char *find_logo_path(void) {
    static char logo_path[512];
    const char *paths[] = {
        "assets/settingsx.svg",
        "/usr/share/vsysinfo/settingsx.svg"
    };
    
    for (int i = 0; i < 2; i++) {
        if (access(paths[i], F_OK) == 0) {
            strncpy(logo_path, paths[i], sizeof(logo_path) - 1);
            logo_path[sizeof(logo_path) - 1] = '\0';
            return logo_path;
        }
    }
    return NULL;
}

/* ------------------------------------------------------------------ */
/*  Main Builder                                                      */
/* ------------------------------------------------------------------ */
DcAboutPage *dc_about_page_new(void) {
    DcAboutPage *page = g_new0(DcAboutPage, 1);

    GtkWidget *content  = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    GtkWidget *scroller = gtk_scrolled_window_new(NULL, NULL);
    add_css_class(content, "abt-shell");
    add_css_class(scroller, "abt-scroll-hidden");

    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroller),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_overlay_scrolling(GTK_SCROLLED_WINDOW(scroller), TRUE);
    gtk_container_add(GTK_CONTAINER(scroller), content);
    g_signal_connect(scroller, "realize", G_CALLBACK(on_realize), NULL);

    /* ================================================================ */
    /*  Logo Header                                                     */
    /* ================================================================ */
    GtkWidget *logo_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_halign(logo_box, GTK_ALIGN_CENTER);
    add_css_class(logo_box, "abt-logo");

    const char *lp = find_logo_path();
    GtkWidget *img = lp ? gtk_image_new_from_file(lp) : gtk_image_new();
    gtk_box_pack_start(GTK_BOX(logo_box), img, FALSE, FALSE, 0);
    
    gtk_box_pack_start(GTK_BOX(content), logo_box, FALSE, FALSE, 0);

    /* ================================================================ */
    /*  System Information Card                                         */
    /* ================================================================ */
    GtkWidget *sys_card = create_card();
    GtkWidget *sys_box = gtk_bin_get_child(GTK_BIN(sys_card));

    gtk_box_pack_start(GTK_BOX(sys_box), create_group_label("SYSTEM INFORMATION"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(sys_box), create_info_row("Distribution", &page->lbl_distro), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(sys_box), create_info_row("Desktop Environment", &page->lbl_de), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(sys_box), create_info_row("Kernel Version", &page->lbl_kernel), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(sys_box), create_info_row("OS Type", &page->lbl_os_type), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(sys_box), create_info_row("Computer Model", &page->lbl_model), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(sys_box), create_info_row("Shell", &page->lbl_shell), FALSE, FALSE, 0);

    /* ================================================================ */
    /*  Hardware Information Card                                       */
    /* ================================================================ */
    GtkWidget *hw_card = create_card();
    GtkWidget *hw_box = gtk_bin_get_child(GTK_BIN(hw_card));

    gtk_box_pack_start(GTK_BOX(hw_box), create_group_label("HARDWARE"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hw_box), create_info_row("Total RAM", &page->lbl_ram), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hw_box), create_info_row("Total Disk Space", &page->lbl_disk), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hw_box), create_info_row("Graphics Adapters", &page->lbl_graphics), FALSE, FALSE, 0);

    /* Assemble main view */
    gtk_box_pack_start(GTK_BOX(content), sys_card, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(content), hw_card, FALSE, FALSE, 0);

    page->root = scroller;
    return page;
}

void dc_about_page_free(DcAboutPage *page) {
    g_free(page);
}

GtkWidget *dc_about_page_get_widget(DcAboutPage *page) {
    return page->root;
}

void dc_about_page_set_os_info(DcAboutPage *page, const char *distro, const char *de, const char *kernel, const char *os_type, const char *model, const char *shell) {
    gtk_label_set_text(GTK_LABEL(page->lbl_distro), distro ? distro : "");
    gtk_label_set_text(GTK_LABEL(page->lbl_de), de ? de : "");
    gtk_label_set_text(GTK_LABEL(page->lbl_kernel), kernel ? kernel : "");
    gtk_label_set_text(GTK_LABEL(page->lbl_os_type), os_type ? os_type : "");
    gtk_label_set_text(GTK_LABEL(page->lbl_model), model ? model : "");
    gtk_label_set_text(GTK_LABEL(page->lbl_shell), shell ? shell : "");
}

void dc_about_page_set_hardware_info(DcAboutPage *page, const char *ram, const char *disk) {
    gtk_label_set_text(GTK_LABEL(page->lbl_ram), ram ? ram : "");
    gtk_label_set_text(GTK_LABEL(page->lbl_disk), disk ? disk : "");
}

void dc_about_page_set_graphics_info(DcAboutPage *page, const char *graphics) {
    gtk_label_set_text(GTK_LABEL(page->lbl_graphics), graphics ? graphics : "");
}
