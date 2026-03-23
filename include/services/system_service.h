#ifndef DC_SYSTEM_SERVICE_H
#define DC_SYSTEM_SERVICE_H

#include <glib-object.h>

G_BEGIN_DECLS

#define DC_TYPE_SYSTEM_SERVICE (dc_system_service_get_type())
G_DECLARE_FINAL_TYPE(DcSystemService, dc_system_service, DC, SYSTEM_SERVICE, GObject)

DcSystemService *dc_system_service_new(void);

/* Timezone */
char **dc_system_service_get_timezones(DcSystemService *self, int *out_count);
char *dc_system_service_get_current_timezone(DcSystemService *self);
void dc_system_service_set_timezone(DcSystemService *self, const char *timezone);

/* NTP (Automatic Date & Time) */
gboolean dc_system_service_get_ntp_status(DcSystemService *self);
void dc_system_service_set_ntp_status(DcSystemService *self, gboolean enabled);

/* Locale */
char **dc_system_service_get_locales(DcSystemService *self, int *out_count);
char *dc_system_service_get_current_locale(DcSystemService *self);
void dc_system_service_set_locale(DcSystemService *self, const char *locale);

G_END_DECLS

#endif /* DC_SYSTEM_SERVICE_H */
