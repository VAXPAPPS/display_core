#include "app_controller_internal.h"
#include <string.h>

/* ----------------------------------------------------- */
/*  Helpers                                             */
/* ----------------------------------------------------- */
static void reload_layouts_ui(DcAppController *app);

static void on_remove_layout(GtkWidget *widget, gpointer user_data) {
    DcAppController *app = user_data;
    const char *code = g_object_get_data(G_OBJECT(widget), "layout_code");
    if (code) {
        if (dc_input_service_remove_keyboard_layout(app->input_service, code)) {
            // Reload UI
            reload_layouts_ui(app);
        }
    }
}

static void reload_layouts_ui(DcAppController *app) {
    dc_keyboard_page_clear_layouts(app->keyboard_page);
    char **layouts = dc_input_service_list_keyboard_layouts(app->input_service);
    if (!layouts) return;

    for (int i = 0; layouts[i]; i++) {
        const char *name = dc_keyboard_page_lookup_layout_name(layouts[i]);
        dc_keyboard_page_add_layout_row(app->keyboard_page, layouts[i], name, G_CALLBACK(on_remove_layout), app);
        g_free(layouts[i]);
    }
    g_free(layouts);
}

/* ----------------------------------------------------- */
/*  Signal Handlers from UI                             */
/* ----------------------------------------------------- */

static void on_add_layout_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn;
    DcAppController *app = user_data;
    const char *code = dc_keyboard_page_get_selected_add_layout(app->keyboard_page);
    if (code) {
        if (dc_input_service_add_keyboard_layout(app->input_service, code)) {
            reload_layouts_ui(app);
        }
    }
}

static void on_apply_options_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn;
    DcAppController *app = user_data;
    const gchar *options = gtk_entry_get_text(GTK_ENTRY(dc_keyboard_page_get_options_entry(app->keyboard_page)));
    
    // We only send it to dbus
    dc_input_service_set_keyboard_options(app->input_service, options);
}

static void on_model_combo_changed(GtkComboBox *combo, gpointer user_data) {
    DcAppController *app = user_data;
    if (app->suppress_input_updates) return;
    
    gchar *model = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(combo));
    if (model) {
        dc_input_service_set_keyboard_model(app->input_service, model);
        g_free(model);
    }
}

/* ----------------------------------------------------- */
/*  Public Setup                                        */
/* ----------------------------------------------------- */

void dc_app_keyboard_load(DcAppController *app) {
    app->suppress_input_updates = TRUE;

    // Load active layouts into the ListBox
    reload_layouts_ui(app);

    // Get current device settings directly
    gchar *layouts = NULL, *model = NULL, *opts = NULL;
    dc_input_service_get_keyboard_settings(app->input_service, &layouts, &model, &opts);

    if (model) {
        GtkComboBox *m_combo = GTK_COMBO_BOX(dc_keyboard_page_get_model_combo(app->keyboard_page));
        gtk_combo_box_set_active_id(m_combo, model);
    }
    
    if (opts) {
        gtk_entry_set_text(GTK_ENTRY(dc_keyboard_page_get_options_entry(app->keyboard_page)), opts);
    }

    g_free(layouts);
    g_free(model);
    g_free(opts);

    app->suppress_input_updates = FALSE;
}

void dc_app_keyboard_connect_signals(DcAppController *app) {
    g_signal_connect(dc_keyboard_page_get_add_button(app->keyboard_page), "clicked", G_CALLBACK(on_add_layout_clicked), app);
    g_signal_connect(dc_keyboard_page_get_apply_options_button(app->keyboard_page), "clicked", G_CALLBACK(on_apply_options_clicked), app);
    g_signal_connect(dc_keyboard_page_get_model_combo(app->keyboard_page), "changed", G_CALLBACK(on_model_combo_changed), app);
}
