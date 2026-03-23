#ifndef DC_POWER_PAGE_H
#define DC_POWER_PAGE_H

#include <gtk/gtk.h>

typedef struct _DcPowerPage DcPowerPage;

DcPowerPage *dc_power_page_new(void);
void         dc_power_page_free(DcPowerPage *page);

/* Getters for UI elements */
GtkWidget *dc_power_page_get_widget(DcPowerPage *page);

/* Display & Keyboard */
GtkWidget *dc_power_page_get_brightness_scale(DcPowerPage *page);
GtkWidget *dc_power_page_get_kb_brightness_scale(DcPowerPage *page);

/* Power & Battery */
GtkWidget *dc_power_page_get_profiles_combo(DcPowerPage *page);
void       dc_power_page_set_battery_status(DcPowerPage *page, double percentage, gboolean charging, gint64 time_to_empty);

/* Energy Saving */
GtkWidget *dc_power_page_get_dim_combo(DcPowerPage *page);
GtkWidget *dc_power_page_get_blank_combo(DcPowerPage *page);
GtkWidget *dc_power_page_get_suspend_combo(DcPowerPage *page);

/* Hardware Actions */
GtkWidget *dc_power_page_get_lid_ac_combo(DcPowerPage *page);
GtkWidget *dc_power_page_get_lid_bat_combo(DcPowerPage *page);
GtkWidget *dc_power_page_get_power_btn_combo(DcPowerPage *page);
GtkWidget *dc_power_page_get_critical_batt_combo(DcPowerPage *page);

#endif // DC_POWER_PAGE_H
