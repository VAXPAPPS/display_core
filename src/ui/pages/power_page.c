#include "ui/pages/power_page.h"

struct _DcPowerPage {
    GtkWidget *root;

    /* Display & Keyboard */
    GtkWidget *brightness_scale;
    GtkWidget *kb_brightness_scale;
    GtkWidget *kb_brightness_row;

    /* Power & Battery */
    GtkWidget *profiles_combo;
    GtkWidget *profiles_row;
    GtkWidget *battery_status_label;

    /* Energy Saving */
    GtkWidget *dim_combo;
    GtkWidget *blank_combo;
    GtkWidget *suspend_combo;

    /* Hardware Actions */
    GtkWidget *lid_ac_combo;
    GtkWidget *lid_bat_combo;
    GtkWidget *power_btn_combo;
    GtkWidget *critical_batt_combo;
};

/* ------------------------------------------------------------------ */
/*  CSS (Dark Glassmorphism)                                          */
/* ------------------------------------------------------------------ */
static const char *POWER_CSS =
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
    "combobox button:hover { background-color: rgba(255,255,255,0.14); }";

static void on_realize(GtkWidget *widget, gpointer user_data) {
    (void)user_data;
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider, POWER_CSS, -1, NULL);
    gtk_style_context_add_provider_for_screen(
        gtk_widget_get_screen(widget),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
    );
    g_object_unref(provider);
}

/* ------------------------------------------------------------------ */
/*  دوال مساعدة (Helper Functions)                                     */
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
    if (control) gtk_box_pack_end(GTK_BOX(row), control,  FALSE, FALSE, 0);
    
    return row;
}

static GtkWidget *create_card(const char *title, const char *subtitle) {
    GtkWidget *frame      = gtk_frame_new(NULL);
    GtkWidget *outer_box  = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    (void) title; (void) subtitle;

    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);
    gtk_container_set_border_width(GTK_CONTAINER(frame), 0);
    add_css_class(frame, "comp-card");

    gtk_container_add(GTK_CONTAINER(frame), outer_box);
    return frame;
}

/* ------------------------------------------------------------------ */
/*  البناء الرئيسي (Main Builder)                                      */
/* ------------------------------------------------------------------ */
DcPowerPage *dc_power_page_new(void) {
    DcPowerPage *page = g_new0(DcPowerPage, 1);

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
    /*  Card 1: Display & Keyboard                                      */
    /* ================================================================ */
    GtkWidget *display_card = create_card("Display & Keyboard", "");
    GtkWidget *display_box = gtk_bin_get_child(GTK_BIN(display_card));

    page->brightness_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.0, 100.0, 1.0);
    page->kb_brightness_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.0, 100.0, 1.0);

    gtk_widget_set_size_request(page->brightness_scale, 160, -1);
    gtk_widget_set_size_request(page->kb_brightness_scale, 160, -1);
    gtk_scale_set_draw_value(GTK_SCALE(page->brightness_scale), TRUE);
    gtk_scale_set_draw_value(GTK_SCALE(page->kb_brightness_scale), TRUE);

    page->kb_brightness_row = create_setting_row_impl("Keyboard Backlight", "Adjust keyboard brightness", page->kb_brightness_scale);

    gtk_box_pack_start(GTK_BOX(display_box), create_group_label("BRIGHTNESS"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(display_box), create_setting_row_impl("Screen Brightness", "Adjust the backlight of your display", page->brightness_scale), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(display_box), page->kb_brightness_row, FALSE, FALSE, 0);

    /* ================================================================ */
    /*  Card 2: Power & Battery                                         */
    /* ================================================================ */
    GtkWidget *power_card = create_card("Power & Battery", "");
    GtkWidget *power_box = gtk_bin_get_child(GTK_BIN(power_card));

    page->profiles_combo = gtk_combo_box_text_new();
    page->battery_status_label = gtk_label_new("Calculating...");
    gtk_label_set_xalign(GTK_LABEL(page->battery_status_label), 1.0);
    add_css_class(page->battery_status_label, "comp-setting-desc");

    page->profiles_row = create_setting_row_impl("Power Profile", "Choose the system-wide power profile", page->profiles_combo);
    
    gtk_box_pack_start(GTK_BOX(power_box), create_group_label("STATUS"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(power_box), page->profiles_row, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(power_box), create_setting_row_impl("Battery Status", "Current battery charge level and state", page->battery_status_label), FALSE, FALSE, 0);

    /* ================================================================ */
    /*  Card 3: Energy Saving                                           */
    /* ================================================================ */
    GtkWidget *energy_card = create_card("Energy Saving", "");
    GtkWidget *energy_box = gtk_bin_get_child(GTK_BIN(energy_card));

    page->dim_combo = gtk_combo_box_text_new();
    page->blank_combo = gtk_combo_box_text_new();
    page->suspend_combo = gtk_combo_box_text_new();

    struct { GtkWidget *combo; const char *values[8]; const char *ids[8]; } timeouts[] = {
        { page->dim_combo, {"1 minute", "2 minutes", "5 minutes", "10 minutes", "Never", NULL}, {"60", "120", "300", "600", "0", NULL} },
        { page->blank_combo, {"2 minutes", "5 minutes", "10 minutes", "15 minutes", "30 minutes", "Never", NULL}, {"120", "300", "600", "900", "1800", "0", NULL} },
        { page->suspend_combo, {"5 minutes", "15 minutes", "30 minutes", "1 hour", "2 hours", "Never", NULL}, {"300", "900", "1800", "3600", "7200", "0", NULL} }
    };

    for (int i = 0; i < 3; i++) {
        for (int j = 0; timeouts[i].values[j]; j++) {
            gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(timeouts[i].combo), timeouts[i].ids[j], timeouts[i].values[j]);
        }
    }

    gtk_box_pack_start(GTK_BOX(energy_box), create_group_label("IDLE TIMEOUTS"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(energy_box), create_setting_row_impl("Dim Screen", "Dim the display after being idle", page->dim_combo), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(energy_box), create_setting_row_impl("Turn Off Screen", "Turn off the display completely after being idle", page->blank_combo), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(energy_box), create_setting_row_impl("Suspend System", "Suspend the system to memory after being idle", page->suspend_combo), FALSE, FALSE, 0);

    /* ================================================================ */
    /*  Card 4: Hardware Actions                                        */
    /* ================================================================ */
    GtkWidget *hw_card = create_card("Hardware Actions", "");
    GtkWidget *hw_box = gtk_bin_get_child(GTK_BIN(hw_card));

    page->lid_ac_combo = gtk_combo_box_text_new();
    page->lid_bat_combo = gtk_combo_box_text_new();
    page->power_btn_combo = gtk_combo_box_text_new();
    page->critical_batt_combo = gtk_combo_box_text_new();

    const char *actions[] = {"suspend", "hibernate", "poweroff", "lock", "ignore", NULL};
    GtkWidget *combos[] = {page->lid_ac_combo, page->lid_bat_combo, page->power_btn_combo, page->critical_batt_combo};
    
    for (int i = 0; i < 4; i++) {
        for (int j = 0; actions[j]; j++) {
            gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(combos[i]), actions[j], actions[j]);
        }
    }

    gtk_box_pack_start(GTK_BOX(hw_box), create_group_label("LAPTOP LID"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hw_box), create_setting_row_impl("Lid Close (AC)", "Action to take when lid is closed on power", page->lid_ac_combo), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hw_box), create_setting_row_impl("Lid Close (Battery)", "Action to take when lid is closed on battery", page->lid_bat_combo), FALSE, FALSE, 0);
    
    gtk_box_pack_start(GTK_BOX(hw_box), create_group_label("BUTTONS & TRIGGERS"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hw_box), create_setting_row_impl("Power Button", "Action to take when the physical power button is pressed", page->power_btn_combo), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hw_box), create_setting_row_impl("Critical Battery", "Action to take when the battery reaches a critical level", page->critical_batt_combo), FALSE, FALSE, 0);

    /* Add cards to main box */
    gtk_box_pack_start(GTK_BOX(content), display_card, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(content), power_card, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(content), energy_card, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(content), hw_card, FALSE, FALSE, 0);

    page->root = scroller;
    return page;
}

void dc_power_page_free(DcPowerPage *page) {
    g_free(page);
}

void dc_power_page_set_battery_status(DcPowerPage *page, double percentage, gboolean charging, gint64 time_to_empty) {
    if (!page->battery_status_label) return;
    
    char status[128];
    if (percentage < 0) {
        snprintf(status, sizeof(status), "Not present");
    } else {
        snprintf(status, sizeof(status), "%.0f%% - %s", percentage, charging ? "Charging" : "Discharging");
        if (!charging && time_to_empty > 0) {
            snprintf(status + strlen(status), sizeof(status) - strlen(status), " (%ld mins left)", time_to_empty / 60);
        }
    }
    gtk_label_set_text(GTK_LABEL(page->battery_status_label), status);
}

GtkWidget *dc_power_page_get_widget(DcPowerPage *page) { return page->root; }
GtkWidget *dc_power_page_get_brightness_scale(DcPowerPage *page) { return page->brightness_scale; }
GtkWidget *dc_power_page_get_kb_brightness_scale(DcPowerPage *page) { return page->kb_brightness_scale; }
GtkWidget *dc_power_page_get_profiles_combo(DcPowerPage *page) { return page->profiles_combo; }
GtkWidget *dc_power_page_get_dim_combo(DcPowerPage *page) { return page->dim_combo; }
GtkWidget *dc_power_page_get_blank_combo(DcPowerPage *page) { return page->blank_combo; }
GtkWidget *dc_power_page_get_suspend_combo(DcPowerPage *page) { return page->suspend_combo; }
GtkWidget *dc_power_page_get_lid_ac_combo(DcPowerPage *page) { return page->lid_ac_combo; }
GtkWidget *dc_power_page_get_lid_bat_combo(DcPowerPage *page) { return page->lid_bat_combo; }
GtkWidget *dc_power_page_get_power_btn_combo(DcPowerPage *page) { return page->power_btn_combo; }
GtkWidget *dc_power_page_get_critical_batt_combo(DcPowerPage *page) { return page->critical_batt_combo; }
