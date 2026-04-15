#include "ui/pages/bluetooth_page.h"

struct _DcBluetoothPage {
    GtkWidget *root;
    GtkWidget *switch_power;
    GtkWidget *list_devices;

    DcBluetoothPageCallbacks cb;
    gpointer user_data;
};

static const char *BLUETOOTH_CSS =
    ".bt-scroll-hidden scrollbar { opacity: 0; min-width: 0; min-height: 0; }"
    ".bt-shell { padding: 40px 18px 48px; }"
    ".bt-group-label { color: rgba(255,255,255,0.28); font-size: 10px; font-weight: 700; letter-spacing: 0.09em; padding: 10px 18px 2px; }"
    ".bt-row { padding: 12px 18px; border-bottom: 1px solid rgba(255,255,255,0.05); }"
    ".bt-title { color: rgba(255,255,255,0.92); font-size: 13px; font-weight: 600; }"
    ".bt-desc { color: rgba(255,255,255,0.60); font-size: 11px; margin-top: 4px; }"
    ".bt-card { background-color: rgba(14,14,14,0.72); border: 1px solid rgba(255,255,255,0.10); border-radius: 18px; margin-bottom: 24px; box-shadow: 0 4px 12px rgba(0,0,0,0.1); }"
    "switch.bt-switch { border-radius: 14px; outline: none; box-shadow: inset 0 0 2px rgba(0,0,0,0.2); }"
    "switch.bt-switch:checked { background-color: #2196F3; }"
    ".bt-btn { background-color: rgba(255,255,255,0.1); color: white; border: 1px solid rgba(255,255,255,0.2); border-radius: 8px; padding: 6px 12px; font-weight: 600; cursor: pointer; transition: all 0.2s; }"
    ".bt-btn:hover { background-color: rgba(255,255,255,0.2); }"
    ".bt-btn-primary { background-color: #2196F3; border-color: #2196F3; }"
    ".bt-btn-primary:hover { background-color: #1976D2; border-color: #1976D2; }"
    ".bt-btn-danger { background-color: rgba(244,67,54,0.2); color: #F44336; border-color: rgba(244,67,54,0.4); }"
    ".bt-btn-danger:hover { background-color: rgba(244,67,54,0.4); }"
    ;

static void on_realize(GtkWidget *widget, gpointer user_data) {
    (void)user_data;
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider, BLUETOOTH_CSS, -1, NULL);
    gtk_style_context_add_provider_for_screen(
        gtk_widget_get_screen(widget),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
    );
    g_object_unref(provider);
}

static void add_css_class(GtkWidget *widget, const char *class_name) {
    gtk_style_context_add_class(gtk_widget_get_style_context(widget), class_name);
}

static GtkWidget *create_group_label(const char *text) {
    GtkWidget *label = gtk_label_new(text);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    add_css_class(label, "bt-group-label");
    return label;
}

static GtkWidget *create_switch_row(const char *title, const char *desc, GtkWidget **out_switch) {
    GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    
    GtkWidget *vbox_text = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    GtkWidget *t_lbl = gtk_label_new(title);
    gtk_widget_set_halign(t_lbl, GTK_ALIGN_START);
    add_css_class(t_lbl, "bt-title");
    gtk_box_pack_start(GTK_BOX(vbox_text), t_lbl, FALSE, FALSE, 0);

    if (desc) {
        GtkWidget *d_lbl = gtk_label_new(desc);
        gtk_widget_set_halign(d_lbl, GTK_ALIGN_START);
        add_css_class(d_lbl, "bt-desc");
        gtk_box_pack_start(GTK_BOX(vbox_text), d_lbl, FALSE, FALSE, 0);
    }

    GtkWidget *sw = gtk_switch_new();
    gtk_widget_set_valign(vbox_text, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(sw, GTK_ALIGN_CENTER);
    gtk_widget_set_hexpand(vbox_text, TRUE);

    add_css_class(sw, "bt-switch");
    add_css_class(row, "bt-row");

    gtk_box_pack_start(GTK_BOX(row), vbox_text, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(row), sw, FALSE, FALSE, 0);

    *out_switch = sw;
    return row;
}

static GtkWidget *create_card(void) {
    GtkWidget *frame = gtk_frame_new(NULL);
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);
    gtk_container_set_border_width(GTK_CONTAINER(frame), 0);
    add_css_class(frame, "bt-card");

    gtk_container_add(GTK_CONTAINER(frame), vbox);
    return frame;
}

DcBluetoothPage *dc_bluetooth_page_new(void) {
    DcBluetoothPage *page = g_new0(DcBluetoothPage, 1);

    GtkWidget *content  = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    GtkWidget *scroller = gtk_scrolled_window_new(NULL, NULL);
    add_css_class(content, "bt-shell");
    add_css_class(scroller, "bt-scroll-hidden");

    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroller),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_overlay_scrolling(GTK_SCROLLED_WINDOW(scroller), TRUE);
    gtk_container_add(GTK_CONTAINER(scroller), content);
    g_signal_connect(scroller, "realize", G_CALLBACK(on_realize), NULL);

    /* Main Bluetooth Switch Card */
    GtkWidget *main_card = create_card();
    GtkWidget *main_box = gtk_bin_get_child(GTK_BIN(main_card));
    gtk_box_pack_start(GTK_BOX(main_box), create_group_label("BLUETOOTH"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(main_box), create_switch_row("Bluetooth", "Enable or disable bluetooth", &page->switch_power), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(content), main_card, FALSE, FALSE, 0);

    /* Devices Card */
    GtkWidget *devices_card = create_card();
    GtkWidget *devices_box = gtk_bin_get_child(GTK_BIN(devices_card));
    gtk_box_pack_start(GTK_BOX(devices_box), create_group_label("DEVICES"), FALSE, FALSE, 0);
    
    page->list_devices = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start(GTK_BOX(devices_box), page->list_devices, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(content), devices_card, FALSE, FALSE, 0);

    page->root = scroller;
    return page;
}

void dc_bluetooth_page_free(DcBluetoothPage *page) {
    if (page) g_free(page);
}

GtkWidget *dc_bluetooth_page_get_widget(DcBluetoothPage *page) {
    return page ? page->root : NULL;
}

GtkWidget *dc_bluetooth_page_get_power_switch(DcBluetoothPage *page) {
    return page ? page->switch_power : NULL;
}

void dc_bluetooth_page_set_callbacks(DcBluetoothPage *page, DcBluetoothPageCallbacks *cb, gpointer user_data) {
    if (!page || !cb) return;
    page->cb = *cb;
    page->user_data = user_data;
}

static void on_btn_clicked(GtkButton *btn, gpointer user_data) {
    GObject *obj = G_OBJECT(btn);
    DcBluetoothPage *page = g_object_get_data(obj, "page");
    char *path = g_object_get_data(obj, "path");
    const char *action = g_object_get_data(obj, "action");

    if (!page || !path || !action) return;

    if (g_strcmp0(action, "pair") == 0 && page->cb.on_pair) {
        page->cb.on_pair(path, page->user_data);
    } else if (g_strcmp0(action, "connect") == 0 && page->cb.on_connect) {
        page->cb.on_connect(path, page->user_data);
    } else if (g_strcmp0(action, "disconnect") == 0 && page->cb.on_disconnect) {
        page->cb.on_disconnect(path, page->user_data);
    } else if (g_strcmp0(action, "remove") == 0 && page->cb.on_remove) {
        page->cb.on_remove(path, page->user_data);
    }
}

void dc_bluetooth_page_populate_devices(DcBluetoothPage *page, GList *devices) {
    if (!page || !page->list_devices) return;

    /* clear existing */
    GList *children = gtk_container_get_children(GTK_CONTAINER(page->list_devices));
    for (GList *iter = children; iter != NULL; iter = g_list_next(iter)) {
        gtk_widget_destroy(GTK_WIDGET(iter->data));
    }
    g_list_free(children);

    if (!devices) {
        GtkWidget *empty_lbl = gtk_label_new("No devices found");
        add_css_class(empty_lbl, "bt-desc");
        gtk_widget_set_margin_top(empty_lbl, 16);
        gtk_widget_set_margin_bottom(empty_lbl, 16);
        gtk_box_pack_start(GTK_BOX(page->list_devices), empty_lbl, FALSE, FALSE, 0);
        gtk_widget_show_all(page->list_devices);
        return;
    }

    for (GList *iter = devices; iter != NULL; iter = g_list_next(iter)) {
        DcBluetoothDevice *dev = iter->data;

        GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
        add_css_class(row, "bt-row");

        GtkWidget *vbox_text = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
        GtkWidget *t_lbl = gtk_label_new(dev->name ? dev->name : "Unknown");
        gtk_widget_set_halign(t_lbl, GTK_ALIGN_START);
        add_css_class(t_lbl, "bt-title");
        gtk_box_pack_start(GTK_BOX(vbox_text), t_lbl, FALSE, FALSE, 0);

        char desc[256];
        g_snprintf(desc, sizeof(desc), "%s %s %s",
            dev->paired ? "[Paired]" : "",
            dev->connected ? "[Connected]" : "",
            dev->address ? dev->address : "");
            
        GtkWidget *d_lbl = gtk_label_new(desc);
        gtk_widget_set_halign(d_lbl, GTK_ALIGN_START);
        add_css_class(d_lbl, "bt-desc");
        gtk_box_pack_start(GTK_BOX(vbox_text), d_lbl, FALSE, FALSE, 0);

        gtk_widget_set_valign(vbox_text, GTK_ALIGN_CENTER);
        gtk_widget_set_hexpand(vbox_text, TRUE);
        gtk_box_pack_start(GTK_BOX(row), vbox_text, TRUE, TRUE, 0);

        // Buttons
        if (!dev->paired) {
            GtkWidget *btn = gtk_button_new_with_label("Pair");
            add_css_class(btn, "bt-btn");
            g_object_set_data(G_OBJECT(btn), "page", page);
            g_object_set_data_full(G_OBJECT(btn), "path", g_strdup(dev->object_path), g_free);
            g_object_set_data(G_OBJECT(btn), "action", "pair");
            g_signal_connect(btn, "clicked", G_CALLBACK(on_btn_clicked), NULL);
            gtk_widget_set_valign(btn, GTK_ALIGN_CENTER);
            gtk_box_pack_start(GTK_BOX(row), btn, FALSE, FALSE, 0);
        } else {
            if (dev->connected) {
                GtkWidget *btn = gtk_button_new_with_label("Disconnect");
                add_css_class(btn, "bt-btn");
                g_object_set_data(G_OBJECT(btn), "page", page);
                g_object_set_data_full(G_OBJECT(btn), "path", g_strdup(dev->object_path), g_free);
                g_object_set_data(G_OBJECT(btn), "action", "disconnect");
                g_signal_connect(btn, "clicked", G_CALLBACK(on_btn_clicked), NULL);
                gtk_widget_set_valign(btn, GTK_ALIGN_CENTER);
                gtk_box_pack_start(GTK_BOX(row), btn, FALSE, FALSE, 0);
            } else {
                GtkWidget *btn = gtk_button_new_with_label("Connect");
                add_css_class(btn, "bt-btn");
                add_css_class(btn, "bt-btn-primary");
                g_object_set_data(G_OBJECT(btn), "page", page);
                g_object_set_data_full(G_OBJECT(btn), "path", g_strdup(dev->object_path), g_free);
                g_object_set_data(G_OBJECT(btn), "action", "connect");
                g_signal_connect(btn, "clicked", G_CALLBACK(on_btn_clicked), NULL);
                gtk_widget_set_valign(btn, GTK_ALIGN_CENTER);
                gtk_box_pack_start(GTK_BOX(row), btn, FALSE, FALSE, 0);
            }

            GtkWidget *rem_btn = gtk_button_new_with_label("Forget");
            add_css_class(rem_btn, "bt-btn");
            add_css_class(rem_btn, "bt-btn-danger");
            g_object_set_data(G_OBJECT(rem_btn), "page", page);
            g_object_set_data_full(G_OBJECT(rem_btn), "path", g_strdup(dev->object_path), g_free);
            g_object_set_data(G_OBJECT(rem_btn), "action", "remove");
            g_signal_connect(rem_btn, "clicked", G_CALLBACK(on_btn_clicked), NULL);
            gtk_widget_set_valign(rem_btn, GTK_ALIGN_CENTER);
            gtk_box_pack_start(GTK_BOX(row), rem_btn, FALSE, FALSE, 0);
        }

        gtk_box_pack_start(GTK_BOX(page->list_devices), row, FALSE, FALSE, 0);
    }
    gtk_widget_show_all(page->list_devices);
}
