#include "app_controller_internal.h"
#include <string.h>

/* Defines the internal mapping for our UI combo boxes to actual XDG mime types */
typedef struct {
    const char *combo_key;
    const char *mime_type;
} MimeMap;

static const MimeMap MIME_MAPS[] = {
    { "web",   "x-scheme-handler/http" },
    { "mail",  "x-scheme-handler/mailto" },
    { "file",  "inode/directory" },
    { "text",  "text/plain" },
    { "video", "video/mp4" },
    { "audio", "audio/mpeg" },
    { "image", "image/jpeg" },
    { NULL, NULL }
};

static void populate_combo(DcAppController *app, const char *combo_key, const char *mime_type) {
    if (!app->default_apps_page || !app->default_apps_service) return;

    GtkWidget *combo = dc_default_apps_page_get_combo(app->default_apps_page, combo_key);
    if (!combo) return;

    // Clear existing
    gtk_combo_box_text_remove_all(GTK_COMBO_BOX_TEXT(combo));

    DcAppChoice *choices = dc_default_apps_service_get_choices_for_type(app->default_apps_service, mime_type);
    if (!choices) return;

    for (int i = 0; choices[i].id != NULL; i++) {
        gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(combo), choices[i].id, choices[i].name);
    }
    
    char *def_id = dc_default_apps_service_get_default_id(app->default_apps_service, mime_type);
    if (def_id) {
        gtk_combo_box_set_active_id(GTK_COMBO_BOX_TEXT(combo), def_id);
        g_free(def_id);
    } else if (choices[0].id) {
        // Fallback to first if no default is formally set
        gtk_combo_box_set_active_id(GTK_COMBO_BOX_TEXT(combo), choices[0].id);
    }
    
    dc_app_choices_free(choices);
}

void dc_app_default_apps_load(DcAppController *app) {
    for (int i = 0; MIME_MAPS[i].combo_key != NULL; i++) {
        populate_combo(app, MIME_MAPS[i].combo_key, MIME_MAPS[i].mime_type);
    }
}

static void on_combo_changed(GtkComboBox *combo, gpointer user_data) {
    DcAppController *app = user_data;
    const char *combo_key = g_object_get_data(G_OBJECT(combo), "dc-combo-key");
    if (!combo_key) return;

    const char *mime_type = NULL;
    for (int i = 0; MIME_MAPS[i].combo_key != NULL; i++) {
        if (strcmp(MIME_MAPS[i].combo_key, combo_key) == 0) {
            mime_type = MIME_MAPS[i].mime_type;
            break;
        }
    }
    if (!mime_type) return;

    const char *app_id = gtk_combo_box_get_active_id(combo);
    if (app_id) {
        dc_default_apps_service_set_default(app->default_apps_service, mime_type, app_id);
    }
}

void dc_app_default_apps_connect_signals(DcAppController *app) {
    if (!app->default_apps_page) return;

    for (int i = 0; MIME_MAPS[i].combo_key != NULL; i++) {
        GtkWidget *combo = dc_default_apps_page_get_combo(app->default_apps_page, MIME_MAPS[i].combo_key);
        if (combo) {
            // Attach the key so the callback knows which type was clicked
            g_object_set_data(G_OBJECT(combo), "dc-combo-key", (gpointer)MIME_MAPS[i].combo_key);
            g_signal_connect(combo, "changed", G_CALLBACK(on_combo_changed), app);
        }
    }
}
