#include "services/xrandr_service.h"

#include <math.h>
#include <stdio.h>
#include <X11/Xatom.h>

typedef struct {
    const char *name;
    gboolean writable;
} DcVrrPropertyCandidate;

static const DcVrrPropertyCandidate vrr_property_candidates[] = {
    { "VRR_ENABLED", TRUE },
    { "vrr_enabled", TRUE },
    { "VariableRefresh", TRUE },
    { "Variable Refresh Rate", TRUE },
    { "adaptive_sync", TRUE },
    { "AdaptiveSync", TRUE },
    { "vrr_capable", FALSE },
    { "VRR_CAPABLE", FALSE },
    { "adaptive_sync_capable", FALSE },
    { "Adaptive Sync", FALSE },
    { "freesync_capable", FALSE }
};

static XRRModeInfo *find_mode_info(XRRScreenResources *resources, RRMode mode) {
    int i;

    for (i = 0; i < resources->nmode; i++) {
        if (resources->modes[i].id == mode) {
            return &resources->modes[i];
        }
    }

    return NULL;
}

static char *mode_to_label(const XRRModeInfo *mode) {
    double refresh_rate;

    if (mode == NULL || mode->hTotal == 0 || mode->vTotal == 0) {
        return g_strdup("Unknown");
    }

    refresh_rate = (double) mode->dotClock / ((double) mode->hTotal * (double) mode->vTotal);
    return g_strdup_printf("%lux%lu @ %.2f Hz",
                           (unsigned long) mode->width,
                           (unsigned long) mode->height,
                           refresh_rate);
}

static RRCrtc find_crtc_for_output(Display *display,
                                   XRRScreenResources *resources,
                                   XRROutputInfo *output_info,
                                   RROutput output_id) {
    int i;

    if (output_info->crtc != None) {
        return output_info->crtc;
    }

    for (i = 0; i < output_info->ncrtc; i++) {
        if (output_info->crtcs[i] != None) {
            return output_info->crtcs[i];
        }
    }

    for (i = 0; i < resources->ncrtc; i++) {
        XRRCrtcInfo *crtc_info = XRRGetCrtcInfo(display, resources, resources->crtcs[i]);
        gboolean used = FALSE;
        int j;

        if (crtc_info == NULL) {
            continue;
        }

        for (j = 0; j < crtc_info->noutput; j++) {
            if (crtc_info->outputs[j] == output_id) {
                used = TRUE;
                break;
            }
        }

        XRRFreeCrtcInfo(crtc_info);

        if (!used) {
            return resources->crtcs[i];
        }
    }

    return None;
}

static void set_error(char **error_message, const char *message) {
    if (error_message == NULL) {
        return;
    }

    g_free(*error_message);
    *error_message = g_strdup(message);
}

static double clamp_unit(double value) {
    if (value < 0.0) {
        return 0.0;
    }
    if (value > 1.0) {
        return 1.0;
    }
    return value;
}

static gboolean is_hour_in_range(int hour, int start_hour, int end_hour) {
    if (start_hour == end_hour) {
        return TRUE;
    }
    if (start_hour < end_hour) {
        return hour >= start_hour && hour < end_hour;
    }
    return hour >= start_hour || hour < end_hour;
}

static gboolean should_enable_night_light(const DcDisplayEditConfig *config) {
    GDateTime *now;
    int hour;
    gboolean enabled;

    if (!config->night_light_enabled) {
        return FALSE;
    }

    if (!config->night_light_use_schedule) {
        return TRUE;
    }

    if (g_strcmp0(config->night_light_schedule, "always") == 0) {
        return TRUE;
    }

    now = g_date_time_new_now_local();
    hour = g_date_time_get_hour(now);

    if (g_strcmp0(config->night_light_schedule, "custom") == 0) {
        enabled = is_hour_in_range(hour,
                                   config->night_light_custom_start_hour,
                                   config->night_light_custom_end_hour);
        g_date_time_unref(now);
        return enabled;
    }

    enabled = is_hour_in_range(hour, 18, 7);
    g_date_time_unref(now);
    return enabled;
}

static double compute_brightness_scale(const DcDisplayEditConfig *config) {
    GDateTime *now;
    int hour;
    double scale;

    if (!config->adaptive_brightness) {
        return 1.0;
    }

    now = g_date_time_new_now_local();
    hour = g_date_time_get_hour(now);

    if (hour >= 23 || hour < 6) {
        scale = 0.72;
    } else if (hour >= 19 || hour < 8) {
        scale = 0.84;
    } else {
        scale = 1.0;
    }

    g_date_time_unref(now);
    return scale;
}

static void kelvin_to_rgb_scale(int temperature, double *red, double *green, double *blue) {
    double temp;
    double red_value;
    double green_value;
    double blue_value;

    temp = CLAMP((double) temperature, 1000.0, 40000.0) / 100.0;

    if (temp <= 66.0) {
        red_value = 255.0;
        green_value = 99.4708025861 * log(temp) - 161.1195681661;
        if (temp <= 19.0) {
            blue_value = 0.0;
        } else {
            blue_value = 138.5177312231 * log(temp - 10.0) - 305.0447927307;
        }
    } else {
        red_value = 329.698727446 * pow(temp - 60.0, -0.1332047592);
        green_value = 288.1221695283 * pow(temp - 60.0, -0.0755148492);
        blue_value = 255.0;
    }

    *red = clamp_unit(red_value / 255.0);
    *green = clamp_unit(green_value / 255.0);
    *blue = clamp_unit(blue_value / 255.0);
}

static gboolean apply_gamma_to_output(Display *display,
                                      XRRScreenResources *resources,
                                      XRROutputInfo *output_info,
                                      double red_scale,
                                      double green_scale,
                                      double blue_scale,
                                      double gamma_value,
                                      double brightness_scale,
                                      double vibrance_amount) {
    int gamma_size;
    XRRCrtcGamma *gamma;
    int i;

    if (output_info->crtc == None) {
        return TRUE;
    }

    gamma_size = XRRGetCrtcGammaSize(display, output_info->crtc);
    if (gamma_size <= 0) {
        return FALSE;
    }

    gamma = XRRAllocGamma(gamma_size);
    if (gamma == NULL) {
        return FALSE;
    }

    for (i = 0; i < gamma_size; i++) {
        double normalized;
        double corrected;
        double red_value;
        double green_value;
        double blue_value;
        double luminance;

        normalized = gamma_size == 1 ? 1.0 : (double) i / (double) (gamma_size - 1);
        corrected = pow(normalized, 1.0 / MAX(gamma_value, 0.1));
        red_value = corrected * red_scale * brightness_scale;
        green_value = corrected * green_scale * brightness_scale;
        blue_value = corrected * blue_scale * brightness_scale;

        if (vibrance_amount > 0.0) {
            luminance = (red_value * 0.2126) + (green_value * 0.7152) + (blue_value * 0.0722);
            red_value = luminance + ((red_value - luminance) * vibrance_amount);
            green_value = luminance + ((green_value - luminance) * vibrance_amount);
            blue_value = luminance + ((blue_value - luminance) * vibrance_amount);
        }

        gamma->red[i] = (gushort) (CLAMP(red_value, 0.0, 1.0) * 65535.0);
        gamma->green[i] = (gushort) (CLAMP(green_value, 0.0, 1.0) * 65535.0);
        gamma->blue[i] = (gushort) (CLAMP(blue_value, 0.0, 1.0) * 65535.0);
    }

    XRRSetCrtcGamma(display, output_info->crtc, gamma);
    XRRFreeGamma(gamma);
    (void) resources;
    return TRUE;
}

static gboolean property_name_matches(const char *property_name,
                                      const char *candidate_name) {
    return g_ascii_strcasecmp(property_name, candidate_name) == 0;
}

static gboolean is_vrr_property_name(const char *property_name,
                                     gboolean *writable_candidate) {
    guint i;

    if (writable_candidate != NULL) {
        *writable_candidate = FALSE;
    }

    if (property_name == NULL) {
        return FALSE;
    }

    for (i = 0; i < G_N_ELEMENTS(vrr_property_candidates); i++) {
        if (property_name_matches(property_name, vrr_property_candidates[i].name)) {
            if (writable_candidate != NULL) {
                *writable_candidate = vrr_property_candidates[i].writable;
            }
            return TRUE;
        }
    }

    return FALSE;
}

static gboolean output_has_vrr_property(Display *display,
                                        RROutput output,
                                        gboolean writable_only,
                                        Atom *property_atom) {
    Atom *properties;
    int property_count;
    gboolean found;
    int i;

    properties = XRRListOutputProperties(display, output, &property_count);
    if (properties == NULL) {
        return FALSE;
    }

    found = FALSE;
    for (i = 0; i < property_count; i++) {
        char *property_name;
        gboolean writable_candidate;

        property_name = XGetAtomName(display, properties[i]);
        if (property_name == NULL) {
            continue;
        }

        if (is_vrr_property_name(property_name, &writable_candidate) &&
            (!writable_only || writable_candidate)) {
            found = TRUE;
            if (property_atom != NULL) {
                *property_atom = properties[i];
            }
            XFree(property_name);
            break;
        }

        XFree(property_name);
    }

    XFree(properties);
    return found;
}

static gboolean set_output_boolean_property(Display *display,
                                            RROutput output,
                                            Atom property,
                                            long value) {
    XRRPropertyInfo *property_info;
    long property_value;

    property_info = XRRQueryOutputProperty(display, output, property);
    if (property_info == NULL) {
        return FALSE;
    }

    if (property_info->immutable) {
        XFree(property_info);
        return FALSE;
    }

    property_value = value;
    XRRChangeOutputProperty(display,
                            output,
                            property,
                            XA_INTEGER,
                            32,
                            PropModeReplace,
                            (unsigned char *) &property_value,
                            1);
    XFree(property_info);
    return TRUE;
}

DcXrandrService *dc_xrandr_service_new(char **error_message) {
    DcXrandrService *service;
    int event_base;
    int error_base;
    int xrandr_major;
    int xrandr_minor;

    service = g_new0(DcXrandrService, 1);
    service->display = XOpenDisplay(NULL);

    if (service->display == NULL) {
        set_error(error_message, "Could not open X display.");
        g_free(service);
        return NULL;
    }

    if (!XRRQueryExtension(service->display, &event_base, &error_base)) {
        set_error(error_message, "XRandR extension is not available on this X server.");
        XCloseDisplay(service->display);
        g_free(service);
        return NULL;
    }

    xrandr_major = 1;
    xrandr_minor = 5;
    if (!XRRQueryVersion(service->display, &xrandr_major, &xrandr_minor)) {
        set_error(error_message, "Failed to query XRandR version.");
        XCloseDisplay(service->display);
        g_free(service);
        return NULL;
    }

    service->root = DefaultRootWindow(service->display);
    return service;
}

void dc_xrandr_service_free(DcXrandrService *service) {
    if (service == NULL) {
        return;
    }

    if (service->display != NULL) {
        XCloseDisplay(service->display);
    }

    g_free(service);
}

gboolean dc_xrandr_service_load_outputs(DcXrandrService *service,
                                        GPtrArray **outputs,
                                        char **error_message) {
    XRRScreenResources *resources;
    RROutput primary_output;
    GPtrArray *loaded_outputs;
    int i;

    if (service == NULL || outputs == NULL) {
        set_error(error_message, "Display service is not initialized.");
        return FALSE;
    }

    resources = XRRGetScreenResourcesCurrent(service->display, service->root);
    if (resources == NULL) {
        set_error(error_message, "Failed to read XRandR screen resources.");
        return FALSE;
    }

    loaded_outputs = g_ptr_array_new_with_free_func((GDestroyNotify) dc_display_output_free);
    primary_output = XRRGetOutputPrimary(service->display, service->root);

    for (i = 0; i < resources->noutput; i++) {
        XRROutputInfo *output_info = XRRGetOutputInfo(service->display, resources, resources->outputs[i]);
        DcDisplayOutput *output;
        XRRCrtcInfo *crtc_info = NULL;
        int j;

        if (output_info == NULL) {
            continue;
        }

        if (output_info->connection != RR_Connected) {
            XRRFreeOutputInfo(output_info);
            continue;
        }

        output = dc_display_output_new();
        output->output_id = resources->outputs[i];
        output->name = g_strdup(output_info->name);
        output->connected = TRUE;
        output->enabled = output_info->crtc != None;
        output->primary = (primary_output == output->output_id);
        output->crtc_id = output_info->crtc;
        output->current_rotation = RR_Rotate_0;

        if (output_info->crtc != None) {
            crtc_info = XRRGetCrtcInfo(service->display, resources, output_info->crtc);
            if (crtc_info != NULL) {
                output->current_mode = crtc_info->mode;
                output->current_rotation = crtc_info->rotation &
                                           (RR_Rotate_0 | RR_Rotate_90 | RR_Rotate_180 | RR_Rotate_270);
                output->x = crtc_info->x;
                output->y = crtc_info->y;
            }
        }

        for (j = 0; j < output_info->nmode; j++) {
            XRRModeInfo *mode_info = find_mode_info(resources, output_info->modes[j]);
            char *label;
            double refresh_rate;

            if (mode_info == NULL || mode_info->hTotal == 0 || mode_info->vTotal == 0) {
                continue;
            }

            refresh_rate = (double) mode_info->dotClock /
                           ((double) mode_info->hTotal * (double) mode_info->vTotal);
            label = mode_to_label(mode_info);
            g_ptr_array_add(output->modes,
                            dc_display_mode_new(mode_info->id,
                                                (int) mode_info->width,
                                                (int) mode_info->height,
                                                refresh_rate,
                                                label));
            g_free(label);
        }

        if (crtc_info != NULL) {
            XRRFreeCrtcInfo(crtc_info);
        }

        XRRFreeOutputInfo(output_info);
        g_ptr_array_add(loaded_outputs, output);
    }

    XRRFreeScreenResources(resources);
    *outputs = loaded_outputs;
    return TRUE;
}

gboolean dc_xrandr_service_apply_configs(DcXrandrService *service,
                                         GPtrArray *configs,
                                         char **error_message) {
    XRRScreenResources *resources;
    GString *errors;
    gboolean success = TRUE;
    RROutput primary_output = None;
    guint i;

    if (service == NULL) {
        set_error(error_message, "Display service is not initialized.");
        return FALSE;
    }

    resources = XRRGetScreenResources(service->display, service->root);
    if (resources == NULL) {
        set_error(error_message, "Failed to fetch XRandR resources for applying changes.");
        return FALSE;
    }

    errors = g_string_new("");

    for (i = 0; i < configs->len; i++) {
        DcDisplayConfig *config = g_ptr_array_index(configs, i);
        XRROutputInfo *output_info = XRRGetOutputInfo(service->display, resources, config->output_id);
        RRCrtc crtc;

        if (output_info == NULL) {
            g_string_append_printf(errors, "Output %lu: failed to read output info.\n",
                                   (unsigned long) config->output_id);
            success = FALSE;
            continue;
        }

        crtc = find_crtc_for_output(service->display, resources, output_info, config->output_id);

        if (!config->enabled) {
            if (output_info->crtc != None) {
                Status disable_status = XRRSetCrtcConfig(service->display,
                                                         resources,
                                                         output_info->crtc,
                                                         CurrentTime,
                                                         0,
                                                         0,
                                                         None,
                                                         RR_Rotate_0,
                                                         NULL,
                                                         0);
                if (disable_status != Success) {
                    g_string_append_printf(errors, "%s: failed to disable output.\n", output_info->name);
                    success = FALSE;
                }
            }

            XRRFreeOutputInfo(output_info);
            continue;
        }

        if (crtc == None) {
            g_string_append_printf(errors, "%s: no available CRTC was found.\n", output_info->name);
            XRRFreeOutputInfo(output_info);
            success = FALSE;
            continue;
        }

        {
            RROutput output_ids[1];
            Status apply_status;

            output_ids[0] = config->output_id;
            apply_status = XRRSetCrtcConfig(service->display,
                                            resources,
                                            crtc,
                                            CurrentTime,
                                            config->x,
                                            config->y,
                                            config->mode,
                                            config->rotation,
                                            output_ids,
                                            1);
            if (apply_status != Success) {
                g_string_append_printf(errors, "%s: failed to apply configuration.\n", output_info->name);
                success = FALSE;
            }
        }

        if (config->enabled && config->primary) {
            primary_output = config->output_id;
        }

        XRRFreeOutputInfo(output_info);
    }

    if (primary_output != None) {
        XRRSetOutputPrimary(service->display, service->root, primary_output);
    }

    XSync(service->display, False);
    XRRFreeScreenResources(resources);

    if (!success) {
        set_error(error_message, errors->str);
    }

    g_string_free(errors, TRUE);
    return success;
}

gboolean dc_xrandr_service_apply_display_edit(DcXrandrService *service,
                                              const DcDisplayEditConfig *config,
                                              char **error_message) {
    XRRScreenResources *resources;
    GString *errors;
    gboolean success;
    gboolean vrr_supported;
    double red_scale;
    double green_scale;
    double blue_scale;
    double brightness_scale;
    double vibrance_amount;
    int i;

    if (service == NULL || config == NULL) {
        set_error(error_message, "Display service or display edit config is missing.");
        return FALSE;
    }

    vrr_supported = FALSE;
    if (dc_xrandr_service_has_vrr_support(service, &vrr_supported, NULL) && vrr_supported) {
        dc_xrandr_service_apply_vrr(service, config->vrr_enabled, NULL);
    }

    resources = XRRGetScreenResourcesCurrent(service->display, service->root);
    if (resources == NULL) {
        set_error(error_message, "Failed to fetch screen resources for display edit changes.");
        return FALSE;
    }

    red_scale = 1.0;
    green_scale = 1.0;
    blue_scale = 1.0;
    brightness_scale = compute_brightness_scale(config);
    vibrance_amount = 1.0 + ((double) config->vibrance / 100.0);
    if (should_enable_night_light(config)) {
        kelvin_to_rgb_scale(config->night_light_temperature, &red_scale, &green_scale, &blue_scale);
    }

    success = TRUE;
    errors = g_string_new("");

    for (i = 0; i < resources->noutput; i++) {
        XRROutputInfo *output_info;

        output_info = XRRGetOutputInfo(service->display, resources, resources->outputs[i]);
        if (output_info == NULL) {
            success = FALSE;
            g_string_append(errors, "Failed to read output info for display edit changes.\n");
            continue;
        }

        if (output_info->connection == RR_Connected && output_info->crtc != None) {
            if (!apply_gamma_to_output(service->display,
                                       resources,
                                       output_info,
                                       red_scale,
                                       green_scale,
                                       blue_scale,
                                       config->gamma,
                                       brightness_scale,
                                       vibrance_amount)) {
                success = FALSE;
                g_string_append_printf(errors, "%s: failed to apply display edit adjustments.\n", output_info->name);
            }
        }

        XRRFreeOutputInfo(output_info);
    }

    XSync(service->display, False);
    XRRFreeScreenResources(resources);

    if (!success) {
        set_error(error_message, errors->str);
    }

    g_string_free(errors, TRUE);
    return success;
}

gboolean dc_xrandr_service_reset_display_edit(DcXrandrService *service,
                                              char **error_message) {
    DcDisplayEditConfig *config;
    gboolean success;

    config = dc_display_edit_config_new();
    config->vibrance = 0;
    success = dc_xrandr_service_apply_display_edit(service, config, error_message);
    dc_display_edit_config_free(config);
    return success;
}

gboolean dc_xrandr_service_has_vrr_support(DcXrandrService *service,
                                           gboolean *supported,
                                           char **error_message) {
    DcVrrSupportInfo info;

    if (!dc_xrandr_service_get_vrr_support_info(service, &info, error_message)) {
        return FALSE;
    }

    if (supported != NULL) {
        *supported = info.any_supported;
    }
    return TRUE;
}

gboolean dc_xrandr_service_get_vrr_support_info(DcXrandrService *service,
                                                DcVrrSupportInfo *info,
                                                char **error_message) {
    XRRScreenResources *resources;
    DcVrrSupportInfo local_info;
    int i;

    if (service == NULL) {
        set_error(error_message, "Display service is not initialized.");
        return FALSE;
    }

    local_info.any_supported = FALSE;
    local_info.any_writable = FALSE;
    local_info.connected_outputs = 0;
    local_info.supported_outputs = 0;
    local_info.writable_outputs = 0;

    resources = XRRGetScreenResourcesCurrent(service->display, service->root);
    if (resources == NULL) {
        set_error(error_message, "Failed to query screen resources for VRR support.");
        return FALSE;
    }

    for (i = 0; i < resources->noutput; i++) {
        XRROutputInfo *output_info;
        gboolean has_supported_property;
        gboolean has_writable_property;

        output_info = XRRGetOutputInfo(service->display, resources, resources->outputs[i]);
        if (output_info == NULL) {
            continue;
        }

        if (output_info->connection == RR_Connected) {
            local_info.connected_outputs++;
            has_supported_property = output_has_vrr_property(service->display, resources->outputs[i], FALSE, NULL);
            has_writable_property = output_has_vrr_property(service->display, resources->outputs[i], TRUE, NULL);

            if (has_supported_property) {
                local_info.supported_outputs++;
                local_info.any_supported = TRUE;
            }

            if (has_writable_property) {
                local_info.writable_outputs++;
                local_info.any_writable = TRUE;
            }
        }

        XRRFreeOutputInfo(output_info);
    }

    XRRFreeScreenResources(resources);
    if (info != NULL) {
        *info = local_info;
    }
    return TRUE;
}

gboolean dc_xrandr_service_apply_vrr(DcXrandrService *service,
                                     gboolean enabled,
                                     char **error_message) {
    XRRScreenResources *resources;
    gboolean changed_any;
    int i;

    if (service == NULL) {
        set_error(error_message, "Display service is not initialized.");
        return FALSE;
    }

    resources = XRRGetScreenResourcesCurrent(service->display, service->root);
    if (resources == NULL) {
        set_error(error_message, "Failed to query screen resources for VRR changes.");
        return FALSE;
    }

    changed_any = FALSE;
    for (i = 0; i < resources->noutput; i++) {
        XRROutputInfo *output_info;
        Atom property;

        output_info = XRRGetOutputInfo(service->display, resources, resources->outputs[i]);
        if (output_info == NULL) {
            continue;
        }

        if (output_info->connection == RR_Connected &&
            output_has_vrr_property(service->display, resources->outputs[i], TRUE, &property) &&
            set_output_boolean_property(service->display, resources->outputs[i], property, enabled ? 1L : 0L)) {
            changed_any = TRUE;
        }

        XRRFreeOutputInfo(output_info);
    }

    XSync(service->display, False);
    XRRFreeScreenResources(resources);

    if (!changed_any) {
        set_error(error_message, "No writable VRR property was found on connected outputs.");
        return FALSE;
    }

    return TRUE;
}
