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

static gboolean dc_app_draw_sidebar_pill(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    GtkAllocation allocation;
    gboolean active;
    gboolean hovered;

    (void) user_data;

    gtk_widget_get_allocation(widget, &allocation);
    active = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "dc-active"));
    hovered = (gtk_widget_get_state_flags(widget) & GTK_STATE_FLAG_PRELIGHT) != 0;

    dc_app_cairo_rounded_rectangle(cr, 0.5, 0.5,
                                   allocation.width - 1.0,
                                   allocation.height - 1.0,
                                   18.0);

    if (active) {
        cairo_pattern_t *fill = cairo_pattern_create_linear(0, 0, 0, allocation.height);
        cairo_pattern_add_color_stop_rgba(fill, 0.0, 0.29, 0.49, 0.80, 0.78);
        cairo_pattern_add_color_stop_rgba(fill, 1.0, 0.18, 0.29, 0.56, 0.62);
        cairo_set_source(cr, fill);
        cairo_fill_preserve(cr);
        cairo_pattern_destroy(fill);

        cairo_set_source_rgba(cr, 0.39, 0.87, 1.0, 0.95);
        cairo_set_line_width(cr, 1.4);
        cairo_stroke(cr);
    } else if (hovered) {
        cairo_pattern_t *fill = cairo_pattern_create_linear(0, 0, 0, allocation.height);
        cairo_pattern_add_color_stop_rgba(fill, 0.0, 0.22, 0.38, 0.58, 0.22);
        cairo_pattern_add_color_stop_rgba(fill, 1.0, 1.0, 1.0, 1.0, 0.05);
        cairo_set_source(cr, fill);
        cairo_fill_preserve(cr);
        cairo_pattern_destroy(fill);

        cairo_set_source_rgba(cr, 0.40, 0.75, 0.98, 0.34);
        cairo_set_line_width(cr, 1.0);
        cairo_stroke(cr);
    } else {
        cairo_pattern_t *fill = cairo_pattern_create_linear(0, 0, 0, allocation.height);
        cairo_pattern_add_color_stop_rgba(fill, 0.0, 1.0, 1.0, 1.0, 0.06);
        cairo_pattern_add_color_stop_rgba(fill, 1.0, 1.0, 1.0, 1.0, 0.02);
        cairo_set_source(cr, fill);
        cairo_fill_preserve(cr);
        cairo_pattern_destroy(fill);

        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.12);
        cairo_set_line_width(cr, 1.0);
        cairo_stroke(cr);
    }

    return FALSE;
}

static gboolean dc_app_draw_sidebar_indicator(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    GtkAllocation allocation;
    gboolean active;

    (void) user_data;

    gtk_widget_get_allocation(widget, &allocation);
    active = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "dc-active"));

    if (!active) {
        return FALSE;
    }

    dc_app_cairo_rounded_rectangle(cr, 0, 0, allocation.width, allocation.height, allocation.width / 2.0);
    cairo_set_source_rgba(cr, 0.47, 0.91, 1.0, 0.98);
    cairo_fill(cr);

    return FALSE;
}

static gboolean dc_app_draw_sidebar_icon_frame(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    GtkAllocation allocation;
    gboolean active;

    (void) user_data;

    gtk_widget_get_allocation(widget, &allocation);
    active = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "dc-active"));

    dc_app_cairo_rounded_rectangle(cr, 0.5, 0.5,
                                   allocation.width - 1.0,
                                   allocation.height - 1.0,
                                   allocation.width / 2.0);

    if (active) {
        cairo_set_source_rgba(cr, 0.36, 0.82, 1.0, 0.24);
        cairo_fill_preserve(cr);
        cairo_set_source_rgba(cr, 0.42, 0.86, 1.0, 0.62);
    } else {
        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.06);
        cairo_fill_preserve(cr);
        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.12);
    }

    cairo_set_line_width(cr, 1.0);
    cairo_stroke(cr);

    return FALSE;
}

void dc_app_add_css_class(GtkWidget *widget, const char *class_name) {
    gtk_style_context_add_class(gtk_widget_get_style_context(widget), class_name);
}

typedef struct {
    const char *name;
    const char *title;
    const char *icon_name;
} DcSidebarItem;

static const char *const DC_APP_THEME_CSS_PARTS[] = {
    ".app-window { background-color: rgba(0, 0, 0, 0.50); border-radius: 12px; }\n",
    ".app-shell { background-color: rgba(0, 0, 0, 0.50); border-radius: 12px; }\n",
    ".sidebar-card, .topbar-card, .content-card, .toolbar-card, .preview-card, .display-card { background-color: rgba(14, 14, 14, 0.72); border: 1px solid rgba(255, 255, 255, 0.10); border-radius: 18px; box-shadow: 0 12px 30px rgba(0, 0, 0, 0.22); }\n",
    ".toolbar-card, .sidebar-card, .topbar-card, .content-card, .preview-card { padding: 14px; }\n",
    ".page-shell { padding: 16px; }\n",
    ".page-title { color: rgba(255, 255, 255, 0.97); font-size: 24px; font-weight: 800; }\n",
    ".page-subtitle { color: rgba(255, 255, 255, 0.64); font-size: 13px; }\n",
    ".panel-card { background-color: rgba(14, 14, 14, 0.72); border: 1px solid rgba(255, 255, 255, 0.10); border-radius: 18px; padding: 16px; }\n",
    ".setting-title { color: rgba(255, 255, 255, 0.92); font-weight: 700; }\n",
    ".setting-description { color: rgba(255, 255, 255, 0.60); font-size: 12px; }\n",
    ".toolbar-label { color: rgba(255, 255, 255, 0.86); font-weight: 600; }\n",
    ".status-pill { color: rgba(255, 255, 255, 0.92); background-color: rgba(255, 255, 255, 0.08); border-radius: 999px; padding: 8px 14px; }\n",
    "button { color: rgba(255, 255, 255, 0.95); background-image: none; background-color: rgba(255, 255, 255, 0.10); border: 1px solid rgba(255, 255, 255, 0.12); border-radius: 12px; padding: 9px 14px; }\n",
    "button:hover { background-color: rgba(86, 110, 138, 0.16); }\n",
    ".suggested-action { background-color: rgba(255, 255, 255, 0.22); }\n",
    "entry, combobox box, spinbutton { color: rgba(255, 255, 255, 0.95); background-color: rgba(0, 0, 0, 0.24); border: 1px solid rgba(255, 255, 255, 0.10); border-radius: 12px; }\n",
    "label { color: rgba(255, 255, 255, 0.92); }\n",
    "frame>border { border-radius: 18px; border-width: 0; }\n",
    ".display-card { padding: 10px 12px 14px 12px; }\n",
    ".display-card>label { color: rgba(255, 255, 255, 0.95); font-weight: 700; padding-bottom: 10px; }\n",
    ".display-muted { color: rgba(255, 255, 255, 0.66); }\n",
    ".preview-hint { color: rgba(255, 255, 255, 0.70); }\n",
    ".topbar-card { padding: 10px 14px; }\n",
    "stackswitcher.topbar-switcher { background-color: transparent; }\n",
    "stackswitcher.topbar-switcher>button { color: rgba(255, 255, 255, 0.78); background-color: transparent; border: none; border-radius: 12px; padding: 10px 16px; margin: 0 4px; box-shadow: none; }\n",
    "stackswitcher.topbar-switcher>button:hover { background-color: rgba(255, 255, 255, 0.08); color: rgba(255, 255, 255, 0.94); }\n",
    "stackswitcher.topbar-switcher>button:checked { background-color: rgba(255, 255, 255, 0.16); color: rgba(255, 255, 255, 0.98); border: 1px solid rgba(255, 255, 255, 0.08); }\n",
    ".dc-sidebar { background-color: rgba(12, 14, 24, 0.76); background-image: linear-gradient(180deg, rgba(34, 45, 88, 0.56) 0%, rgba(18, 18, 31, 0.82) 46%, rgba(15, 12, 24, 0.88) 100%); border: 1px solid rgba(255, 255, 255, 0.10); border-radius: 30px; box-shadow: inset 0 1px 0 rgba(255, 255, 255, 0.07), 0 24px 54px rgba(0, 0, 0, 0.34); padding: 14px 10px; }\n",
    ".dc-sidebar scrolledwindow, .dc-sidebar viewport, .dc-sidebar-scroller { background-color: transparent; border: none; }\n",
    ".dc-sidebar list, .dc-sidebar-list { background-color: transparent; padding: 0 2px 2px 2px; }\n",
    ".dc-sidebar list row, .dc-sidebar-list row, .dc-sidebar-row { background: transparent; border: none; box-shadow: none; padding: 0; }\n",
    ".dc-sidebar list row:selected, .dc-sidebar list row:selected:hover, .dc-sidebar-list row:selected, .dc-sidebar-list row:selected:hover, .dc-sidebar-row:selected, .dc-sidebar-row:selected:hover { background: transparent; border: none; box-shadow: none; }\n",
    ".dc-sidebar-row-box { padding: 13px 16px; }\n",
    ".dc-sidebar-icon { color: rgba(255, 255, 255, 0.86); opacity: 0.96; }\n",
    ".dc-sidebar-icon-active { color: rgba(111, 225, 255, 0.98); }\n",
    ".dc-sidebar-label { color: rgba(255, 255, 255, 0.90); font-size: 15px; font-weight: 800; letter-spacing: 0.3px; text-shadow: 0 1px 0 rgba(0, 0, 0, 0.24); }\n",
    ".dc-sidebar-label-active { color: rgba(255, 255, 255, 0.98); }\n",
    ".dc-sidebar scrollbar { background: transparent; border: none; opacity: 0; min-width: 0; min-height: 0; }\n",
    ".dc-sidebar scrollbar slider { background-color: transparent; border: none; min-width: 0; min-height: 0; }\n"
};

static const DcSidebarItem DC_SIDEBAR_ITEMS[] = {
    { "wifi", "Wi-Fi", "network-wireless-symbolic" },
    { "bluetooth", "Bluetooth", "bluetooth-active-symbolic" },
    { "ethernet", "Ethernet", "network-wired-symbolic" },
    { "display", "Display", "video-display-symbolic" },
    { "audio", "Sound", "audio-speakers-symbolic" },
    { "mouse", "Mouse", "input-mouse-symbolic" },
    { "keyboard", "Keyboard", "input-keyboard-symbolic" },
    { "power", "Power", "system-shutdown-symbolic" },
    { "themes", "Themes", "applications-graphics-symbolic" },
    { "window-manager", "Window Manager", "preferences-system-windows-symbolic" },
    { "compositor", "Compositor", "preferences-desktop-effects-symbolic" },
    { "display-edit", "Display Edit", "document-edit-symbolic" },
    { "default_apps", "Default Apps", "preferences-desktop-apps-symbolic" },
    { "system", "System", "emblem-system-symbolic" },
    { "about", "About", "help-about-symbolic" }
};

void dc_app_install_css(void) {
    GtkCssProvider *provider;
    GdkScreen *screen;
    GError *error = NULL;
    GString *css_data;

    provider = gtk_css_provider_new();
    css_data = g_string_new(NULL);
    for (gsize i = 0; i < G_N_ELEMENTS(DC_APP_THEME_CSS_PARTS); i++) {
        g_string_append(css_data, DC_APP_THEME_CSS_PARTS[i]);
    }
    gtk_css_provider_load_from_data(provider, css_data->str, -1, &error);
    screen = gdk_screen_get_default();
    if (screen != NULL) {
        gtk_style_context_add_provider_for_screen(screen,
                                                  GTK_STYLE_PROVIDER(provider),
                                                  GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    }

    if (error != NULL) {
        g_warning("Failed to load embedded CSS: %s", error->message);
        g_error_free(error);
    }

    g_string_free(css_data, TRUE);
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

static gboolean on_sidebar_row_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data);

static GtkWidget *dc_app_create_sidebar_row(const DcSidebarItem *item) {
    GtkWidget *row;
    GtkWidget *pill;
    GtkWidget *pill_box;
    GtkWidget *content;
    GtkWidget *indicator;
    GtkWidget *icon_frame;
    GtkWidget *icon;
    GtkWidget *label;

    row = gtk_event_box_new();
    pill = gtk_event_box_new();
    pill_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    content = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    indicator = gtk_event_box_new();
    icon_frame = gtk_event_box_new();
    icon = gtk_image_new_from_icon_name(item->icon_name, GTK_ICON_SIZE_LARGE_TOOLBAR);
    label = gtk_label_new(item->title);

    gtk_event_box_set_visible_window(GTK_EVENT_BOX(row), FALSE);
    gtk_event_box_set_visible_window(GTK_EVENT_BOX(pill), TRUE);
    gtk_event_box_set_visible_window(GTK_EVENT_BOX(indicator), TRUE);
    gtk_event_box_set_visible_window(GTK_EVENT_BOX(icon_frame), TRUE);
    gtk_widget_add_events(row, GDK_BUTTON_PRESS_MASK | GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK);
    gtk_widget_set_app_paintable(pill, TRUE);
    gtk_widget_set_app_paintable(indicator, TRUE);
    gtk_widget_set_app_paintable(icon_frame, TRUE);
    gtk_widget_set_halign(pill, GTK_ALIGN_FILL);
    gtk_widget_set_valign(pill, GTK_ALIGN_FILL);
    gtk_widget_set_halign(pill_box, GTK_ALIGN_FILL);
    gtk_widget_set_valign(pill_box, GTK_ALIGN_FILL);
    gtk_widget_set_halign(content, GTK_ALIGN_FILL);
    gtk_widget_set_valign(content, GTK_ALIGN_CENTER);
    gtk_widget_set_halign(indicator, GTK_ALIGN_START);
    gtk_widget_set_valign(indicator, GTK_ALIGN_CENTER);
    gtk_widget_set_halign(icon_frame, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(icon_frame, GTK_ALIGN_CENTER);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_label_set_xalign(GTK_LABEL(label), 0.0f);
    gtk_widget_set_size_request(row, -1, 58);
    gtk_widget_set_margin_top(row, 6);
    gtk_widget_set_margin_bottom(row, 6);
    gtk_widget_set_margin_start(row, 6);
    gtk_widget_set_margin_end(row, 6);
    gtk_widget_set_margin_top(content, 10);
    gtk_widget_set_margin_bottom(content, 10);
    gtk_widget_set_margin_start(content, 14);
    gtk_widget_set_margin_end(content, 14);
    gtk_widget_set_margin_top(pill_box, 2);
    gtk_widget_set_margin_bottom(pill_box, 2);
    gtk_widget_set_size_request(icon_frame, 34, 34);
    gtk_widget_set_size_request(indicator, 4, 34);

    dc_app_add_css_class(row, "dc-sidebar-row");
    dc_app_add_css_class(content, "dc-sidebar-row-box");
    dc_app_add_css_class(icon, "dc-sidebar-icon");
    dc_app_add_css_class(label, "dc-sidebar-label");

    g_signal_connect(pill, "draw", G_CALLBACK(dc_app_draw_sidebar_pill), NULL);
    g_signal_connect(indicator, "draw", G_CALLBACK(dc_app_draw_sidebar_indicator), NULL);
    g_signal_connect(icon_frame, "draw", G_CALLBACK(dc_app_draw_sidebar_icon_frame), NULL);

    gtk_container_add(GTK_CONTAINER(icon_frame), icon);
    gtk_box_pack_start(GTK_BOX(pill_box), indicator, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(content), icon_frame, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(content), label, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(pill_box), content, TRUE, TRUE, 0);
    gtk_container_add(GTK_CONTAINER(pill), pill_box);
    gtk_container_add(GTK_CONTAINER(row), pill);
    g_object_set_data_full(G_OBJECT(row), "dc-stack-name", g_strdup(item->name), g_free);
    g_object_set_data(G_OBJECT(row), "dc-sidebar-pill", pill);
    g_object_set_data(G_OBJECT(row), "dc-sidebar-indicator", indicator);
    g_object_set_data(G_OBJECT(row), "dc-sidebar-icon-frame", icon_frame);
    g_object_set_data(G_OBJECT(row), "dc-sidebar-icon", icon);
    g_object_set_data(G_OBJECT(row), "dc-sidebar-label", label);

    return row;
}

static void dc_app_set_sidebar_row_active(GtkWidget *row, gboolean active) {
    GtkWidget *pill;
    GtkWidget *indicator;
    GtkWidget *icon_frame;
    GtkWidget *icon;
    GtkWidget *label;

    if (row == NULL) {
        return;
    }

    pill = g_object_get_data(G_OBJECT(row), "dc-sidebar-pill");
    indicator = g_object_get_data(G_OBJECT(row), "dc-sidebar-indicator");
    icon_frame = g_object_get_data(G_OBJECT(row), "dc-sidebar-icon-frame");
    icon = g_object_get_data(G_OBJECT(row), "dc-sidebar-icon");
    label = g_object_get_data(G_OBJECT(row), "dc-sidebar-label");

    g_object_set_data(G_OBJECT(pill), "dc-active", GINT_TO_POINTER(active));
    g_object_set_data(G_OBJECT(indicator), "dc-active", GINT_TO_POINTER(active));
    g_object_set_data(G_OBJECT(icon_frame), "dc-active", GINT_TO_POINTER(active));

    if (active) {
        dc_app_add_css_class(icon, "dc-sidebar-icon-active");
        dc_app_add_css_class(label, "dc-sidebar-label-active");
    } else {
        gtk_style_context_remove_class(gtk_widget_get_style_context(icon), "dc-sidebar-icon-active");
        gtk_style_context_remove_class(gtk_widget_get_style_context(label), "dc-sidebar-label-active");
    }

    gtk_widget_queue_draw(pill);
    gtk_widget_queue_draw(indicator);
    gtk_widget_queue_draw(icon_frame);
}

static void dc_app_select_sidebar_row(DcAppController *app, const char *visible_name) {
    GList *children;

    if (app == NULL || app->sidebar_list == NULL || visible_name == NULL) {
        return;
    }

    children = gtk_container_get_children(GTK_CONTAINER(app->sidebar_list));
    for (GList *iter = children; iter != NULL; iter = iter->next) {
        GtkWidget *row = GTK_WIDGET(iter->data);
        const char *row_name = g_object_get_data(G_OBJECT(row), "dc-stack-name");
        gboolean is_active = g_strcmp0(row_name, visible_name) == 0;

        dc_app_set_sidebar_row_active(row, is_active);
    }
    g_list_free(children);
}

static gboolean on_sidebar_row_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    DcAppController *app = user_data;
    const char *page_name;

    (void) event;

    if (app == NULL || widget == NULL || app->stack == NULL) {
        return FALSE;
    }

    page_name = g_object_get_data(G_OBJECT(widget), "dc-stack-name");
    if (page_name != NULL) {
        gtk_stack_set_visible_child_name(GTK_STACK(app->stack), page_name);
    }

    return TRUE;
}

static void on_stack_visible_child_changed(GObject *object, GParamSpec *pspec, gpointer user_data) {
    DcAppController *app = user_data;
    const char *visible_name;

    (void) object;
    (void) pspec;

    if (app == NULL || app->stack == NULL) {
        return;
    }

    visible_name = gtk_stack_get_visible_child_name(GTK_STACK(app->stack));
    dc_app_select_sidebar_row(app, visible_name);
}

static void activate(GtkApplication *gtk_app, gpointer user_data) {
    DcAppController *app = user_data;
    GtkWidget *root_box;
    GtkWidget *main_content_box;
    GtkWidget *headerbar;
    GtkWidget *sidebar;
    GtkWidget *sidebar_scroller;
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

    sidebar = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    sidebar_scroller = gtk_scrolled_window_new(NULL, NULL);
    app->sidebar_list = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_size_request(sidebar, 260, -1);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sidebar_scroller),
                                   GTK_POLICY_NEVER,
                                   GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_overlay_scrolling(GTK_SCROLLED_WINDOW(sidebar_scroller), TRUE);
    gtk_widget_hide(gtk_scrolled_window_get_vscrollbar(GTK_SCROLLED_WINDOW(sidebar_scroller)));
    gtk_widget_set_margin_top(sidebar, 8);
    gtk_widget_set_margin_bottom(sidebar, 8);
    gtk_widget_set_margin_start(sidebar, 4);
    gtk_widget_set_margin_end(sidebar, 6);
    gtk_container_add(GTK_CONTAINER(sidebar_scroller), app->sidebar_list);
    gtk_box_pack_start(GTK_BOX(sidebar), sidebar_scroller, TRUE, TRUE, 0);
    
    headerbar = dc_custom_headerbar_new(GTK_WINDOW(app->window), "Settings", NULL);

    dc_app_add_css_class(root_box, "app-shell");
    dc_app_add_css_class(sidebar, "dc-sidebar");
    dc_app_add_css_class(sidebar_scroller, "dc-sidebar-scroller");
    dc_app_add_css_class(app->sidebar_list, "dc-sidebar-list");
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

    for (gsize i = 0; i < G_N_ELEMENTS(DC_SIDEBAR_ITEMS); i++) {
        GtkWidget *row = dc_app_create_sidebar_row(&DC_SIDEBAR_ITEMS[i]);
        g_signal_connect(row, "button-press-event",
                         G_CALLBACK(on_sidebar_row_button_press), app);
        gtk_box_pack_start(GTK_BOX(app->sidebar_list), row, FALSE, FALSE, 0);
    }

    g_signal_connect(app->stack, "notify::visible-child-name",
                     G_CALLBACK(on_stack_visible_child_changed), app);
    gtk_stack_set_visible_child_name(GTK_STACK(app->stack), "wifi");
    dc_app_select_sidebar_row(app, "wifi");

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
