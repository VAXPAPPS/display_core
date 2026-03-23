#include "ui/custom_headerbar.h"
//     "  padding: 8px 10px 8px 16px;"
static const char *CUSTOM_HEADERBAR_CSS =
    ".dc-headerbar {"
    "  background:rgba(0, 0, 0, 0);"
    "  border: none;"
    "  border-radius: 18px;"
    "  padding: 8px 10px 8px 16px;"
    "  min-height: 26px;"
    "}"
    ".dc-window-btn {"
    "  min-width: 15px;"
    "  min-height: 15px;"
    "  background-image: none;"
    "  background-color: transparent;"
    "  color: transparent;"
    "  border-radius: 999px;"
    "  padding: 0;"
    "  margin: 0 4px;"
    "  border: 1px solid transparent;"
    "  box-shadow: inset 0 -1px 2px rgba(0,0,0,0.2);"
    "}"
    ".dc-window-btn:hover {"
    "  background-image: none;"
    "  box-shadow: inset 0 -1px 2px rgba(0,0,0,0.2), 0 0 6px rgba(255,255,255,0.15);"
    "}"
    ".dc-window-btn:active,"
    ".dc-window-btn:checked,"
    ".dc-window-btn:focus {"
    "  background-image: none;"
    "  color: transparent;"
    "  outline: none;"
    "}"
    ".dc-btn-close {"
    "  background-color: #ff5f57;"
    "}"
    ".dc-btn-close:hover {"
    "  background-color: #ff3b30;"
    "}"
    ".dc-btn-minimize {"
    "  background-color: #ffbd2e;"
    "}"
    ".dc-btn-minimize:hover {"
    "  background-color: #f5a623;"
    "}"
    ".dc-btn-maximize {"
    "  background-color: #28c840;"
    "}"
    ".dc-btn-maximize:hover {"
    "  background-color: #1db954;"
    "}"
    ".dc-header-title {"
    "  color: rgba(255,255,255,0.9);"
    "  font-size: 14px;"
    "  font-weight: 600;"
    "}";

static void install_custom_headerbar_css(GtkWidget *widget) {
    static gboolean installed = FALSE;
    GtkCssProvider *provider;
    GdkScreen *screen;

    if (installed) {
        return;
    }

    screen = gtk_widget_get_screen(widget);
    if (screen == NULL) {
        return;
    }

    provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider, CUSTOM_HEADERBAR_CSS, -1, NULL);
    gtk_style_context_add_provider_for_screen(screen,
                                              GTK_STYLE_PROVIDER(provider),
                                              GTK_STYLE_PROVIDER_PRIORITY_USER);
    g_object_unref(provider);
    installed = TRUE;
}

static void on_close_clicked(GtkButton *button, gpointer user_data) {
    (void) button;
    gtk_window_close(GTK_WINDOW(user_data));
}

static void on_minimize_clicked(GtkButton *button, gpointer user_data) {
    (void) button;
    gtk_window_iconify(GTK_WINDOW(user_data));
}

static void on_maximize_clicked(GtkButton *button, gpointer user_data) {
    GtkWindow *window;

    (void) button;
    window = GTK_WINDOW(user_data);
    if (gtk_window_is_maximized(window)) {
        gtk_window_unmaximize(window);
    } else {
        gtk_window_maximize(window);
    }
}

static gboolean on_headerbar_button_press(GtkWidget *widget,
                                          GdkEventButton *event,
                                          gpointer user_data) {
    (void) widget;

    if (event->type == GDK_BUTTON_PRESS && event->button == 1) {
        gtk_window_begin_move_drag(GTK_WINDOW(user_data),
                                   event->button,
                                   event->x_root,
                                   event->y_root,
                                   event->time);
        return TRUE;
    }

    return FALSE;
}

static GtkWidget *create_window_button(const char *css_class,
                                       GCallback callback,
                                       gpointer user_data,
                                       const char *tooltip) {
    GtkWidget *button;
    GtkStyleContext *context;

    button = gtk_button_new();
    context = gtk_widget_get_style_context(button);
    gtk_style_context_add_class(context, "dc-window-btn");
    gtk_style_context_add_class(context, css_class);
    gtk_widget_set_can_focus(button, FALSE);
    gtk_widget_set_tooltip_text(button, tooltip);
    g_signal_connect(button, "clicked", callback, user_data);
    return button;
}

GtkWidget *dc_custom_headerbar_new(GtkWindow *window,
                                   const char *title_text,
                                   GtkWidget *center_widget) {
    GtkWidget *event_box;
    GtkWidget *header_box;
    GtkWidget *center_box;
    GtkWidget *controls_box;
    GtkWidget *minimize_button;
    GtkWidget *maximize_button;
    GtkWidget *close_button;
    GtkStyleContext *context;

    event_box = gtk_event_box_new();
    gtk_widget_add_events(event_box, GDK_BUTTON_PRESS_MASK);
    g_signal_connect(event_box,
                     "button-press-event",
                     G_CALLBACK(on_headerbar_button_press),
                     window);
    install_custom_headerbar_css(event_box);

    header_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    context = gtk_widget_get_style_context(header_box);
    gtk_style_context_add_class(context, "dc-headerbar");
    gtk_container_add(GTK_CONTAINER(event_box), header_box);

    center_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_hexpand(center_box, TRUE);
    gtk_widget_set_halign(center_box, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(center_box, GTK_ALIGN_CENTER);
    gtk_widget_set_margin_start(center_box, 8);
    
    if (title_text != NULL && strlen(title_text) > 0) {
        GtkWidget *title_label = gtk_label_new(title_text);
        GtkStyleContext *tctx = gtk_widget_get_style_context(title_label);
        gtk_style_context_add_class(tctx, "dc-header-title");
        gtk_box_pack_start(GTK_BOX(center_box), title_label, FALSE, FALSE, 0);
    }
    
    if (center_widget != NULL) {
        gtk_box_pack_start(GTK_BOX(center_box), center_widget, FALSE, FALSE, 0);
    }

    controls_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_margin_end(controls_box, 6);
    gtk_widget_set_valign(controls_box, GTK_ALIGN_CENTER);
    minimize_button = create_window_button("dc-btn-minimize",
                                           G_CALLBACK(on_minimize_clicked),
                                           window,
                                           "Minimize");
    maximize_button = create_window_button("dc-btn-maximize",
                                           G_CALLBACK(on_maximize_clicked),
                                           window,
                                           "Maximize");
    close_button = create_window_button("dc-btn-close",
                                        G_CALLBACK(on_close_clicked),
                                        window,
                                        "Close");
    gtk_box_pack_start(GTK_BOX(controls_box), minimize_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(controls_box), maximize_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(controls_box), close_button, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(header_box), center_box, TRUE, TRUE, 0);
    gtk_box_pack_end(GTK_BOX(header_box), controls_box, FALSE, FALSE, 0);

    return event_box;
}
