/*
 * display_backend.c
 *
 * تنفيذ طبقة التجريد. تكتشف البيئة عند البدء وتوجّه كل استدعاء
 * إلى الخدمة المناسبة (Wayland أو X11).
 *
 * منطق الاختيار:
 *   1. إذا كان WAYLAND_DISPLAY موجوداً وتم تهيئة zwlr_output_manager بنجاح → Wayland
 *   2. وإلا → X11 / XRandR (الوضع الافتراضي السابق)
 */

#include "services/display_backend.h"

#include <string.h>

/* ============================================================
 * دورة الحياة
 * ============================================================ */

DcDisplayBackend *dc_display_backend_new(char **error_message) {
    DcDisplayBackend *backend;
    const char *wayland_display;

    backend = g_new0(DcDisplayBackend, 1);

    /* محاولة Wayland أولاً */
    wayland_display = g_getenv("WAYLAND_DISPLAY");
    if (wayland_display != NULL && *wayland_display != '\0') {
        char *wlr_error = NULL;

        backend->impl.wlr = dc_wlr_output_service_new(&wlr_error);
        if (backend->impl.wlr != NULL) {
            backend->type = DC_BACKEND_WAYLAND;
            g_free(wlr_error);
            return backend;
        }

        /* سجّل التحذير لكن تابع نحو X11 */
        g_warning("Wayland backend failed (%s), falling back to X11.",
                  wlr_error != NULL ? wlr_error : "unknown error");
        g_free(wlr_error);
    }

    /* X11 fallback */
    backend->type    = DC_BACKEND_X11;
    backend->impl.x11 = dc_xrandr_service_new(error_message);
    if (backend->impl.x11 == NULL) {
        g_free(backend);
        return NULL;
    }

    return backend;
}

void dc_display_backend_free(DcDisplayBackend *backend) {
    if (backend == NULL) return;

    if (backend->type == DC_BACKEND_WAYLAND) {
        dc_wlr_output_service_free(backend->impl.wlr);
    } else {
        dc_xrandr_service_free(backend->impl.x11);
    }

    g_free(backend);
}

/* ============================================================
 * استعلام
 * ============================================================ */

DcBackendType dc_display_backend_get_type(const DcDisplayBackend *backend) {
    if (backend == NULL) return DC_BACKEND_X11;
    return backend->type;
}

/* ============================================================
 * تحميل وتطبيق
 * ============================================================ */

gboolean dc_display_backend_load_outputs(DcDisplayBackend *backend,
                                         GPtrArray       **outputs,
                                         char            **error_message) {
    if (backend == NULL) {
        if (error_message) *error_message = g_strdup("Backend is not initialized.");
        return FALSE;
    }

    if (backend->type == DC_BACKEND_WAYLAND)
        return dc_wlr_output_service_load_outputs(backend->impl.wlr,
                                                  outputs, error_message);

    return dc_xrandr_service_load_outputs(backend->impl.x11,
                                          outputs, error_message);
}

gboolean dc_display_backend_apply_configs(DcDisplayBackend *backend,
                                          GPtrArray        *configs,
                                          char            **error_message) {
    if (backend == NULL) {
        if (error_message) *error_message = g_strdup("Backend is not initialized.");
        return FALSE;
    }

    if (backend->type == DC_BACKEND_WAYLAND)
        return dc_wlr_output_service_apply_configs(backend->impl.wlr,
                                                   configs, error_message);

    return dc_xrandr_service_apply_configs(backend->impl.x11,
                                           configs, error_message);
}

/* ============================================================
 * Display Edit (gamma / night-light / vibrance)
 * ملاحظة: هذه الميزات تعتمد على XRandR gamma ramps وليس لها مكافئ
 * في Wayland حتى الآن. في بيئة Wayland نُعيد TRUE صامتاً لتجنب
 * تعطّل واجهة المستخدم.
 * ============================================================ */

gboolean dc_display_backend_apply_display_edit(DcDisplayBackend          *backend,
                                               const DcDisplayEditConfig *config,
                                               char                     **error_message) {
    if (backend == NULL) {
        if (error_message) *error_message = g_strdup("Backend is not initialized.");
        return FALSE;
    }

    if (backend->type == DC_BACKEND_WAYLAND) {
        /* VRR: طبّق عبر Wayland إذا تغيّر */
        if (config != NULL) {
            gboolean vrr_supported = FALSE;
            if (dc_wlr_output_service_has_vrr_support(backend->impl.wlr,
                                                       &vrr_supported, NULL)
                && vrr_supported) {
                dc_wlr_output_service_apply_vrr(backend->impl.wlr,
                                                config->vrr_enabled, NULL);
            }
        }
        /* gamma/night-light/vibrance: غير مدعومة في Wayland — تُتجاهل */
        return TRUE;
    }

    return dc_xrandr_service_apply_display_edit(backend->impl.x11,
                                                config, error_message);
}

gboolean dc_display_backend_reset_display_edit(DcDisplayBackend *backend,
                                               char            **error_message) {
    if (backend == NULL) {
        if (error_message) *error_message = g_strdup("Backend is not initialized.");
        return FALSE;
    }

    if (backend->type == DC_BACKEND_WAYLAND)
        return TRUE; /* لا يوجد شيء لإعادة ضبطه */

    return dc_xrandr_service_reset_display_edit(backend->impl.x11, error_message);
}

/* ============================================================
 * VRR
 * ============================================================ */

gboolean dc_display_backend_get_vrr_support_info(DcDisplayBackend *backend,
                                                  DcVrrSupportInfo *info,
                                                  char            **error_message) {
    if (backend == NULL) {
        if (error_message) *error_message = g_strdup("Backend is not initialized.");
        return FALSE;
    }

    if (backend->type == DC_BACKEND_WAYLAND) {
        DcWlrVrrSupportInfo wlr_info = { FALSE, FALSE, 0, 0, 0 };
        gboolean ok;

        ok = dc_wlr_output_service_get_vrr_support_info(backend->impl.wlr,
                                                         &wlr_info, error_message);
        if (ok && info != NULL) {
            /* ترجمة DcWlrVrrSupportInfo → DcVrrSupportInfo */
            info->any_supported       = wlr_info.any_supported;
            info->any_writable        = wlr_info.any_writable;
            info->connected_outputs   = wlr_info.connected_outputs;
            info->supported_outputs   = wlr_info.supported_outputs;
            info->writable_outputs    = wlr_info.writable_outputs;
        }
        return ok;
    }

    return dc_xrandr_service_get_vrr_support_info(backend->impl.x11,
                                                   info, error_message);
}

gboolean dc_display_backend_has_vrr_support(DcDisplayBackend *backend,
                                             gboolean         *supported,
                                             char            **error_message) {
    if (backend == NULL) {
        if (error_message) *error_message = g_strdup("Backend is not initialized.");
        return FALSE;
    }

    if (backend->type == DC_BACKEND_WAYLAND)
        return dc_wlr_output_service_has_vrr_support(backend->impl.wlr,
                                                      supported, error_message);

    return dc_xrandr_service_has_vrr_support(backend->impl.x11,
                                              supported, error_message);
}

gboolean dc_display_backend_apply_vrr(DcDisplayBackend *backend,
                                       gboolean          enabled,
                                       char            **error_message) {
    if (backend == NULL) {
        if (error_message) *error_message = g_strdup("Backend is not initialized.");
        return FALSE;
    }

    if (backend->type == DC_BACKEND_WAYLAND)
        return dc_wlr_output_service_apply_vrr(backend->impl.wlr,
                                               enabled, error_message);

    return dc_xrandr_service_apply_vrr(backend->impl.x11, enabled, error_message);
}
