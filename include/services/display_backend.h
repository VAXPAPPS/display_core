#ifndef DC_DISPLAY_BACKEND_H
#define DC_DISPLAY_BACKEND_H

/*
 * display_backend.h
 *
 * طبقة تجريد بين واجهة المستخدم وخدمة الشاشات الحقيقية.
 * تكتشف تلقائياً البيئة (Wayland / X11) وتوجّه الاستدعاءات للخدمة المناسبة.
 */

#include "domain/display_types.h"
#include "services/display_edit_service.h"
#include "services/xrandr_service.h"
#include "services/wlr_output_service.h"

typedef enum {
    DC_BACKEND_X11,
    DC_BACKEND_WAYLAND
} DcBackendType;

typedef struct {
    DcBackendType type;
    union {
        DcXrandrService    *x11;
        DcWlrOutputService *wlr;
    } impl;
} DcDisplayBackend;

/* -------- دورة الحياة -------- */
DcDisplayBackend *dc_display_backend_new(char **error_message);
void              dc_display_backend_free(DcDisplayBackend *backend);

/* -------- استعلام -------- */
DcBackendType dc_display_backend_get_type(const DcDisplayBackend *backend);

/* -------- تحميل وتطبيق -------- */
gboolean dc_display_backend_load_outputs(DcDisplayBackend *backend,
                                         GPtrArray       **outputs,
                                         char            **error_message);

gboolean dc_display_backend_apply_configs(DcDisplayBackend *backend,
                                          GPtrArray        *configs,
                                          char            **error_message);

/* -------- Display Edit (gamma/night-light — X11 فقط) -------- */
gboolean dc_display_backend_apply_display_edit(DcDisplayBackend        *backend,
                                               const DcDisplayEditConfig *config,
                                               char                   **error_message);

gboolean dc_display_backend_reset_display_edit(DcDisplayBackend *backend,
                                               char            **error_message);

/* -------- VRR -------- */
gboolean dc_display_backend_get_vrr_support_info(DcDisplayBackend *backend,
                                                  DcVrrSupportInfo *info,
                                                  char            **error_message);

gboolean dc_display_backend_has_vrr_support(DcDisplayBackend *backend,
                                             gboolean         *supported,
                                             char            **error_message);

gboolean dc_display_backend_apply_vrr(DcDisplayBackend *backend,
                                       gboolean          enabled,
                                       char            **error_message);

#endif /* DC_DISPLAY_BACKEND_H */
