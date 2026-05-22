/*
 * wlr_output_service.c
 *
 * خدمة إدارة الشاشات عبر بروتوكول zwlr_output_management_unstable_v1.
 */

#include "services/wlr_output_service.h"
#include "services/wlr-output-management-unstable-v1-client-protocol.h"

#include <stdio.h>
#include <string.h>

/* ============================================================
 * أدوات مساعدة
 * ============================================================ */

static void set_error(char **err, const char *msg) {
    if (!err) return;
    g_free(*err);
    *err = g_strdup(msg);
}

/* تحويل wl_output_transform إلى Rotation (XRandR) لاستخدامه في DcDisplayOutput */
static Rotation wl_transform_to_rotation(int t) {
    switch (t) {
        case 1:  return RR_Rotate_90;
        case 2:  return RR_Rotate_180;
        case 3:  return RR_Rotate_270;
        default: return RR_Rotate_0;
    }
}

/* تحويل Rotation (XRandR) إلى wl_output_transform */
static int rotation_to_wl_transform(Rotation r) {
    switch (r) {
        case RR_Rotate_90:  return 1;
        case RR_Rotate_180: return 2;
        case RR_Rotate_270: return 3;
        default:            return 0;
    }
}

/* ============================================================
 * Listeners لـ zwlr_output_mode_v1
 * ============================================================ */

static void mode_size(void *data, struct zwlr_output_mode_v1 *wl_mode,
                      int32_t width, int32_t height) {
    DcWlrModeInfo *m = data;
    (void) wl_mode;
    m->width  = (int) width;
    m->height = (int) height;
}

static void mode_refresh(void *data, struct zwlr_output_mode_v1 *wl_mode,
                         int32_t refresh) {
    DcWlrModeInfo *m = data;
    (void) wl_mode;
    m->refresh_mhz = (int) refresh;
}

static void mode_preferred(void *data, struct zwlr_output_mode_v1 *wl_mode) {
    DcWlrModeInfo *m = data;
    (void) wl_mode;
    m->preferred = TRUE;
}

static void mode_finished(void *data, struct zwlr_output_mode_v1 *wl_mode) {
    (void) data;
    zwlr_output_mode_v1_release(wl_mode);
}

static const struct zwlr_output_mode_v1_listener mode_listener = {
    .size      = mode_size,
    .refresh   = mode_refresh,
    .preferred = mode_preferred,
    .finished  = mode_finished,
};

/* ============================================================
 * Listeners لـ zwlr_output_head_v1
 * ============================================================ */

static void head_name(void *data, struct zwlr_output_head_v1 *wl_head,
                      const char *name) {
    DcWlrHeadInfo *h = data;
    (void) wl_head;
    g_free(h->name);
    h->name = g_strdup(name);
}

static void head_description(void *data, struct zwlr_output_head_v1 *wl_head,
                             const char *description) {
    DcWlrHeadInfo *h = data;
    (void) wl_head;
    g_free(h->description);
    h->description = g_strdup(description);
}

static void head_physical_size(void *data, struct zwlr_output_head_v1 *wl_head,
                               int32_t width, int32_t height) {
    (void) data; (void) wl_head; (void) width; (void) height;
}

static void head_mode(void *data, struct zwlr_output_head_v1 *wl_head,
                      struct zwlr_output_mode_v1 *wl_mode) {
    DcWlrHeadInfo *h = data;
    DcWlrModeInfo *m;
    (void) wl_head;

    m = g_new0(DcWlrModeInfo, 1);
    m->wl_mode = wl_mode;
    zwlr_output_mode_v1_add_listener(wl_mode, &mode_listener, m);

    /* أضفه في نهاية القائمة */
    if (h->modes == NULL) {
        h->modes = m;
    } else {
        DcWlrModeInfo *cur = h->modes;
        while (cur->next) cur = cur->next;
        cur->next = m;
    }
}

static void head_enabled(void *data, struct zwlr_output_head_v1 *wl_head,
                         int32_t enabled) {
    DcWlrHeadInfo *h = data;
    (void) wl_head;
    h->enabled = (enabled != 0);
}

static void head_current_mode(void *data, struct zwlr_output_head_v1 *wl_head,
                              struct zwlr_output_mode_v1 *wl_mode) {
    DcWlrHeadInfo *h = data;
    DcWlrModeInfo *cur;
    (void) wl_head;

    for (cur = h->modes; cur != NULL; cur = cur->next) {
        if (cur->wl_mode == wl_mode) {
            h->current_mode = cur;
            return;
        }
    }
}

static void head_position(void *data, struct zwlr_output_head_v1 *wl_head,
                          int32_t x, int32_t y) {
    DcWlrHeadInfo *h = data;
    (void) wl_head;
    h->pos_x = (int) x;
    h->pos_y = (int) y;
}

static void head_transform(void *data, struct zwlr_output_head_v1 *wl_head,
                           int32_t transform) {
    DcWlrHeadInfo *h = data;
    (void) wl_head;
    h->transform = (int) transform;
}

static void head_scale(void *data, struct zwlr_output_head_v1 *wl_head,
                       wl_fixed_t scale) {
    DcWlrHeadInfo *h = data;
    (void) wl_head;
    h->scale = wl_fixed_to_double(scale);
}

static void head_finished(void *data, struct zwlr_output_head_v1 *wl_head) {
    DcWlrHeadInfo *h = data;
    (void) wl_head;
    /* نُبقي البنية لكن نضع اسمها NULL لنتجاهلها عند التحميل */
    g_free(h->name);
    h->name = NULL;
}

static void head_make(void *data, struct zwlr_output_head_v1 *wl_head,
                      const char *make) {
    (void) data; (void) wl_head; (void) make;
}

static void head_model(void *data, struct zwlr_output_head_v1 *wl_head,
                       const char *model) {
    (void) data; (void) wl_head; (void) model;
}

static void head_serial_number(void *data, struct zwlr_output_head_v1 *wl_head,
                               const char *serial_number) {
    (void) data; (void) wl_head; (void) serial_number;
}

static void head_adaptive_sync(void *data, struct zwlr_output_head_v1 *wl_head,
                               uint32_t state) {
    DcWlrHeadInfo *h = data;
    (void) wl_head;
    h->adaptive_sync = (state != 0);
}

static const struct zwlr_output_head_v1_listener head_listener = {
    .name          = head_name,
    .description   = head_description,
    .physical_size = head_physical_size,
    .mode          = head_mode,
    .enabled       = head_enabled,
    .current_mode  = head_current_mode,
    .position      = head_position,
    .transform     = head_transform,
    .scale         = head_scale,
    .finished      = head_finished,
    .make          = head_make,
    .model         = head_model,
    .serial_number = head_serial_number,
    .adaptive_sync = head_adaptive_sync,
};

/* ============================================================
 * Listeners لـ zwlr_output_manager_v1
 * ============================================================ */

static void manager_head(void *data, struct zwlr_output_manager_v1 *manager,
                         struct zwlr_output_head_v1 *wl_head) {
    DcWlrOutputService *svc = data;
    DcWlrHeadInfo *h;
    (void) manager;

    h = g_new0(DcWlrHeadInfo, 1);
    h->wl_head = wl_head;
    h->scale   = 1.0;
    zwlr_output_head_v1_add_listener(wl_head, &head_listener, h);

    /* أضفه في نهاية القائمة */
    if (svc->heads == NULL) {
        svc->heads = h;
    } else {
        DcWlrHeadInfo *cur = svc->heads;
        while (cur->next) cur = cur->next;
        cur->next = h;
    }
}

static void manager_done(void *data, struct zwlr_output_manager_v1 *manager,
                         uint32_t serial) {
    DcWlrOutputService *svc = data;
    (void) manager;
    svc->last_serial  = serial;
    svc->initialized  = TRUE;
}

static void manager_finished(void *data, struct zwlr_output_manager_v1 *manager) {
    DcWlrOutputService *svc = data;
    (void) manager;
    svc->wlr_manager = NULL;
}

static const struct zwlr_output_manager_v1_listener manager_listener = {
    .head     = manager_head,
    .done     = manager_done,
    .finished = manager_finished,
};

/* ============================================================
 * Registry
 * ============================================================ */

static void registry_global(void *data, struct wl_registry *registry,
                             uint32_t name, const char *interface,
                             uint32_t version) {
    DcWlrOutputService *svc = data;

    if (strcmp(interface, zwlr_output_manager_v1_interface.name) == 0) {
        uint32_t bind_ver = (version < 4) ? version : 4;
        svc->wlr_manager = wl_registry_bind(registry, name,
                                            &zwlr_output_manager_v1_interface,
                                            bind_ver);
        zwlr_output_manager_v1_add_listener(svc->wlr_manager, &manager_listener, svc);
    }
}

static void registry_global_remove(void *data, struct wl_registry *registry,
                                   uint32_t name) {
    (void) data; (void) registry; (void) name;
}

static const struct wl_registry_listener registry_listener = {
    .global        = registry_global,
    .global_remove = registry_global_remove,
};

/* ============================================================
 * تحرير الذاكرة الداخلية
 * ============================================================ */

static void free_mode_list(DcWlrModeInfo *m) {
    while (m) {
        DcWlrModeInfo *next = m->next;
        /* wl_mode تم تحريره في finished listener */
        g_free(m);
        m = next;
    }
}

static void free_head_list(DcWlrHeadInfo *h) {
    while (h) {
        DcWlrHeadInfo *next = h->next;
        g_free(h->name);
        g_free(h->description);
        free_mode_list(h->modes);
        if (h->wl_head)
            zwlr_output_head_v1_release(h->wl_head);
        g_free(h);
        h = next;
    }
}

/* ============================================================
 * دوال دورة الحياة العامة
 * ============================================================ */

DcWlrOutputService *dc_wlr_output_service_new(char **error_message) {
    DcWlrOutputService *svc;
    const char *wayland_display;

    wayland_display = g_getenv("WAYLAND_DISPLAY");
    if (wayland_display == NULL || *wayland_display == '\0') {
        set_error(error_message, "WAYLAND_DISPLAY is not set.");
        return NULL;
    }

    svc = g_new0(DcWlrOutputService, 1);
    svc->wl_display = wl_display_connect(NULL);
    if (svc->wl_display == NULL) {
        set_error(error_message, "Could not connect to Wayland display.");
        g_free(svc);
        return NULL;
    }

    svc->wl_registry = wl_display_get_registry(svc->wl_display);
    wl_registry_add_listener(svc->wl_registry, &registry_listener, svc);

    /* roundtrip أول: يُعلن عن الـ globals ويجلب الرؤوس والأوضاع */
    wl_display_roundtrip(svc->wl_display);

    if (svc->wlr_manager == NULL) {
        set_error(error_message,
                  "zwlr_output_manager_v1 is not supported by this Wayland compositor.");
        wl_registry_destroy(svc->wl_registry);
        wl_display_disconnect(svc->wl_display);
        g_free(svc);
        return NULL;
    }

    /* roundtrip ثانٍ: يُكمل استقبال بيانات الرؤوس والأوضاع */
    wl_display_roundtrip(svc->wl_display);

    return svc;
}

void dc_wlr_output_service_free(DcWlrOutputService *svc) {
    if (svc == NULL) return;

    free_head_list(svc->heads);

    if (svc->wlr_manager)
        zwlr_output_manager_v1_stop(svc->wlr_manager);
    if (svc->wl_registry)
        wl_registry_destroy(svc->wl_registry);
    if (svc->wl_display)
        wl_display_disconnect(svc->wl_display);

    g_free(svc);
}

/* ============================================================
 * تحميل الشاشات
 * ============================================================ */

gboolean dc_wlr_output_service_load_outputs(DcWlrOutputService *svc,
                                            GPtrArray         **outputs,
                                            char              **error_message) {
    GPtrArray *arr;
    DcWlrHeadInfo *h;
    guint mode_index;

    if (svc == NULL || outputs == NULL) {
        set_error(error_message, "Wayland display service is not initialized.");
        return FALSE;
    }

    /* تحديث الحالة */
    wl_display_roundtrip(svc->wl_display);

    arr = g_ptr_array_new_with_free_func((GDestroyNotify) dc_display_output_free);

    for (h = svc->heads; h != NULL; h = h->next) {
        DcDisplayOutput *out;
        DcWlrModeInfo   *m;

        /* الرؤوس المنتهية تُتجاهل */
        if (h->name == NULL)
            continue;

        out = dc_display_output_new();
        out->output_id       = (RROutput)(guintptr) h->wl_head;
        out->name            = g_strdup(h->name);
        out->connected       = TRUE;
        out->enabled         = h->enabled;
        out->primary         = FALSE; /* Wayland لا يملك مفهوم primary */
        out->crtc_id         = (RRCrtc) 0;
        out->x               = h->pos_x;
        out->y               = h->pos_y;
        out->current_rotation = wl_transform_to_rotation(h->transform);

        /* الأوضاع */
        mode_index = 0;
        for (m = h->modes; m != NULL; m = m->next) {
            double refresh = (m->refresh_mhz > 0)
                             ? (double) m->refresh_mhz / 1000.0
                             : 0.0;
            char *label = g_strdup_printf("%dx%d @ %.2f Hz",
                                          m->width, m->height, refresh);
            RRMode mode_id = (RRMode)(guintptr) m->wl_mode;
            g_ptr_array_add(out->modes,
                            dc_display_mode_new(mode_id, m->width, m->height,
                                                refresh, label));
            g_free(label);

            if (h->current_mode == m)
                out->current_mode = mode_id;

            mode_index++;
        }

        g_ptr_array_add(arr, out);
    }

    *outputs = arr;
    return TRUE;
}

/* ============================================================
 * Listener للتكوين — نتائج apply/test
 * ============================================================ */

typedef struct {
    gboolean done;
    gboolean succeeded;
} DcWlrApplyResult;

static void config_succeeded(void *data,
                             struct zwlr_output_configuration_v1 *cfg) {
    DcWlrApplyResult *r = data;
    (void) cfg;
    r->done      = TRUE;
    r->succeeded = TRUE;
}

static void config_failed(void *data,
                          struct zwlr_output_configuration_v1 *cfg) {
    DcWlrApplyResult *r = data;
    (void) cfg;
    r->done      = TRUE;
    r->succeeded = FALSE;
}

static void config_cancelled(void *data,
                             struct zwlr_output_configuration_v1 *cfg) {
    DcWlrApplyResult *r = data;
    (void) cfg;
    r->done      = TRUE;
    r->succeeded = FALSE;
}

static const struct zwlr_output_configuration_v1_listener config_listener = {
    .succeeded = config_succeeded,
    .failed    = config_failed,
    .cancelled = config_cancelled,
};

/* ============================================================
 * تطبيق الإعدادات
 * ============================================================ */

gboolean dc_wlr_output_service_apply_configs(DcWlrOutputService *svc,
                                             GPtrArray          *configs,
                                             char              **error_message) {
    struct zwlr_output_configuration_v1 *wlr_config;
    DcWlrApplyResult result = { FALSE, FALSE };
    guint i;

    if (svc == NULL || svc->wlr_manager == NULL) {
        set_error(error_message, "Wayland output manager is not available.");
        return FALSE;
    }

    /* تحديث الحالة قبل البدء */
    wl_display_roundtrip(svc->wl_display);

    wlr_config = zwlr_output_manager_v1_create_configuration(svc->wlr_manager,
                                                              svc->last_serial);
    zwlr_output_configuration_v1_add_listener(wlr_config, &config_listener, &result);

    for (i = 0; i < configs->len; i++) {
        DcDisplayConfig *cfg  = g_ptr_array_index(configs, i);
        DcWlrHeadInfo   *h;

        /* البحث عن الرأس المطابق عبر output_id */
        for (h = svc->heads; h != NULL; h = h->next) {
            if ((RROutput)(guintptr) h->wl_head == cfg->output_id)
                break;
        }

        if (h == NULL || h->name == NULL)
            continue;

        if (!cfg->enabled) {
            zwlr_output_configuration_v1_disable_head(wlr_config, h->wl_head);
        } else {
            struct zwlr_output_configuration_head_v1 *cfg_head;
            DcWlrModeInfo *m;

            cfg_head = zwlr_output_configuration_v1_enable_head(wlr_config,
                                                                 h->wl_head);

            /* البحث عن الوضع المطلوب */
            for (m = h->modes; m != NULL; m = m->next) {
                if ((RRMode)(guintptr) m->wl_mode == cfg->mode)
                    break;
            }
            if (m != NULL)
                zwlr_output_configuration_head_v1_set_mode(cfg_head, m->wl_mode);

            zwlr_output_configuration_head_v1_set_position(cfg_head,
                                                            cfg->x, cfg->y);
            zwlr_output_configuration_head_v1_set_transform(
                cfg_head, rotation_to_wl_transform(cfg->rotation));
        }
    }

    zwlr_output_configuration_v1_apply(wlr_config);
    wl_display_flush(svc->wl_display);

    /* انتظر حتى يصل رد الملحق */
    while (!result.done)
        wl_display_roundtrip(svc->wl_display);

    zwlr_output_configuration_v1_destroy(wlr_config);

    if (!result.succeeded) {
        set_error(error_message,
                  "Wayland compositor rejected the output configuration.");
        return FALSE;
    }

    /* roundtrip إضافي لاستيعاب الأحداث الجديدة */
    wl_display_roundtrip(svc->wl_display);
    return TRUE;
}

/* ============================================================
 * VRR / Adaptive Sync
 * ============================================================ */

gboolean dc_wlr_output_service_get_vrr_support_info(DcWlrOutputService  *svc,
                                                    DcWlrVrrSupportInfo *info,
                                                    char               **error_message) {
    DcWlrHeadInfo *h;
    DcWlrVrrSupportInfo local = { FALSE, FALSE, 0, 0, 0 };
    int bound_version;

    if (svc == NULL) {
        set_error(error_message, "Wayland display service is not initialized.");
        return FALSE;
    }

    /* تحقق من أن النسخة المرتبطة تدعم adaptive_sync (version >= 4) */
    bound_version = zwlr_output_manager_v1_get_version(svc->wlr_manager);

    for (h = svc->heads; h != NULL; h = h->next) {
        if (h->name == NULL) continue;
        local.connected_outputs++;

        if (bound_version >= 4) {
            local.supported_outputs++;
            local.writable_outputs++;
            local.any_supported = TRUE;
            local.any_writable  = TRUE;
        }
    }

    if (info != NULL)
        *info = local;

    return TRUE;
}

gboolean dc_wlr_output_service_has_vrr_support(DcWlrOutputService *svc,
                                               gboolean           *supported,
                                               char              **error_message) {
    DcWlrVrrSupportInfo info;

    if (!dc_wlr_output_service_get_vrr_support_info(svc, &info, error_message))
        return FALSE;

    if (supported != NULL)
        *supported = info.any_supported;

    return TRUE;
}

gboolean dc_wlr_output_service_apply_vrr(DcWlrOutputService *svc,
                                         gboolean            enabled,
                                         char              **error_message) {
    struct zwlr_output_configuration_v1 *wlr_config;
    DcWlrApplyResult result = { FALSE, FALSE };
    DcWlrHeadInfo *h;
    int bound_version;

    if (svc == NULL || svc->wlr_manager == NULL) {
        set_error(error_message, "Wayland output manager is not available.");
        return FALSE;
    }

    bound_version = zwlr_output_manager_v1_get_version(svc->wlr_manager);
    if (bound_version < 4) {
        set_error(error_message,
                  "Adaptive sync requires zwlr_output_manager_v1 version 4+.");
        return FALSE;
    }

    wl_display_roundtrip(svc->wl_display);

    wlr_config = zwlr_output_manager_v1_create_configuration(svc->wlr_manager,
                                                              svc->last_serial);
    zwlr_output_configuration_v1_add_listener(wlr_config, &config_listener, &result);

    for (h = svc->heads; h != NULL; h = h->next) {
        if (h->name == NULL || !h->enabled) continue;

        {
            struct zwlr_output_configuration_head_v1 *cfg_head;
            uint32_t adaptive_sync_state;

            cfg_head = zwlr_output_configuration_v1_enable_head(wlr_config,
                                                                 h->wl_head);

            /* إعادة تعيين الوضع الحالي */
            if (h->current_mode != NULL)
                zwlr_output_configuration_head_v1_set_mode(cfg_head,
                                                            h->current_mode->wl_mode);

            zwlr_output_configuration_head_v1_set_position(cfg_head,
                                                            h->pos_x, h->pos_y);
            zwlr_output_configuration_head_v1_set_transform(
                cfg_head, rotation_to_wl_transform(
                    wl_transform_to_rotation(h->transform)));

            adaptive_sync_state = enabled
                ? ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_ENABLED
                : ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_DISABLED;
            zwlr_output_configuration_head_v1_set_adaptive_sync(cfg_head,
                                                                 adaptive_sync_state);
        }
    }

    zwlr_output_configuration_v1_apply(wlr_config);
    wl_display_flush(svc->wl_display);

    while (!result.done)
        wl_display_roundtrip(svc->wl_display);

    zwlr_output_configuration_v1_destroy(wlr_config);

    if (!result.succeeded) {
        set_error(error_message,
                  "Failed to apply adaptive sync state via Wayland.");
        return FALSE;
    }

    wl_display_roundtrip(svc->wl_display);
    return TRUE;
}
