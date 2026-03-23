#include "app_controller_internal.h"

/* ------------------------------------------------------------------ */
/* LOAD DATA                                                          */
/* ------------------------------------------------------------------ */
void dc_app_system_load(DcAppController *app) {
    if (!app->system_page || !app->system_service) return;

    GtkWidget *combo_tz = dc_system_page_get_timezone_combo(app->system_page);
    GtkWidget *sw_ntp = dc_system_page_get_ntp_switch(app->system_page);
    GtkWidget *combo_loc = dc_system_page_get_locale_combo(app->system_page);

    g_signal_handlers_block_by_func(sw_ntp, G_CALLBACK(gtk_switch_get_active), NULL); // Pseudo block
    // We will block by data inside signals function actually. 
    // It's safer to just populate first without signals connected (since load runs before connect).

    // 1. NTP Switch
    gboolean ntp = dc_system_service_get_ntp_status(app->system_service);
    gtk_switch_set_active(GTK_SWITCH(sw_ntp), ntp);

    // 2. Timezones
    gtk_combo_box_text_remove_all(GTK_COMBO_BOX_TEXT(combo_tz));
    int tz_count = 0;
    char **tzs = dc_system_service_get_timezones(app->system_service, &tz_count);
    if (tzs) {
        for (int i = 0; i < tz_count; i++) {
            gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(combo_tz), tzs[i], tzs[i]);
            g_free(tzs[i]);
        }
        g_free(tzs);
    }
    char *cur_tz = dc_system_service_get_current_timezone(app->system_service);
    if (cur_tz) {
        gtk_combo_box_set_active_id(GTK_COMBO_BOX_TEXT(combo_tz), cur_tz);
        g_free(cur_tz);
    }

    // 3. Locales
    gtk_combo_box_text_remove_all(GTK_COMBO_BOX_TEXT(combo_loc));
    int loc_count = 0;
    char **locs = dc_system_service_get_locales(app->system_service, &loc_count);
    if (locs) {
        for (int i = 0; i < loc_count; i++) {
            gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(combo_loc), locs[i], locs[i]);
            g_free(locs[i]);
        }
        g_free(locs);
    }
    char *cur_loc = dc_system_service_get_current_locale(app->system_service);
    if (cur_loc) {
        gtk_combo_box_set_active_id(GTK_COMBO_BOX_TEXT(combo_loc), cur_loc);
        g_free(cur_loc);
    }
}

/* ------------------------------------------------------------------ */
/* SIGNALS                                                            */
/* ------------------------------------------------------------------ */
static void on_ntp_switched(GtkSwitch *sw, GParamSpec *pspec, gpointer user_data) {
    (void)pspec;
    DcAppController *app = user_data;
    gboolean is_on = gtk_switch_get_active(sw);
    dc_system_service_set_ntp_status(app->system_service, is_on);
}

static void on_timezone_changed(GtkComboBox *combo, gpointer user_data) {
    DcAppController *app = user_data;
    const char *tz = gtk_combo_box_get_active_id(combo);
    if (tz) {
        dc_system_service_set_timezone(app->system_service, tz);
    }
}

static void on_locale_changed(GtkComboBox *combo, gpointer user_data) {
    DcAppController *app = user_data;
    const char *loc = gtk_combo_box_get_active_id(combo);
    if (loc) {
        dc_system_service_set_locale(app->system_service, loc);
    }
}

void dc_app_system_connect_signals(DcAppController *app) {
    if (!app->system_page) return;

    GtkWidget *combo_tz = dc_system_page_get_timezone_combo(app->system_page);
    GtkWidget *sw_ntp = dc_system_page_get_ntp_switch(app->system_page);
    GtkWidget *combo_loc = dc_system_page_get_locale_combo(app->system_page);

    if (sw_ntp) {
        g_signal_connect(sw_ntp, "notify::active", G_CALLBACK(on_ntp_switched), app);
    }
    if (combo_tz) {
        g_signal_connect(combo_tz, "changed", G_CALLBACK(on_timezone_changed), app);
    }
    if (combo_loc) {
        g_signal_connect(combo_loc, "changed", G_CALLBACK(on_locale_changed), app);
    }
}
