#ifndef DC_DEFAULT_APPS_SERVICE_H
#define DC_DEFAULT_APPS_SERVICE_H

#include <glib-object.h>

G_BEGIN_DECLS

#define DC_TYPE_DEFAULT_APPS_SERVICE (dc_default_apps_service_get_type())
G_DECLARE_FINAL_TYPE(DcDefaultAppsService, dc_default_apps_service, DC, DEFAULT_APPS_SERVICE, GObject)

typedef struct {
    char *id;
    char *name;
} DcAppChoice;

DcDefaultAppsService *dc_default_apps_service_new(void);

// Returns a contiguous array of DcAppChoice terminated by id=NULL. Result must be freed by caller.
DcAppChoice *dc_default_apps_service_get_choices_for_type(DcDefaultAppsService *self, const char *mime_type);
void dc_app_choices_free(DcAppChoice *choices);

// Returns the ID of the default app, or NULL if none. Caller must g_free.
char *dc_default_apps_service_get_default_id(DcDefaultAppsService *self, const char *mime_type);

// Sets the default app for the mime type by ID. Returns TRUE on success.
gboolean dc_default_apps_service_set_default(DcDefaultAppsService *self, const char *mime_type, const char *app_id);

G_END_DECLS

#endif /* DC_DEFAULT_APPS_SERVICE_H */
