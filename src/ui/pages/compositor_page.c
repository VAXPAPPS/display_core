#include "ui/pages/compositor_page.h"

/* ------------------------------------------------------------------ */
/*  struct — يبقى هنا في الـ .c فقط (opaque type في الـ .h)           */
/* ------------------------------------------------------------------ */
struct _DcCompositorPage {
    GtkWidget *root;
    GtkWidget *shadow_switch;
    GtkWidget *shadow_radius_scale;
    GtkWidget *shadow_opacity_scale;
    GtkWidget *shadow_red_scale;
    GtkWidget *shadow_green_scale;
    GtkWidget *shadow_blue_scale;
    GtkWidget *fading_switch;
    GtkWidget *active_opacity_scale;
    GtkWidget *inactive_opacity_scale;
    GtkWidget *corner_radius_scale;
    GtkWidget *detect_rounded_switch;
    GtkWidget *blur_method_combo;
    GtkWidget *blur_strength_scale;
    GtkWidget *blur_background_switch;
    GtkWidget *blur_background_frame_switch;
    GtkWidget *backend_combo;
    GtkWidget *vsync_switch;
    GtkWidget *use_damage_switch;
};

/* ------------------------------------------------------------------ */
/*  CSS مدمج — Dark Glassmorphism متوافق مع ستايل التطبيق              */
/* ------------------------------------------------------------------ */
static const char *COMPOSITOR_CSS =
    ".comp-shell {"
    "  padding: 20px 18px 48px;"
    "}"
    ".comp-page-title {"
    "  color: rgba(255,255,255,0.97);"
    "  font-size: 24px;"
    "  font-weight: 800;"
    "}"
    ".comp-page-subtitle {"
    "  color: rgba(255,255,255,0.55);"
    "  font-size: 13px;"
    "}"
    ".comp-toolbar {"
    "  padding: 6px 0 4px;"
    "}"
    ".comp-status-pill {"
    "  color: rgba(255,255,255,0.85);"
    "  background-color: rgba(255,255,255,0.07);"
    "  border: 1px solid rgba(255,255,255,0.10);"
    "  border-radius: 999px;"
    "  padding: 7px 14px;"
    "  font-size: 12px;"
    "}"
    ".comp-card {"
    "  background-color: rgba(14,14,14,0.72);"
    "  border: 1px solid rgba(255,255,255,0.10);"
    "  border-radius: 18px;"
    "}"
    ".comp-card-header {"
    "  padding: 14px 18px 13px;"
    "  border-bottom: 1px solid rgba(255,255,255,0.07);"
    "}"
    ".comp-card-title {"
    "  color: rgba(255,255,255,0.97);"
    "  font-size: 14px;"
    "  font-weight: 700;"
    "}"
    ".comp-card-subtitle {"
    "  color: rgba(255,255,255,0.45);"
    "  font-size: 12px;"
    "}"
    ".comp-group-label {"
    "  color: rgba(255,255,255,0.28);"
    "  font-size: 10px;"
    "  font-weight: 700;"
    "  letter-spacing: 0.09em;"
    "  padding: 10px 18px 2px;"
    "}"
    ".comp-row {"
    "  padding: 10px 18px;"
    "  border-bottom: 1px solid rgba(255,255,255,0.05);"
    "}"
    ".comp-setting-title {"
    "  color: rgba(255,255,255,0.92);"
    "  font-size: 13px;"
    "  font-weight: 600;"
    "}"
    ".comp-setting-desc {"
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
    "}"
    ".comp-btn {"
    "  background-color: rgba(255,255,255,0.10);"
    "  background-image: none;"
    "  border: 1px solid rgba(255,255,255,0.12);"
    "  border-radius: 12px;"
    "  padding: 8px 16px;"
    "  color: rgba(255,255,255,0.92);"
    "  font-size: 13px;"
    "  font-weight: 500;"
    "}"
    ".comp-btn:hover {"
    "  background-color: rgba(255,255,255,0.16);"
    "}"
    ".comp-btn-primary {"
    "  background-color: rgba(255,255,255,0.22);"
    "  background-image: none;"
    "  border: 1px solid rgba(255,255,255,0.18);"
    "  border-radius: 12px;"
    "  padding: 8px 16px;"
    "  color: rgba(255,255,255,0.97);"
    "  font-size: 13px;"
    "  font-weight: 600;"
    "}"
    ".comp-btn-primary:hover {"
    "  background-color: rgba(255,255,255,0.30);"
    "}";

/* ------------------------------------------------------------------ */
/*  تحميل CSS عند realize                                              */
/* ------------------------------------------------------------------ */
static void on_realize(GtkWidget *widget, gpointer user_data) {
    (void)user_data;
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider, COMPOSITOR_CSS, -1, NULL);
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

static GtkWidget *create_group_label(const char *text) {
    GtkWidget *label = gtk_label_new(text);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    add_css_class(label, "comp-group-label");
    return label;
}

static GtkWidget *create_setting_row(const char *title,
                                     const char *description,
                                     GtkWidget  *control) {
    GtkWidget *row      = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    GtkWidget *text_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 3);
    GtkWidget *t_label  = gtk_label_new(title);
    GtkWidget *d_label  = gtk_label_new(description);

    gtk_widget_set_halign(t_label,  GTK_ALIGN_START);
    gtk_widget_set_halign(d_label,  GTK_ALIGN_START);
    gtk_widget_set_hexpand(text_box, TRUE);
    gtk_label_set_line_wrap(GTK_LABEL(d_label), TRUE);
    gtk_widget_set_valign(control,  GTK_ALIGN_CENTER);

    add_css_class(t_label, "comp-setting-title");
    add_css_class(d_label, "comp-setting-desc");
    add_css_class(row,     "comp-row");

    gtk_box_pack_start(GTK_BOX(text_box), t_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(text_box), d_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(row), text_box, TRUE,  TRUE,  0);
    gtk_box_pack_end  (GTK_BOX(row), control,  FALSE, FALSE, 0);
    return row;
}

static GtkWidget *create_card(const char *title, const char *subtitle) {
    GtkWidget *frame      = gtk_frame_new(NULL);
    GtkWidget *outer_box  = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    (void) title;
    (void) subtitle;

    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);
    gtk_container_set_border_width(GTK_CONTAINER(frame), 0);

    add_css_class(frame,      "comp-card");

    gtk_container_add(GTK_CONTAINER(frame), outer_box);
    return frame;
}

/* ------------------------------------------------------------------ */
/*  البناء الرئيسي                                                     */
/* ------------------------------------------------------------------ */
DcCompositorPage *dc_compositor_page_new(void) {
    DcCompositorPage *page = g_new0(DcCompositorPage, 1);

    GtkWidget *content  = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);
    GtkWidget *scroller = gtk_scrolled_window_new(NULL, NULL);
    add_css_class(content, "comp-shell");

    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroller),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_overlay_scrolling(GTK_SCROLLED_WINDOW(scroller), TRUE);
    gtk_container_add(GTK_CONTAINER(scroller), content);
    g_signal_connect(scroller, "realize", G_CALLBACK(on_realize), NULL);

    /* ================================================================ */
    /*  كارد Effects                                                     */
    /* ================================================================ */
    GtkWidget *effects_card = create_card(
        "Effects",
        "Edit the live compositor configuration stored in venom.conf.");
    GtkWidget *effects_box = gtk_bin_get_child(GTK_BIN(effects_card));

    page->shadow_switch                = gtk_switch_new();
    page->shadow_radius_scale          = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.0, 80.0,  1.0);
    page->shadow_opacity_scale         = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.0, 100.0, 1.0);
    page->shadow_red_scale             = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.0, 1.0,   0.01);
    page->shadow_green_scale           = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.0, 1.0,   0.01);
    page->shadow_blue_scale            = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.0, 1.0,   0.01);
    page->fading_switch                = gtk_switch_new();
    page->active_opacity_scale         = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.0, 100.0, 1.0);
    page->inactive_opacity_scale       = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.0, 100.0, 1.0);
    page->corner_radius_scale          = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.0, 48.0,  1.0);
    page->detect_rounded_switch        = gtk_switch_new();
    page->blur_method_combo            = gtk_combo_box_text_new();
    page->blur_strength_scale          = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.0, 20.0,  1.0);
    page->blur_background_switch       = gtk_switch_new();
    page->blur_background_frame_switch = gtk_switch_new();

    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(page->blur_method_combo), "none",        "none");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(page->blur_method_combo), "dual_kawase", "dual_kawase");

    GtkWidget *scales[] = {
        page->shadow_radius_scale,  page->shadow_opacity_scale,
        page->shadow_red_scale,     page->shadow_green_scale,
        page->shadow_blue_scale,
        page->active_opacity_scale, page->inactive_opacity_scale,
        page->corner_radius_scale,  page->blur_strength_scale
    };
    for (int i = 0; i < 9; i++) {
        gtk_widget_set_size_request(scales[i], 160, -1);
        gtk_widget_set_hexpand(scales[i], FALSE);
        gtk_scale_set_draw_value(GTK_SCALE(scales[i]), TRUE);
    }

    gtk_box_pack_start(GTK_BOX(effects_box), create_group_label("SHADOWS"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(effects_box), create_setting_row("Shadows",        "Enable or disable global compositor shadows.",             page->shadow_switch),        FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(effects_box), create_setting_row("Shadow Radius",  "Adjust the softness and spread of shadows around windows.", page->shadow_radius_scale),  FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(effects_box), create_setting_row("Shadow Opacity", "Control how visible the compositor shadow should appear.",  page->shadow_opacity_scale), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(effects_box), create_setting_row("Shadow Red",     "Set the red channel of shadow color from 0.0 to 1.0.",      page->shadow_red_scale),     FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(effects_box), create_setting_row("Shadow Green",   "Set the green channel of shadow color from 0.0 to 1.0.",    page->shadow_green_scale),   FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(effects_box), create_setting_row("Shadow Blue",    "Set the blue channel of shadow color from 0.0 to 1.0.",     page->shadow_blue_scale),    FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(effects_box), create_group_label("FADING & OPACITY"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(effects_box), create_setting_row("Fading",           "Enable fade in and fade out transitions for windows.",     page->fading_switch),          FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(effects_box), create_setting_row("Active Opacity",   "Set the opacity of the currently focused window.",         page->active_opacity_scale),   FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(effects_box), create_setting_row("Inactive Opacity", "Dim or preserve unfocused windows using explicit opacity.", page->inactive_opacity_scale), FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(effects_box), create_group_label("CORNERS & BLUR"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(effects_box), create_setting_row("Corner Radius",          "Control the roundness used by compositor window corners.",       page->corner_radius_scale),          FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(effects_box), create_setting_row("Detect Rounded Corners", "Prevent blur from leaking outside transparent rounded corners.", page->detect_rounded_switch),        FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(effects_box), create_setting_row("Blur Method",            "Choose the blur algorithm declared in venom.conf.",              page->blur_method_combo),            FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(effects_box), create_setting_row("Blur Strength",          "Tune the strength of the chosen blur method.",                   page->blur_strength_scale),          FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(effects_box), create_setting_row("Blur Background",        "Blur transparent backgrounds behind client windows.",            page->blur_background_switch),       FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(effects_box), create_setting_row("Blur Frames",            "Apply background blur to window frames when supported.",         page->blur_background_frame_switch), FALSE, FALSE, 0);

    /* ================================================================ */
    /*  كارد Rendering                                                   */
    /* ================================================================ */
    GtkWidget *performance_card = create_card(
        "Rendering",
        "Backend and synchronization values are written back into the same config file.");
    GtkWidget *performance_box = gtk_bin_get_child(GTK_BIN(performance_card));

    page->backend_combo     = gtk_combo_box_text_new();
    page->vsync_switch      = gtk_switch_new();
    page->use_damage_switch = gtk_switch_new();

    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(page->backend_combo), "glx",     "glx");
    gtk_box_pack_start(GTK_BOX(performance_box), create_group_label("BACKEND & SYNC"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(performance_box), create_setting_row("VSync",      "Reduce tearing and keep compositing aligned with the monitor refresh.", page->vsync_switch),      FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(performance_box), create_setting_row("Use Damage", "Allow damage tracking to reduce redraw work and lower CPU usage.",      page->use_damage_switch), FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(content), effects_card,     FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(content), performance_card, FALSE, FALSE, 0);

    page->root = scroller;
    return page;
}

/* ------------------------------------------------------------------ */
/*  Getters                                                             */
/* ------------------------------------------------------------------ */
void       dc_compositor_page_free                            (DcCompositorPage *page) { g_free(page); }
GtkWidget *dc_compositor_page_get_widget                      (DcCompositorPage *page) { return page->root; }
GtkWidget *dc_compositor_page_get_shadow_switch               (DcCompositorPage *page) { return page->shadow_switch; }
GtkWidget *dc_compositor_page_get_shadow_radius_scale         (DcCompositorPage *page) { return page->shadow_radius_scale; }
GtkWidget *dc_compositor_page_get_shadow_opacity_scale        (DcCompositorPage *page) { return page->shadow_opacity_scale; }
GtkWidget *dc_compositor_page_get_shadow_red_scale            (DcCompositorPage *page) { return page->shadow_red_scale; }
GtkWidget *dc_compositor_page_get_shadow_green_scale          (DcCompositorPage *page) { return page->shadow_green_scale; }
GtkWidget *dc_compositor_page_get_shadow_blue_scale           (DcCompositorPage *page) { return page->shadow_blue_scale; }
GtkWidget *dc_compositor_page_get_fading_switch               (DcCompositorPage *page) { return page->fading_switch; }
GtkWidget *dc_compositor_page_get_active_opacity_scale        (DcCompositorPage *page) { return page->active_opacity_scale; }
GtkWidget *dc_compositor_page_get_inactive_opacity_scale      (DcCompositorPage *page) { return page->inactive_opacity_scale; }
GtkWidget *dc_compositor_page_get_corner_radius_scale         (DcCompositorPage *page) { return page->corner_radius_scale; }
GtkWidget *dc_compositor_page_get_detect_rounded_switch       (DcCompositorPage *page) { return page->detect_rounded_switch; }
GtkWidget *dc_compositor_page_get_blur_method_combo           (DcCompositorPage *page) { return page->blur_method_combo; }
GtkWidget *dc_compositor_page_get_blur_strength_scale         (DcCompositorPage *page) { return page->blur_strength_scale; }
GtkWidget *dc_compositor_page_get_blur_background_switch      (DcCompositorPage *page) { return page->blur_background_switch; }
GtkWidget *dc_compositor_page_get_blur_background_frame_switch(DcCompositorPage *page) { return page->blur_background_frame_switch; }
GtkWidget *dc_compositor_page_get_backend_combo               (DcCompositorPage *page) { return page->backend_combo; }
GtkWidget *dc_compositor_page_get_vsync_switch                (DcCompositorPage *page) { return page->vsync_switch; }
GtkWidget *dc_compositor_page_get_use_damage_switch           (DcCompositorPage *page) { return page->use_damage_switch; }
