#include "ui/pages/display_page.h"

/* ------------------------------------------------------------------ */
/*  struct                                                              */
/* ------------------------------------------------------------------ */
struct _DcDisplayPage {
    GtkWidget *root;
    GtkWidget *content_box;
    GtkWidget *status_label;
    GtkWidget *profile_combo;
    GtkWidget *profile_entry;
    GtkWidget *refresh_button;
    GtkWidget *apply_button;
    GtkWidget *extend_button;
    GtkWidget *mirror_button;
    GtkWidget *internal_button;
    GtkWidget *external_button;
    GtkWidget *save_profile_button;
    GtkWidget *load_profile_button;
};

/* ------------------------------------------------------------------ */
/*  CSS مدمج — Dark Glassmorphism متوافق مع ستايل التطبيق              */
/* ------------------------------------------------------------------ */
static const char *DISPLAY_CSS =
    ".disp-scroll-hidden scrollbar {"
    "  opacity: 0;"
    "  min-width: 0;"
    "  min-height: 0;"
    "}"
    ".disp-scroll-hidden scrollbar slider {"
    "  min-width: 0;"
    "  min-height: 0;"
    "}"
    ".disp-shell {"
    "  padding: 20px 18px 24px;"
    "}"

    /* ── عنوان الصفحة ── */
    ".disp-page-title {"
    "  color: rgba(255,255,255,0.97);"
    "  font-size: 24px;"
    "  font-weight: 800;"
    "}"
    ".disp-page-subtitle {"
    "  color: rgba(255,255,255,0.55);"
    "  font-size: 13px;"
    "}"

    /* ── كارد الـ toolbar ── */
    ".disp-toolbar-card {"
    "  background-color: rgba(14,14,14,0.72);"
    "  border: 1px solid rgba(255,255,255,0.10);"
    "  border-radius: 18px;"
    "  padding: 16px 18px;"
    "}"

    /* ── تسمية المجموعة داخل toolbar ── */
    ".disp-group-label {"
    "  color: rgba(255,255,255,0.28);"
    "  font-size: 10px;"
    "  font-weight: 700;"
    "  letter-spacing: 0.09em;"
    "  padding-bottom: 6px;"
    "}"

    /* ── كارد الـ preview ── */
    ".disp-preview-card {"
    "  background-color: rgba(14,14,14,0.72);"
    "  border: 1px solid rgba(255,255,255,0.10);"
    "  border-radius: 18px;"
    "  padding: 14px 16px;"
    "}"
    ".disp-preview-label {"
    "  color: rgba(255,255,255,0.97);"
    "  font-size: 13px;"
    "  font-weight: 700;"
    "  padding-bottom: 10px;"
    "}"
    ".disp-hint-label {"
    "  color: rgba(255,255,255,0.38);"
    "  font-size: 11.5px;"
    "  font-style: italic;"
    "  padding: 4px 2px 0;"
    "}"

    /* ── شريط الحالة السفلي ── */
    ".disp-bottom-bar {"
    "  padding: 6px 0 0;"
    "}"
    ".disp-status-pill {"
    "  color: rgba(255,255,255,0.85);"
    "  background-color: rgba(255,255,255,0.07);"
    "  border: 1px solid rgba(255,255,255,0.10);"
    "  border-radius: 999px;"
    "  padding: 7px 14px;"
    "  font-size: 12px;"
    "}"

    /* ── أزرار عامة ── */
    ".disp-btn {"
    "  background-color: rgba(255,255,255,0.10);"
    "  background-image: none;"
    "  border: 1px solid rgba(255,255,255,0.12);"
    "  border-radius: 12px;"
    "  padding: 8px 14px;"
    "  color: rgba(255,255,255,0.92);"
    "  font-size: 13px;"
    "  font-weight: 500;"
    "}"
    ".disp-btn:hover {"
    "  background-color: rgba(255,255,255,0.16);"
    "}"

    /* ── زر Apply (primary) ── */
    ".disp-btn-primary {"
    "  background-color: rgba(255,255,255,0.22);"
    "  background-image: none;"
    "  border: 1px solid rgba(255,255,255,0.18);"
    "  border-radius: 12px;"
    "  padding: 8px 18px;"
    "  color: rgba(255,255,255,0.97);"
    "  font-size: 13px;"
    "  font-weight: 600;"
    "}"
    ".disp-btn-primary:hover {"
    "  background-color: rgba(255,255,255,0.30);"
    "}"

    /* ── Entry ── */
    "entry {"
    "  background-color: rgba(255,255,255,0.08);"
    "  background-image: none;"
    "  border: 1px solid rgba(255,255,255,0.12);"
    "  border-radius: 10px;"
    "  padding: 7px 12px;"
    "  color: rgba(255,255,255,0.92);"
    "  font-size: 13px;"
    "}"
    "entry:focus {"
    "  border-color: rgba(255,255,255,0.30);"
    "  background-color: rgba(255,255,255,0.11);"
    "}"

    /* ── ComboBox ── */
    "combobox button {"
    "  background-color: rgba(255,255,255,0.08);"
    "  background-image: none;"
    "  border: 1px solid rgba(255,255,255,0.12);"
    "  border-radius: 10px;"
    "  padding: 6px 10px;"
    "  color: rgba(255,255,255,0.90);"
    "  font-size: 13px;"
    "}"
    "combobox button:hover {"
    "  background-color: rgba(255,255,255,0.14);"
    "}";

/* ------------------------------------------------------------------ */
/*  تحميل CSS عند realize                                              */
/* ------------------------------------------------------------------ */
static void on_realize(GtkWidget *widget, gpointer user_data) {
    (void)user_data;
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider, DISPLAY_CSS, -1, NULL);
    gtk_style_context_add_provider_for_screen(
        gtk_widget_get_screen(widget),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
    );
    g_object_unref(provider);
}

/* ------------------------------------------------------------------ */
/*  دوال مساعدة                                                        */
/* ------------------------------------------------------------------ */
static void add_css_class(GtkWidget *widget, const char *class_name) {
    gtk_style_context_add_class(gtk_widget_get_style_context(widget), class_name);
}

static GtkWidget *create_page_header(const char *title, const char *subtitle) {
    GtkWidget *box         = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    GtkWidget *title_label = gtk_label_new(title);
    GtkWidget *sub_label   = gtk_label_new(subtitle);

    gtk_widget_set_halign(title_label, GTK_ALIGN_START);
    gtk_widget_set_halign(sub_label,   GTK_ALIGN_START);
    add_css_class(title_label, "disp-page-title");
    add_css_class(sub_label,   "disp-page-subtitle");

    gtk_box_pack_start(GTK_BOX(box), title_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), sub_label,   FALSE, FALSE, 0);
    return box;
}

static GtkWidget *create_group_label(const char *text) {
    GtkWidget *label = gtk_label_new(text);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    add_css_class(label, "disp-group-label");
    return label;
}

/* ------------------------------------------------------------------ */
/*  البناء الرئيسي                                                     */
/* ------------------------------------------------------------------ */
DcDisplayPage *dc_display_page_new(GtkWidget *preview_widget) {
    DcDisplayPage *page = g_new0(DcDisplayPage, 1);

    /* ── الجذر: ScrolledWindow مع تمرير عمودي ── */
    GtkWidget *scroller = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroller),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_overlay_scrolling(GTK_SCROLLED_WINDOW(scroller), TRUE);
    add_css_class(scroller, "disp-scroll-hidden");
    page->root = scroller;
    g_signal_connect(page->root, "realize", G_CALLBACK(on_realize), NULL);

    GtkWidget *page_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);
    add_css_class(page_box, "disp-shell");
    gtk_container_add(GTK_CONTAINER(scroller), page_box);

    /* ── عنوان الصفحة ── */
    gtk_box_pack_start(GTK_BOX(page_box),
                       create_page_header("Display",
                                          "Control screens, saved layouts, and live X11 positioning."),
                       FALSE, FALSE, 0);

    /* ================================================================ */
    /*  كارد الـ Toolbar                                                 */
    /* ================================================================ */
    GtkWidget *toolbar_card = gtk_frame_new(NULL);
    GtkWidget *toolbar_box  = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_frame_set_shadow_type(GTK_FRAME(toolbar_card), GTK_SHADOW_NONE);
    gtk_container_set_border_width(GTK_CONTAINER(toolbar_card), 0);
    add_css_class(toolbar_card, "disp-toolbar-card");
    gtk_container_add(GTK_CONTAINER(toolbar_card), toolbar_box);

    /* مجموعة Quick Layouts */
    page->extend_button   = gtk_button_new_with_label("Extend");
    page->mirror_button   = gtk_button_new_with_label("Mirror");
    page->internal_button = gtk_button_new_with_label("Internal Only");
    page->external_button = gtk_button_new_with_label("External Only");

    add_css_class(page->extend_button,   "disp-btn");
    add_css_class(page->mirror_button,   "disp-btn");
    add_css_class(page->internal_button, "disp-btn");
    add_css_class(page->external_button, "disp-btn");

    GtkWidget *quick_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_box_pack_start(GTK_BOX(quick_box), page->extend_button,   FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(quick_box), page->mirror_button,   FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(quick_box), page->internal_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(quick_box), page->external_button, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(toolbar_box), create_group_label("QUICK LAYOUTS"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar_box), quick_box, FALSE, FALSE, 0);

    /* فاصل خفيف */
    GtkWidget *separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_widget_set_opacity(separator, 0.10);
    gtk_box_pack_start(GTK_BOX(toolbar_box), separator, FALSE, FALSE, 2);

    /* مجموعة Profiles */
    page->profile_combo        = gtk_combo_box_text_new();
    page->profile_entry        = gtk_entry_new();
    page->save_profile_button  = gtk_button_new_with_label("Save Profile");
    page->load_profile_button  = gtk_button_new_with_label("Load Profile");

    gtk_entry_set_placeholder_text(GTK_ENTRY(page->profile_entry), "Profile name...");
    add_css_class(page->save_profile_button, "disp-btn");
    add_css_class(page->load_profile_button, "disp-btn");

    GtkWidget *profile_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_box_pack_start(GTK_BOX(profile_box), page->profile_combo,       TRUE,  TRUE,  0);
    gtk_box_pack_start(GTK_BOX(profile_box), page->load_profile_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(profile_box), page->profile_entry,       TRUE,  TRUE,  0);
    gtk_box_pack_start(GTK_BOX(profile_box), page->save_profile_button, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(toolbar_box), create_group_label("PROFILES"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar_box), profile_box, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(page_box), toolbar_card, FALSE, FALSE, 0);

    /* ================================================================ */
    /*  كارد الـ Preview                                                 */
    /* ================================================================ */
    GtkWidget *preview_card  = gtk_frame_new(NULL);
    GtkWidget *preview_inner = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    GtkWidget *preview_title = gtk_label_new("Layout Preview");
    GtkWidget *hint_label    = gtk_label_new("Drag monitors in the preview to update X/Y positions.");

    gtk_frame_set_shadow_type(GTK_FRAME(preview_card), GTK_SHADOW_NONE);
    gtk_container_set_border_width(GTK_CONTAINER(preview_card), 0);
    add_css_class(preview_card,  "disp-preview-card");
    add_css_class(preview_title, "disp-preview-label");
    add_css_class(hint_label,    "disp-hint-label");

    gtk_widget_set_halign(preview_title, GTK_ALIGN_START);
    gtk_widget_set_halign(hint_label,    GTK_ALIGN_START);

    gtk_box_pack_start(GTK_BOX(preview_inner), preview_title,  FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(preview_inner), preview_widget, TRUE,  TRUE,  0);
    gtk_box_pack_start(GTK_BOX(preview_inner), hint_label,     FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(preview_card), preview_inner);

    gtk_box_pack_start(GTK_BOX(page_box), preview_card, FALSE, FALSE, 0);

    /* ================================================================ */
    /*  صندوق المحتوى (output rows يُضاف من الخارج)                     */
    /* ================================================================ */
    page->content_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_box_pack_start(GTK_BOX(page_box), page->content_box, FALSE, FALSE, 0);

    /* ================================================================ */
    /*  شريط الحالة + الأزرار السفلية                                    */
    /* ================================================================ */
    page->status_label  = gtk_label_new("● Connecting to X server...");
    page->refresh_button = gtk_button_new_with_label("Refresh");
    page->apply_button   = gtk_button_new_with_label("Apply");

    add_css_class(page->status_label,   "disp-status-pill");
    add_css_class(page->refresh_button, "disp-btn");
    add_css_class(page->apply_button,   "disp-btn-primary");
    gtk_widget_set_halign(page->status_label, GTK_ALIGN_START);

    GtkWidget *bottom_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    add_css_class(bottom_bar, "disp-bottom-bar");
    gtk_box_pack_start(GTK_BOX(bottom_bar), page->status_label,   TRUE,  TRUE,  0);
    gtk_box_pack_end  (GTK_BOX(bottom_bar), page->apply_button,   FALSE, FALSE, 0);
    gtk_box_pack_end  (GTK_BOX(bottom_bar), page->refresh_button, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(page_box), bottom_bar, FALSE, FALSE, 0);

    return page;
}

/* ------------------------------------------------------------------ */
/*  Getters                                                             */
/* ------------------------------------------------------------------ */
void       dc_display_page_free                  (DcDisplayPage *page) { g_free(page); }
GtkWidget *dc_display_page_get_widget            (DcDisplayPage *page) { return page->root; }
GtkWidget *dc_display_page_get_content_box       (DcDisplayPage *page) { return page->content_box; }
GtkWidget *dc_display_page_get_status_label      (DcDisplayPage *page) { return page->status_label; }
GtkWidget *dc_display_page_get_profile_combo     (DcDisplayPage *page) { return page->profile_combo; }
GtkWidget *dc_display_page_get_profile_entry     (DcDisplayPage *page) { return page->profile_entry; }
GtkWidget *dc_display_page_get_refresh_button    (DcDisplayPage *page) { return page->refresh_button; }
GtkWidget *dc_display_page_get_apply_button      (DcDisplayPage *page) { return page->apply_button; }
GtkWidget *dc_display_page_get_extend_button     (DcDisplayPage *page) { return page->extend_button; }
GtkWidget *dc_display_page_get_mirror_button     (DcDisplayPage *page) { return page->mirror_button; }
GtkWidget *dc_display_page_get_internal_button   (DcDisplayPage *page) { return page->internal_button; }
GtkWidget *dc_display_page_get_external_button   (DcDisplayPage *page) { return page->external_button; }
GtkWidget *dc_display_page_get_save_profile_button(DcDisplayPage *page) { return page->save_profile_button; }
GtkWidget *dc_display_page_get_load_profile_button(DcDisplayPage *page) { return page->load_profile_button; }
