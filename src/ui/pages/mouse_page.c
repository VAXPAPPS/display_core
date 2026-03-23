#include "ui/pages/mouse_page.h"

struct _DcMousePage {
    GtkWidget *root;

    /* Mouse */
    GtkWidget *mouse_speed_scale;
    GtkWidget *mouse_accel_scale;
    GtkWidget *mouse_natural_scroll_switch;
    GtkWidget *mouse_left_handed_switch;

    /* Touchpad */
    GtkWidget *touchpad_enabled_switch;
    GtkWidget *touchpad_tap_to_click_switch;
    GtkWidget *touchpad_natural_scroll_switch;
    GtkWidget *touchpad_scroll_method_combo;
    GtkWidget *touchpad_speed_scale;
    GtkWidget *touchpad_disable_while_typing_switch;
};

/* ------------------------------------------------------------------ */
/*  CSS (Dark Glassmorphism)                                          */
/* ------------------------------------------------------------------ */
static const char *MOUSE_CSS =
    ".comp-scroll-hidden scrollbar { opacity: 0; min-width: 0; min-height: 0; }"
    ".comp-scroll-hidden scrollbar slider { min-width: 0; min-height: 0; }"
    ".comp-shell { padding: 20px 18px 48px; }"
    ".comp-group-label { color: rgba(255,255,255,0.28); font-size: 10px; font-weight: 700; letter-spacing: 0.09em; padding: 10px 18px 2px; }"
    ".comp-row { padding: 10px 18px; border-bottom: 1px solid rgba(255,255,255,0.05); }"
    ".comp-setting-title { color: rgba(255,255,255,0.92); font-size: 13px; font-weight: 600; }"
    ".comp-setting-desc { color: rgba(255,255,255,0.45); font-size: 11.5px; }"
    ".comp-card { background-color: rgba(14,14,14,0.72); border: 1px solid rgba(255,255,255,0.10); border-radius: 18px; }"
    "scale trough { min-height: 4px; border-radius: 2px; background-color: rgba(255,255,255,0.13); }"
    "scale trough highlight { background-color: rgba(255,255,255,0.50); border-radius: 2px; }"
    "scale trough slider { min-width: 16px; min-height: 16px; border-radius: 50%; background-color: rgba(255,255,255,0.92); border: none; }"
    "scale value { color: rgba(255,255,255,0.38); font-size: 11px; min-width: 26px; }"
    "combobox button { background-color: rgba(255,255,255,0.08); background-image: none; border: 1px solid rgba(255,255,255,0.12); border-radius: 10px; padding: 6px 10px; color: rgba(255,255,255,0.90); font-size: 12px; }"
    "combobox button:hover { background-color: rgba(255,255,255,0.14); }"
    "switch slider { background-color: rgba(255,255,255,0.85); box-shadow: 0 1px 2px rgba(0,0,0,0.5); }"
    "switch:checked { background-color: rgba(60,160,250,0.8); border-color: rgba(60,160,250,0.9); }"
    "switch:checked slider { background-color: #ffffff; }";

static void on_realize(GtkWidget *widget, gpointer user_data) {
    (void)user_data;
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider, MOUSE_CSS, -1, NULL);
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
    add_css_class(label, "comp-group-label");
    return label;
}

static GtkWidget *create_setting_row_impl(const char *title, const char *description, GtkWidget *control) {
    GtkWidget *row      = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    GtkWidget *text_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 3);
    GtkWidget *t_label  = gtk_label_new(title);
    GtkWidget *d_label  = gtk_label_new(description);

    gtk_widget_set_halign(t_label,  GTK_ALIGN_START);
    gtk_widget_set_halign(d_label,  GTK_ALIGN_START);
    gtk_widget_set_hexpand(text_box, TRUE);
    gtk_label_set_line_wrap(GTK_LABEL(d_label), TRUE);
    if (control) gtk_widget_set_valign(control,  GTK_ALIGN_CENTER);

    add_css_class(t_label, "comp-setting-title");
    add_css_class(d_label, "comp-setting-desc");
    add_css_class(row,     "comp-row");

    gtk_box_pack_start(GTK_BOX(text_box), t_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(text_box), d_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(row), text_box, TRUE,  TRUE,  0);
    if (control) {
        if (GTK_IS_SCALE(control)) {
            // Give scales more flex
            gtk_widget_set_size_request(control, 130, -1);
        }
        gtk_box_pack_end(GTK_BOX(row), control,  FALSE, FALSE, 0);
    }
    
    return row;
}

static GtkWidget *create_card(void) {
    GtkWidget *frame      = gtk_frame_new(NULL);
    GtkWidget *outer_box  = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);
    gtk_container_set_border_width(GTK_CONTAINER(frame), 0);
    add_css_class(frame, "comp-card");

    gtk_container_add(GTK_CONTAINER(frame), outer_box);
    return frame;
}

static GtkWidget *create_scale(double min, double max, double step) {
    GtkWidget *scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, min, max, step);
    gtk_scale_set_draw_value(GTK_SCALE(scale), TRUE);
    gtk_scale_set_value_pos(GTK_SCALE(scale), GTK_POS_BOTTOM);
    return scale;
}

/* ------------------------------------------------------------------ */
/*  Main Builder                                                      */
/* ------------------------------------------------------------------ */
DcMousePage *dc_mouse_page_new(void) {
    DcMousePage *page = g_new0(DcMousePage, 1);

    GtkWidget *content  = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);
    GtkWidget *scroller = gtk_scrolled_window_new(NULL, NULL);
    add_css_class(content, "comp-shell");
    add_css_class(scroller, "comp-scroll-hidden");

    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroller),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_overlay_scrolling(GTK_SCROLLED_WINDOW(scroller), TRUE);
    gtk_container_add(GTK_CONTAINER(scroller), content);
    g_signal_connect(scroller, "realize", G_CALLBACK(on_realize), NULL);

    /* ================================================================ */
    /*  Card 1: General Mouse                                           */
    /* ================================================================ */
    GtkWidget *mouse_card = create_card();
    GtkWidget *mouse_box = gtk_bin_get_child(GTK_BIN(mouse_card));

    page->mouse_speed_scale = create_scale(0.0, 2.0, 0.1);
    page->mouse_accel_scale = create_scale(-1.0, 1.0, 0.1);
    page->mouse_natural_scroll_switch = gtk_switch_new();
    page->mouse_left_handed_switch = gtk_switch_new();

    gtk_box_pack_start(GTK_BOX(mouse_box), create_group_label("GENERAL MOUSE"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(mouse_box), create_setting_row_impl("Mouse Speed", "Adjust the pointer speed of your regular mouse", page->mouse_speed_scale), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(mouse_box), create_setting_row_impl("Mouse Acceleration", "Set acceleration profile (-1.0 to 1.0)", page->mouse_accel_scale), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(mouse_box), create_setting_row_impl("Natural Scrolling", "Scrolling moves content instead of scrollbar", page->mouse_natural_scroll_switch), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(mouse_box), create_setting_row_impl("Left Handed Mode", "Swap left and right mouse buttons", page->mouse_left_handed_switch), FALSE, FALSE, 0);

    /* ================================================================ */
    /*  Card 2: Touchpad                                                */
    /* ================================================================ */
    GtkWidget *tp_card = create_card();
    GtkWidget *tp_box = gtk_bin_get_child(GTK_BIN(tp_card));

    page->touchpad_enabled_switch = gtk_switch_new();
    page->touchpad_tap_to_click_switch = gtk_switch_new();
    page->touchpad_natural_scroll_switch = gtk_switch_new();
    page->touchpad_disable_while_typing_switch = gtk_switch_new();
    
    page->touchpad_scroll_method_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(page->touchpad_scroll_method_combo), "none", "None");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(page->touchpad_scroll_method_combo), "two-finger", "Two Finger");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(page->touchpad_scroll_method_combo), "edge", "Edge Scrolling");

    page->touchpad_speed_scale = create_scale(0.0, 2.0, 0.1); // Actually the doc says 0 to 1, but we allow 2 just in case

    gtk_box_pack_start(GTK_BOX(tp_box), create_group_label("TOUCHPAD"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(tp_box), create_setting_row_impl("Enable Touchpad", "Turn the laptop touchpad on or off", page->touchpad_enabled_switch), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(tp_box), create_setting_row_impl("Tap to Click", "Tap the touchpad to simulate a left click", page->touchpad_tap_to_click_switch), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(tp_box), create_setting_row_impl("Touchpad Speed", "Adjust the exact speed of the touchpad pointer", page->touchpad_speed_scale), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(tp_box), create_setting_row_impl("Scroll Method", "Choose how you want to scroll with the touchpad", page->touchpad_scroll_method_combo), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(tp_box), create_setting_row_impl("Natural Scrolling", "Scrolling downwards moves the page up", page->touchpad_natural_scroll_switch), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(tp_box), create_setting_row_impl("Disable While Typing", "Prevent accidental clicks while typing on the keyboard", page->touchpad_disable_while_typing_switch), FALSE, FALSE, 0);

    /* Add cards to main box */
    gtk_box_pack_start(GTK_BOX(content), mouse_card, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(content), tp_card, FALSE, FALSE, 0);

    page->root = scroller;
    return page;
}

void dc_mouse_page_free(DcMousePage *page) {
    g_free(page);
}

GtkWidget *dc_mouse_page_get_widget(DcMousePage *page) { return page->root; }

/* Mouse Accessors */
GtkWidget *dc_mouse_page_get_mouse_speed_scale(DcMousePage *page) { return page->mouse_speed_scale; }
GtkWidget *dc_mouse_page_get_mouse_accel_scale(DcMousePage *page) { return page->mouse_accel_scale; }
GtkWidget *dc_mouse_page_get_mouse_natural_scroll_switch(DcMousePage *page) { return page->mouse_natural_scroll_switch; }
GtkWidget *dc_mouse_page_get_mouse_left_handed_switch(DcMousePage *page) { return page->mouse_left_handed_switch; }

/* Touchpad Accessors */
GtkWidget *dc_mouse_page_get_touchpad_enabled_switch(DcMousePage *page) { return page->touchpad_enabled_switch; }
GtkWidget *dc_mouse_page_get_touchpad_tap_to_click_switch(DcMousePage *page) { return page->touchpad_tap_to_click_switch; }
GtkWidget *dc_mouse_page_get_touchpad_natural_scroll_switch(DcMousePage *page) { return page->touchpad_natural_scroll_switch; }
GtkWidget *dc_mouse_page_get_touchpad_scroll_method_combo(DcMousePage *page) { return page->touchpad_scroll_method_combo; }
GtkWidget *dc_mouse_page_get_touchpad_speed_scale(DcMousePage *page) { return page->touchpad_speed_scale; }
GtkWidget *dc_mouse_page_get_touchpad_disable_while_typing_switch(DcMousePage *page) { return page->touchpad_disable_while_typing_switch; }
