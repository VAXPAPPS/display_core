#ifndef DC_INPUT_SERVICE_H
#define DC_INPUT_SERVICE_H

#include <glib-object.h>

G_BEGIN_DECLS

#define DC_TYPE_INPUT_SERVICE (dc_input_service_get_type())
G_DECLARE_FINAL_TYPE(DcInputService, dc_input_service, DC, INPUT_SERVICE, GObject)

DcInputService *dc_input_service_new(void);

/* Keyboard */
gboolean dc_input_service_set_keyboard_layouts(DcInputService *self, const gchar *layouts);
gboolean dc_input_service_set_keyboard_model(DcInputService *self, const gchar *model);
gboolean dc_input_service_set_keyboard_options(DcInputService *self, const gchar *options);
void     dc_input_service_get_keyboard_settings(DcInputService *self, gchar **layouts, gchar **model, gchar **options);
gboolean dc_input_service_add_keyboard_layout(DcInputService *self, const gchar *layout);
gboolean dc_input_service_remove_keyboard_layout(DcInputService *self, const gchar *layout);
char   **dc_input_service_list_keyboard_layouts(DcInputService *self);

/* Mouse */
gboolean dc_input_service_set_mouse_accel(DcInputService *self, double accel);
gboolean dc_input_service_set_mouse_speed(DcInputService *self, double speed);
gboolean dc_input_service_set_mouse_natural_scroll(DcInputService *self, gboolean enabled);
gboolean dc_input_service_set_mouse_left_handed(DcInputService *self, gboolean enabled);
void     dc_input_service_get_mouse_settings(DcInputService *self, double *accel, double *speed, gboolean *natural_scroll, gboolean *left_handed);

/* Touchpad */
gboolean dc_input_service_set_touchpad_enabled(DcInputService *self, gboolean enabled);
gboolean dc_input_service_set_touchpad_tap_to_click(DcInputService *self, gboolean enabled);
gboolean dc_input_service_set_touchpad_natural_scroll(DcInputService *self, gboolean enabled);
gboolean dc_input_service_set_touchpad_scroll_method(DcInputService *self, const gchar *method);
gboolean dc_input_service_set_touchpad_speed(DcInputService *self, double speed);
gboolean dc_input_service_set_touchpad_disable_while_typing(DcInputService *self, gboolean enabled);
void     dc_input_service_get_touchpad_settings(DcInputService *self, gboolean *enabled, gboolean *tap_to_click, gboolean *natural_scroll, gchar **scroll_method, double *speed, gboolean *disable_while_typing);

G_END_DECLS

#endif // DC_INPUT_SERVICE_H
