#ifndef DC_XRANDR_SERVICE_H
#define DC_XRANDR_SERVICE_H

#include "domain/display_types.h"

typedef struct {
    Display *display;
    Window root;
} DcXrandrService;

DcXrandrService *dc_xrandr_service_new(char **error_message);
void dc_xrandr_service_free(DcXrandrService *service);

gboolean dc_xrandr_service_load_outputs(DcXrandrService *service,
                                        GPtrArray **outputs,
                                        char **error_message);
gboolean dc_xrandr_service_apply_configs(DcXrandrService *service,
                                         GPtrArray *configs,
                                         char **error_message);

#endif
