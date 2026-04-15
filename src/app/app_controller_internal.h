#ifndef DC_APP_CONTROLLER_INTERNAL_H
#define DC_APP_CONTROLLER_INTERNAL_H

#include <gtk/gtk.h>

#include "services/xrandr_service.h"
#include "ui/custom_headerbar.h"
#include "ui/output_row.h"
#include "ui/pages/audio_page.h"
#include "ui/pages/compositor_page.h"
#include "ui/pages/display_edit_page.h"
#include "ui/pages/display_page.h"
#include "ui/pages/themes_page.h"
#include "ui/pages/window_manager_page.h"
#include "ui/pages/power_page.h"
#include "ui/pages/keyboard_page.h"
#include "ui/pages/mouse_page.h"
#include "ui/pages/about_page.h"
#include "ui/pages/default_apps_page.h"
#include "ui/pages/system_page.h"
#include "ui/pages/bluetooth_page.h"
#include "services/power_service.h"
#include "services/input_service.h"
#include "services/sysinfo_service.h"
#include "services/default_apps_service.h"
#include "services/system_service.h"
#include "services/bluetooth_service.h"
#include "ui/preview_canvas.h"

#define DC_REVERT_TIMEOUT_SECONDS 15
#define DC_PRIMARY_VENOM_CONFIG_PATH "/home/x/.config/venom-miasma/venom.conf"
#define DC_FALLBACK_VENOM_CONFIG_PATH "/etc/venom/venom.conf"

typedef struct {
    DcXrandrService *service;
    GtkApplication *gtk_app;
    GtkWidget *window;
    GtkWidget *stack;
    DcDisplayPage *display_page;
    DcAudioPage *audio_page;
    DcThemesPage *themes_page;
    DcDisplayEditPage *display_edit_page;
    DcWindowManagerPage *window_manager_page;
    DcCompositorPage *compositor_page;
    DcPowerPage *power_page;
    DcKeyboardPage *keyboard_page;
    DcMousePage *mouse_page;
    DcAboutPage *about_page;
    DcDefaultAppsPage *default_apps_page;
    DcSystemPage *system_page;
    DcBluetoothPage *bluetooth_page;
    DcPowerService *power_service;
    DcInputService *input_service;
    DcSysinfoService *sysinfo_service;
    DcDefaultAppsService *default_apps_service;
    DcSystemService *system_service;
    DcBluetoothService *bluetooth_service;
    GPtrArray *output_models;
    GPtrArray *rows;
    DcPreviewCanvas *preview;
    guint compositor_autosave_timeout_id;
    gboolean suppress_compositor_autosave;
    guint themes_autosave_timeout_id;
    gboolean suppress_themes_autosave;
    guint window_manager_autosave_timeout_id;
    gboolean suppress_window_manager_autosave;
    guint display_edit_autosave_timeout_id;
    gboolean suppress_display_edit_autosave;
    guint display_edit_refresh_timeout_id;
    gboolean suppress_audio_updates;
    guint audio_refresh_idle_id;
    gboolean suppress_power_updates;
    gboolean suppress_input_updates;
} DcAppController;

typedef struct {
    GtkWidget *dialog;
    GtkWidget *label;
    guint timeout_id;
    gint remaining_seconds;
} DcRevertDialogData;

void dc_app_add_css_class(GtkWidget *widget, const char *class_name);
void dc_app_install_css(void);
void dc_app_set_status(DcAppController *app, const char *message);
char *dc_app_resolve_venom_config_path(void);

void dc_app_clear_rows(DcAppController *app);
void dc_app_reload_outputs(DcAppController *app);
void dc_app_connect_display_page_signals(DcAppController *app);

void dc_app_compositor_load(DcAppController *app);
void dc_app_compositor_connect_signals(DcAppController *app);

void dc_app_audio_load(DcAppController *app);
void dc_app_audio_connect_signals(DcAppController *app);

void dc_app_display_edit_load(DcAppController *app);
void dc_app_display_edit_configure_capabilities(DcAppController *app);
void dc_app_display_edit_connect_signals(DcAppController *app);
gboolean dc_app_display_edit_refresh_runtime(gpointer user_data);

void dc_app_themes_load(DcAppController *app);
void dc_app_themes_connect_signals(DcAppController *app);

void dc_app_window_manager_load(DcAppController *app);
void dc_app_window_manager_connect_signals(DcAppController *app);

void dc_app_power_load(DcAppController *app);
void dc_app_power_connect_signals(DcAppController *app);

void dc_app_keyboard_load(DcAppController *app);
void dc_app_keyboard_connect_signals(DcAppController *app);

void dc_app_mouse_load(DcAppController *app);
void dc_app_mouse_connect_signals(DcAppController *app);

void dc_app_about_load(DcAppController *app);

void dc_app_default_apps_load(DcAppController *app);
void dc_app_default_apps_connect_signals(DcAppController *app);

void dc_app_system_load(DcAppController *app);
void dc_app_system_connect_signals(DcAppController *app);

void dc_app_bluetooth_load(DcAppController *app);
void dc_app_bluetooth_connect_signals(DcAppController *app);

#endif
