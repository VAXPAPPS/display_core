#include "services/default_apps_service.h"
#include <gio/gio.h>
#include <string.h>

struct _DcDefaultAppsService {
    GObject parent_instance;
};

G_DEFINE_TYPE(DcDefaultAppsService, dc_default_apps_service, G_TYPE_OBJECT)

static void dc_default_apps_service_class_init(DcDefaultAppsServiceClass *klass) {
    (void)klass;
}

static void dc_default_apps_service_init(DcDefaultAppsService *self) {
    (void)self;
}

DcDefaultAppsService *dc_default_apps_service_new(void) {
    return g_object_new(DC_TYPE_DEFAULT_APPS_SERVICE, NULL);
}

DcAppChoice *dc_default_apps_service_get_choices_for_type(DcDefaultAppsService *self, const char *mime_type) {
    (void)self;
    GList *apps = g_app_info_get_all_for_type(mime_type);
    guint length = g_list_length(apps);
    
    DcAppChoice *choices = g_new0(DcAppChoice, length + 1);
    
    int i = 0;
    for (GList *l = apps; l != NULL; l = l->next) {
        GAppInfo *info = G_APP_INFO(l->data);
        const char *id = g_app_info_get_id(info);
        const char *name = g_app_info_get_name(info);
        
        if (id && name) {
            choices[i].id = g_strdup(id);
            choices[i].name = g_strdup(name);
            i++;
        }
    }
    g_list_free_full(apps, g_object_unref);
    return choices;
}

void dc_app_choices_free(DcAppChoice *choices) {
    if (!choices) return;
    for (int i = 0; choices[i].id != NULL; i++) {
        g_free(choices[i].id);
        g_free(choices[i].name);
    }
    g_free(choices);
}

char *dc_default_apps_service_get_default_id(DcDefaultAppsService *self, const char *mime_type) {
    (void)self;
    GAppInfo *info = g_app_info_get_default_for_type(mime_type, FALSE);
    if (!info) return NULL;
    
    char *id = g_strdup(g_app_info_get_id(info));
    g_object_unref(info);
    return id;
}

gboolean dc_default_apps_service_set_default(DcDefaultAppsService *self, const char *mime_type, const char *app_id) {
    (void)self;
    if (!app_id || !mime_type) return FALSE;
    
    GList *all_apps = g_app_info_get_all();
    GAppInfo *target_app = NULL;
    
    for (GList *l = all_apps; l != NULL; l = l->next) {
        GAppInfo *info = G_APP_INFO(l->data);
        const char *id = g_app_info_get_id(info);
        if (g_strcmp0(id, app_id) == 0) {
            target_app = g_object_ref(info);
            break;
        }
    }
    g_list_free_full(all_apps, g_object_unref);
    
    if (!target_app) return FALSE;
    
    GError *err = NULL;
    gboolean success = g_app_info_set_as_default_for_type(target_app, mime_type, &err);
    if (!success) {
        g_warning("Failed to set default app: %s", err ? err->message : "Unknown error");
        if (err) g_error_free(err);
    }
    
    g_object_unref(target_app);
    return success;
}
