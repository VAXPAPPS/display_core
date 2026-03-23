#ifndef DC_POWER_SERVICE_H
#define DC_POWER_SERVICE_H

#include <glib-object.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define DC_TYPE_POWER_SERVICE (dc_power_service_get_type())
G_DECLARE_FINAL_TYPE(DcPowerService, dc_power_service, DC, POWER_SERVICE, GObject)

DcPowerService *dc_power_service_new(void);

/* Brightness */
int  dc_power_service_get_brightness(DcPowerService *self);
void dc_power_service_set_brightness(DcPowerService *self, int level);

/* Keyboard Brightness */
gboolean dc_power_service_is_keyboard_supported(DcPowerService *self);
int      dc_power_service_get_keyboard_brightness(DcPowerService *self);
void     dc_power_service_set_keyboard_brightness(DcPowerService *self, int level);

/* Battery Info */
void dc_power_service_get_battery_info(DcPowerService *self, double *percentage, gboolean *charging, gint64 *time_to_empty);
gboolean dc_power_service_get_power_source(DcPowerService *self);

/* Profiles */
gboolean dc_power_service_is_profiles_available(DcPowerService *self);
gchar*   dc_power_service_get_active_profile(DcPowerService *self);
void     dc_power_service_set_active_profile(DcPowerService *self, const gchar *profile);
char**   dc_power_service_get_profiles(DcPowerService *self);

/* Timeouts */
void dc_power_service_get_idle_timeouts(DcPowerService *self, guint *dim, guint *blank, guint *suspend);
void dc_power_service_set_idle_timeouts(DcPowerService *self, guint dim, guint blank, guint suspend);

/* Actions */
void dc_power_service_get_lid_action(DcPowerService *self, gchar **ac_action, gchar **batt_action);
void dc_power_service_set_lid_action(DcPowerService *self, const gchar *ac_action, const gchar *batt_action);

void dc_power_service_get_power_button_action(DcPowerService *self, gchar **action);
void dc_power_service_set_power_button_action(DcPowerService *self, const gchar *action);

void dc_power_service_get_critical_action(DcPowerService *self, gchar **action);
void dc_power_service_set_critical_action(DcPowerService *self, const gchar *action);

G_END_DECLS

#endif // DC_POWER_SERVICE_H
