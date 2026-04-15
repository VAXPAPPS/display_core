#include "app/app_controller.h"

#include "app_controller_internal.h"
#include "services/audio_service.h"

static void dc_app_cairo_rounded_rectangle(cairo_t *cr,
                                           double x,
                                           double y,
                                           double width,
                                           double height,
                                           double radius) {
    const double degrees = G_PI / 180.0;

    cairo_new_sub_path(cr);
    cairo_arc(cr, x + width - radius, y + radius, radius, -90 * degrees, 0 * degrees);
    cairo_arc(cr, x + width - radius, y + height - radius, radius, 0 * degrees, 90 * degrees);
    cairo_arc(cr, x + radius, y + height - radius, radius, 90 * degrees, 180 * degrees);
    cairo_arc(cr, x + radius, y + radius, radius, 180 * degrees, 270 * degrees);
    cairo_close_path(cr);
}

static gboolean dc_app_draw_window_background(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    GtkAllocation allocation;

    (void) user_data;

    gtk_widget_get_allocation(widget, &allocation);
    cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.50);
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    dc_app_cairo_rounded_rectangle(cr, 0, 0, allocation.width, allocation.height, 12.0);
    cairo_fill(cr);
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

    return FALSE;
}

void dc_app_add_css_class(GtkWidget *widget, const char *class_name) {
    gtk_style_context_add_class(gtk_widget_get_style_context(widget), class_name);
}

void dc_app_install_css(void) {
    GtkCssProvider *provider;
    GdkScreen *screen;
    char *cwd;
    char *css_path;
    GError *error = NULL;

    provider = gtk_css_provider_new();
    cwd = g_get_current_dir();
    css_path = g_build_filename(cwd, "assets", "theme.css", NULL);
    gtk_css_provider_load_from_path(provider, css_path, &error);
    screen = gdk_screen_get_default();
    if (screen != NULL) {
        gtk_style_context_add_provider_for_screen(screen,
                                                  GTK_STYLE_PROVIDER(provider),
                                                  GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    }

    if (error != NULL) {
        g_warning("Failed to load CSS from %s: %s", css_path, error->message);
        g_error_free(error);
    }

    g_free(cwd);
    g_free(css_path);
    g_object_unref(provider);
}

void dc_app_set_status(DcAppController *app, const char *message) {
    gtk_label_set_text(GTK_LABEL(dc_display_page_get_status_label(app->display_page)), message);
}

char *dc_app_resolve_venom_config_path(void) {
    if (g_file_test(DC_PRIMARY_VENOM_CONFIG_PATH, G_FILE_TEST_EXISTS)) {
        return g_strdup(DC_PRIMARY_VENOM_CONFIG_PATH);
    }

    if (g_file_test(DC_FALLBACK_VENOM_CONFIG_PATH, G_FILE_TEST_EXISTS)) {
        return g_strdup(DC_FALLBACK_VENOM_CONFIG_PATH);
    }

    return g_strdup(DC_PRIMARY_VENOM_CONFIG_PATH);
}

static void activate(GtkApplication *gtk_app, gpointer user_data) {
    DcAppController *app = user_data;
    GtkWidget *root_box;
    GtkWidget *main_content_box;
    GtkWidget *headerbar;
    GtkWidget *sidebar;
    GtkWidget *stack_frame;

    app->preview = dc_preview_canvas_new();
    app->display_page = dc_display_page_new(dc_preview_canvas_get_widget(app->preview));
    app->audio_page = dc_audio_page_new();
    app->themes_page = dc_themes_page_new();
    app->display_edit_page = dc_display_edit_page_new();
    app->window_manager_page = dc_window_manager_page_new();
    app->compositor_page = dc_compositor_page_new();
    app->power_page = dc_power_page_new();
    app->keyboard_page = dc_keyboard_page_new();
    app->mouse_page = dc_mouse_page_new();
    app->about_page = dc_about_page_new();
    app->default_apps_page = dc_default_apps_page_new();
    app->system_page = dc_system_page_new();
    app->bluetooth_page = dc_bluetooth_page_new();
    app->wifi_page = dc_wifi_page_new();
    app->ethernet_page = dc_ethernet_page_new();
    app->window = gtk_application_window_new(gtk_app);
    gtk_window_set_title(GTK_WINDOW(app->window), "Display Settings");
    gtk_window_set_default_size(GTK_WINDOW(app->window), 980, 760);
    gtk_window_set_decorated(GTK_WINDOW(app->window), FALSE);
    gtk_container_set_border_width(GTK_CONTAINER(app->window), 12);
    gtk_widget_set_name(app->window, "display-core-window");
    dc_app_add_css_class(app->window, "app-window");
    gtk_widget_set_app_paintable(app->window, TRUE);
    g_signal_connect(app->window, "draw", G_CALLBACK(dc_app_draw_window_background), NULL);

    {
        GdkScreen *screen = gtk_widget_get_screen(app->window);
        GdkVisual *visual = gdk_screen_get_rgba_visual(screen);

        if (visual != NULL && gdk_screen_is_composited(screen)) {
            gtk_widget_set_visual(app->window, visual);
        }
    }

    root_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    main_content_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    
    stack_frame = gtk_frame_new(NULL);
    app->stack = gtk_stack_new();
    gtk_stack_set_transition_type(GTK_STACK(app->stack), GTK_STACK_TRANSITION_TYPE_CROSSFADE);
    gtk_stack_set_transition_duration(GTK_STACK(app->stack), 220);

    sidebar = gtk_stack_sidebar_new();
    gtk_stack_sidebar_set_stack(GTK_STACK_SIDEBAR(sidebar), GTK_STACK(app->stack));
    gtk_widget_set_size_request(sidebar, 180, -1);
    
    headerbar = dc_custom_headerbar_new(GTK_WINDOW(app->window), "Settings", NULL);

    dc_app_add_css_class(root_box, "app-shell");
    dc_app_add_css_class(sidebar, "dc-sidebar");
    dc_app_add_css_class(stack_frame, "content-card");

    gtk_container_add(GTK_CONTAINER(app->window), root_box);
    gtk_container_add(GTK_CONTAINER(stack_frame), app->stack);

    gtk_frame_set_shadow_type(GTK_FRAME(stack_frame), GTK_SHADOW_NONE);
    gtk_container_set_border_width(GTK_CONTAINER(stack_frame), 0);

    gtk_box_pack_start(GTK_BOX(main_content_box), sidebar, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(main_content_box), stack_frame, TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(root_box), headerbar, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(root_box), main_content_box, TRUE, TRUE, 0);

    gtk_stack_add_titled(GTK_STACK(app->stack),
                         dc_display_page_get_widget(app->display_page),
                         "display",
                         "Display");
    gtk_stack_add_titled(GTK_STACK(app->stack),
                         dc_audio_page_get_widget(app->audio_page),
                         "audio",
                         "Audio");
    gtk_stack_add_titled(GTK_STACK(app->stack),
                         dc_themes_page_get_widget(app->themes_page),
                         "themes",
                         "Themes");
    gtk_stack_add_titled(GTK_STACK(app->stack),
                         dc_display_edit_page_get_widget(app->display_edit_page),
                         "display-edit",
                         "Display Edit");
    gtk_stack_add_titled(GTK_STACK(app->stack),
                         dc_window_manager_page_get_widget(app->window_manager_page),
                         "window-manager",
                         "Window Manager");
    gtk_stack_add_titled(GTK_STACK(app->stack),
                         dc_compositor_page_get_widget(app->compositor_page),
                         "compositor",
                         "Compositor");
    gtk_stack_add_titled(GTK_STACK(app->stack),
                         dc_power_page_get_widget(app->power_page),
                         "power",
                         "Power");
    gtk_stack_add_titled(GTK_STACK(app->stack),
                         dc_keyboard_page_get_widget(app->keyboard_page),
                         "keyboard",
                         "Keyboard");
    gtk_stack_add_titled(GTK_STACK(app->stack),
                         dc_mouse_page_get_widget(app->mouse_page),
                         "mouse",
                         "Mouse & Touchpad");
    gtk_stack_add_titled(GTK_STACK(app->stack),
                         dc_about_page_get_widget(app->about_page),
                         "about",
                         "System About");
    gtk_stack_add_titled(GTK_STACK(app->stack),
                         dc_default_apps_page_get_widget(app->default_apps_page),
                         "default_apps",
                         "Default Apps");
    gtk_stack_add_titled(GTK_STACK(app->stack),
                         dc_system_page_get_widget(app->system_page),
                         "system",
                         "System");
    gtk_stack_add_titled(GTK_STACK(app->stack),
                         dc_bluetooth_page_get_widget(app->bluetooth_page),
                         "bluetooth",
                         "Bluetooth");
    gtk_stack_add_titled(GTK_STACK(app->stack),
                         dc_wifi_page_get_widget(app->wifi_page),
                         "wifi",
                         "Wi-Fi");
    gtk_stack_add_titled(GTK_STACK(app->stack),
                         dc_ethernet_page_get_widget(app->ethernet_page),
                         "ethernet",
                         "Ethernet");

    dc_app_connect_display_page_signals(app);
    gtk_widget_show_all(app->window);
    dc_app_reload_outputs(app);
    dc_app_audio_load(app);
    dc_app_display_edit_load(app);
    dc_app_themes_load(app);
    dc_app_window_manager_load(app);
    dc_app_display_edit_configure_capabilities(app);
    dc_app_compositor_load(app);
    dc_app_power_load(app);
    dc_app_keyboard_load(app);
    dc_app_mouse_load(app);
    dc_app_about_load(app);
    dc_app_default_apps_load(app);
    dc_app_system_load(app);
    dc_app_bluetooth_load(app);
    dc_app_wifi_load(app);
    dc_app_ethernet_load(app);
    
    dc_app_audio_connect_signals(app);
    dc_app_display_edit_connect_signals(app);
    dc_app_themes_connect_signals(app);
    dc_app_window_manager_connect_signals(app);
    dc_app_compositor_connect_signals(app);
    dc_app_power_connect_signals(app);
    dc_app_keyboard_connect_signals(app);
    dc_app_mouse_connect_signals(app);
    dc_app_default_apps_connect_signals(app);
    dc_app_system_connect_signals(app);
    dc_app_bluetooth_connect_signals(app);
    dc_app_wifi_connect_signals(app);
    dc_app_ethernet_connect_signals(app);
    app->display_edit_refresh_timeout_id = g_timeout_add_seconds(60, dc_app_display_edit_refresh_runtime, app);
}

static DcAppController *dc_app_controller_new(char **error_message) {
    DcAppController *app = g_new0(DcAppController, 1);

    app->service = dc_xrandr_service_new(error_message);
    if (app->service == NULL) {
        g_free(app);
        return NULL;
    }
    app->power_service = dc_power_service_new();
    app->input_service = dc_input_service_new();
    app->sysinfo_service = dc_sysinfo_service_new();
    app->default_apps_service = dc_default_apps_service_new();
    app->system_service = dc_system_service_new();
    app->bluetooth_service = dc_bluetooth_service_new();
    app->network_service   = dc_network_service_new();

    dc_app_install_css();
    app->gtk_app = gtk_application_new("com.displaycore.settings", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app->gtk_app, "activate", G_CALLBACK(activate), app);
    return app;
}

static void dc_app_controller_free(DcAppController *app) {
    if (app == NULL) {
        return;
    }

    dc_app_clear_rows(app);

    if (app->display_page != NULL) {
        dc_display_page_free(app->display_page);
    }
    if (app->audio_page != NULL) {
        dc_audio_page_free(app->audio_page);
    }
    if (app->display_edit_page != NULL) {
        dc_display_edit_page_free(app->display_edit_page);
    }
    if (app->themes_page != NULL) {
        dc_themes_page_free(app->themes_page);
    }
    if (app->window_manager_page != NULL) {
        dc_window_manager_page_free(app->window_manager_page);
    }
    if (app->compositor_page != NULL) {
        dc_compositor_page_free(app->compositor_page);
    }
    if (app->power_page != NULL) {
        dc_power_page_free(app->power_page);
    }
    if (app->keyboard_page != NULL) {
        dc_keyboard_page_free(app->keyboard_page);
    }
    if (app->mouse_page != NULL) {
        dc_mouse_page_free(app->mouse_page);
    }
    if (app->about_page != NULL) {
        dc_about_page_free(app->about_page);
    }
    if (app->default_apps_page != NULL) {
        dc_default_apps_page_free(app->default_apps_page);
    }
    if (app->system_page != NULL) {
        dc_system_page_free(app->system_page);
    }
    if (app->bluetooth_page != NULL) {
        dc_bluetooth_page_free(app->bluetooth_page);
    }
    if (app->wifi_page != NULL) {
        dc_wifi_page_free(app->wifi_page);
    }
    if (app->ethernet_page != NULL) {
        dc_ethernet_page_free(app->ethernet_page);
    }
    if (app->preview != NULL) {
        dc_preview_canvas_free(app->preview);
    }
    if (app->compositor_autosave_timeout_id != 0) {
        g_source_remove(app->compositor_autosave_timeout_id);
    }
    if (app->themes_autosave_timeout_id != 0) {
        g_source_remove(app->themes_autosave_timeout_id);
    }
    if (app->window_manager_autosave_timeout_id != 0) {
        g_source_remove(app->window_manager_autosave_timeout_id);
    }
    if (app->display_edit_autosave_timeout_id != 0) {
        g_source_remove(app->display_edit_autosave_timeout_id);
    }
    if (app->display_edit_refresh_timeout_id != 0) {
        g_source_remove(app->display_edit_refresh_timeout_id);
    }
    if (app->audio_refresh_idle_id != 0) {
        g_source_remove(app->audio_refresh_idle_id);
    }

    if (app->gtk_app != NULL) {
        g_object_unref(app->gtk_app);
    }

    if (app->service != NULL) {
        dc_xrandr_service_free(app->service);
    }
    if (app->power_service != NULL) {
        g_object_unref(app->power_service);
    }
    if (app->input_service != NULL) {
        g_object_unref(app->input_service);
    }
    if (app->sysinfo_service != NULL) {
        g_object_unref(app->sysinfo_service);
    }
    if (app->default_apps_service != NULL) {
        g_object_unref(app->default_apps_service);
    }
    if (app->system_service != NULL) {
        g_object_unref(app->system_service);
    }
    if (app->bluetooth_service != NULL) {
        g_object_unref(app->bluetooth_service);
    }
    if (app->network_service != NULL) {
        g_object_unref(app->network_service);
    }
    dc_audio_service_cleanup();

    g_free(app);
}

int dc_app_controller_run(int argc, char **argv) {
    DcAppController *app;
    char *error_message = NULL;
    int status;

    app = dc_app_controller_new(&error_message);
    if (app == NULL) {
        g_printerr("%s\n", error_message != NULL ? error_message : "Failed to initialize the app.");
        g_free(error_message);
        return 1;
    }

    status = g_application_run(G_APPLICATION(app->gtk_app), argc, argv);
    dc_app_controller_free(app);
    g_free(error_message);
    return status;
}
