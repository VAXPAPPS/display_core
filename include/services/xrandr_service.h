#ifndef DC_XRANDR_SERVICE_H
#define DC_XRANDR_SERVICE_H

#include "domain/display_types.h"
#include "services/display_edit_service.h"

typedef struct {
    Display *display;
    Window root;
} DcXrandrService;

typedef struct {
    gboolean any_supported;
    gboolean any_writable;
    guint connected_outputs;
    guint supported_outputs;
    guint writable_outputs;
} DcVrrSupportInfo;

DcXrandrService *dc_xrandr_service_new(char **error_message);
void dc_xrandr_service_free(DcXrandrService *service);

gboolean dc_xrandr_service_load_outputs(DcXrandrService *service,
                                        GPtrArray **outputs,
                                        char **error_message);
gboolean dc_xrandr_service_apply_configs(DcXrandrService *service,
                                         GPtrArray *configs,
                                         char **error_message);
gboolean dc_xrandr_service_apply_display_edit(DcXrandrService *service,
                                              const DcDisplayEditConfig *config,
                                              char **error_message);
gboolean dc_xrandr_service_reset_display_edit(DcXrandrService *service,
                                              char **error_message);
gboolean dc_xrandr_service_get_vrr_support_info(DcXrandrService *service,
                                                DcVrrSupportInfo *info,
                                                char **error_message);
gboolean dc_xrandr_service_has_vrr_support(DcXrandrService *service,
                                           gboolean *supported,
                                           char **error_message);
gboolean dc_xrandr_service_apply_vrr(DcXrandrService *service,
                                     gboolean enabled,
                                     char **error_message);

#endif
