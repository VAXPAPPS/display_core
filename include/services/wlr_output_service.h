#ifndef DC_WLR_OUTPUT_SERVICE_H
#define DC_WLR_OUTPUT_SERVICE_H

/*
 * wlr_output_service.h
 *
 * واجهة خدمة إدارة الشاشات عبر بروتوكول Wayland wlr-output-management-unstable-v1.
 * توفر نفس الواجهة العامة لـ xrandr_service لتسهيل التبديل عبر طبقة التجريد.
 */

#include "domain/display_types.h"
#include "services/display_edit_service.h"

#include <wayland-client.h>

/* -------- أنواع VRR (مشتركة مع xrandr_service) -------- */
typedef struct {
    gboolean any_supported;
    gboolean any_writable;
    guint    connected_outputs;
    guint    supported_outputs;
    guint    writable_outputs;
} DcWlrVrrSupportInfo;

/* -------- البنية الداخلية للخدمة -------- */

/* وصف داخلي لوضع عرض Wayland */
typedef struct _DcWlrModeInfo DcWlrModeInfo;
struct _DcWlrModeInfo {
    struct zwlr_output_mode_v1 *wl_mode;
    int    width;
    int    height;
    int    refresh_mhz;   /* معدل التحديث بـ mHz */
    gboolean preferred;
    DcWlrModeInfo *next;
};

/* وصف داخلي لرأس شاشة Wayland */
typedef struct _DcWlrHeadInfo DcWlrHeadInfo;
struct _DcWlrHeadInfo {
    struct zwlr_output_head_v1 *wl_head;
    char    *name;
    char    *description;
    gboolean enabled;
    int      pos_x;
    int      pos_y;
    int      transform;         /* wl_output_transform */
    double   scale;
    gboolean adaptive_sync;
    DcWlrModeInfo *modes;       /* قائمة مترابطة من الأوضاع */
    DcWlrModeInfo *current_mode;
    DcWlrHeadInfo *next;
};

typedef struct {
    struct wl_display       *wl_display;
    struct wl_registry      *wl_registry;
    struct zwlr_output_manager_v1 *wlr_manager;

    /* قائمة الرؤوس المُكتشفة */
    DcWlrHeadInfo           *heads;

    /* آخر serial مستلم من حدث done */
    uint32_t                 last_serial;

    /* هل اكتمل التهيئة الأولي؟ */
    gboolean                 initialized;
} DcWlrOutputService;

/* -------- دوال دورة الحياة -------- */
DcWlrOutputService *dc_wlr_output_service_new(char **error_message);
void                dc_wlr_output_service_free(DcWlrOutputService *service);

/* -------- تحميل وتطبيق الإعدادات -------- */
gboolean dc_wlr_output_service_load_outputs(DcWlrOutputService *service,
                                            GPtrArray         **outputs,
                                            char              **error_message);

gboolean dc_wlr_output_service_apply_configs(DcWlrOutputService *service,
                                             GPtrArray          *configs,
                                             char              **error_message);

/* -------- Display Edit (VRR فقط — الباقي X11-only) -------- */
gboolean dc_wlr_output_service_has_vrr_support(DcWlrOutputService  *service,
                                               gboolean            *supported,
                                               char               **error_message);

gboolean dc_wlr_output_service_get_vrr_support_info(DcWlrOutputService  *service,
                                                    DcWlrVrrSupportInfo *info,
                                                    char               **error_message);

gboolean dc_wlr_output_service_apply_vrr(DcWlrOutputService *service,
                                         gboolean            enabled,
                                         char              **error_message);

/*
 * ملاحظة: apply_display_edit (الـ gamma / night-light / vibrance) تعتمد على
 * XRandR gamma ramps وليس لها مكافئ في بروتوكول Wayland الحالي.
 * في بيئة Wayland تُنفَّذ هذه الميزات عبر XRandR fallback أو تُعطَّل.
 */

#endif /* DC_WLR_OUTPUT_SERVICE_H */
