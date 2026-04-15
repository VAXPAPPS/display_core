#include "ui/pages/ethernet_page.h"

/* ═══════════════════════════════════════════════════════════════════════════
 * CSS — same visual design system as wifi_page / bluetooth_page
 * ═══════════════════════════════════════════════════════════════════════════ */

static const char *ETHERNET_CSS =
    ".eth-scroll-hidden scrollbar { opacity:0; min-width:0; min-height:0; }"
    ".eth-scroll-hidden scrollbar slider { min-width:0; min-height:0; }"

    ".eth-shell { padding: 40px 18px 48px; }"

    ".eth-group-label {"
    "  color: rgba(255,255,255,0.28);"
    "  font-size: 10px;"
    "  font-weight: 700;"
    "  letter-spacing: 0.09em;"
    "  padding: 10px 18px 2px;"
    "}"

    ".eth-card {"
    "  background-color: rgba(14,14,14,0.72);"
    "  border: 1px solid rgba(255,255,255,0.10);"
    "  border-radius: 18px;"
    "  margin-bottom: 24px;"
    "  box-shadow: 0 4px 12px rgba(0,0,0,0.10);"
    "}"

    ".eth-row {"
    "  padding: 12px 18px;"
    "  border-bottom: 1px solid rgba(255,255,255,0.05);"
    "}"

    ".eth-title { color: rgba(255,255,255,0.92); font-size: 13px; font-weight: 600; }"
    ".eth-value { color: rgba(255,255,255,0.65); font-size: 12px; font-family: monospace; }"
    ".eth-desc  { color: rgba(255,255,255,0.45); font-size: 11px; margin-top: 3px; }"

    /* Connected state badge */
    ".eth-badge-connected {"
    "  color: #66BB6A;"
    "  font-size: 10px;"
    "  font-weight: 700;"
    "  letter-spacing: 0.06em;"
    "}"
    ".eth-badge-disconnected {"
    "  color: rgba(255,255,255,0.35);"
    "  font-size: 10px;"
    "  font-weight: 700;"
    "  letter-spacing: 0.06em;"
    "}"

    /* Ethernet icon label */
    ".eth-icon {"
    "  color: rgba(255,255,255,0.70);"
    "  font-size: 28px;"
    "}"
    ".eth-icon-connected { color: #66BB6A; }"

    /* Buttons */
    ".eth-btn {"
    "  background-color: rgba(255,255,255,0.10);"
    "  color: white;"
    "  border: 1px solid rgba(255,255,255,0.20);"
    "  border-radius: 8px;"
    "  padding: 5px 12px;"
    "  font-weight: 600;"
    "}"
    ".eth-btn:hover { background-color: rgba(255,255,255,0.20); }"

    ".eth-btn-danger {"
    "  background-color: rgba(244,67,54,0.15);"
    "  color: #EF9A9A;"
    "  border-color: rgba(244,67,54,0.30);"
    "}"
    ".eth-btn-danger:hover { background-color: rgba(244,67,54,0.30); }"

    /* Speed pill */
    ".eth-speed-pill {"
    "  background-color: rgba(33,150,243,0.15);"
    "  color: #4FC3F7;"
    "  border: 1px solid rgba(33,150,243,0.25);"
    "  border-radius: 20px;"
    "  padding: 2px 10px;"
    "  font-size: 11px;"
    "  font-weight: 700;"
    "}"
    ;

/* ═══════════════════════════════════════════════════════════════════════════
 * Internal struct
 * ═══════════════════════════════════════════════════════════════════════════ */

struct _DcEthernetPage {
    GtkWidget *root;
    GtkWidget *adapters_box;   /* GtkBox that holds per-adapter cards */

    DcEthernetPageCallbacks  cb;
    gpointer                 user_data;
};

/* ── helpers ─────────────────────────────────────────────────────────────── */

static void add_css(GtkWidget *w, const char *cls) {
    gtk_style_context_add_class(gtk_widget_get_style_context(w), cls);
}

static void on_realize(GtkWidget *widget, gpointer user_data) {
    (void)user_data;
    GtkCssProvider *p = gtk_css_provider_new();
    gtk_css_provider_load_from_data(p, ETHERNET_CSS, -1, NULL);
    gtk_style_context_add_provider_for_screen(
        gtk_widget_get_screen(widget),
        GTK_STYLE_PROVIDER(p),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(p);
}

static GtkWidget *create_card(void) {
    GtkWidget *frame = gtk_frame_new(NULL);
    GtkWidget *box   = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);
    gtk_container_set_border_width(GTK_CONTAINER(frame), 0);
    add_css(frame, "eth-card");
    gtk_container_add(GTK_CONTAINER(frame), box);
    return frame;
}

static GtkWidget *group_label(const char *text) {
    GtkWidget *l = gtk_label_new(text);
    gtk_widget_set_halign(l, GTK_ALIGN_START);
    add_css(l, "eth-group-label");
    return l;
}

/* Build a two-column info row: label on left, value on right */
static GtkWidget *info_row(const char *label_text, const char *value_text) {
    GtkWidget *row   = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    GtkWidget *lbl   = gtk_label_new(label_text);
    GtkWidget *val   = gtk_label_new(value_text);

    gtk_widget_set_halign(lbl, GTK_ALIGN_START);
    gtk_widget_set_halign(val, GTK_ALIGN_END);
    gtk_widget_set_hexpand(lbl, TRUE);

    add_css(lbl, "eth-title");
    add_css(val, "eth-value");
    add_css(row, "eth-row");

    gtk_box_pack_start(GTK_BOX(row), lbl, TRUE, TRUE,  0);
    gtk_box_pack_start(GTK_BOX(row), val, FALSE, FALSE, 0);
    return row;
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Disconnect button callback
 * ═══════════════════════════════════════════════════════════════════════════ */

static void on_disconnect_btn_clicked(GtkButton *btn, gpointer user_data) {
    (void)user_data;
    DcEthernetPage *page     = g_object_get_data(G_OBJECT(btn), "page");
    char           *dev_path = g_object_get_data(G_OBJECT(btn), "dev-path");

    if (page && page->cb.on_disconnect && dev_path)
        page->cb.on_disconnect(dev_path, page->user_data);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Constructor
 * ═══════════════════════════════════════════════════════════════════════════ */

DcEthernetPage *dc_ethernet_page_new(void)
{
    DcEthernetPage *page = g_new0(DcEthernetPage, 1);

    GtkWidget *scroller = gtk_scrolled_window_new(NULL, NULL);
    GtkWidget *content  = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    add_css(scroller, "eth-scroll-hidden");
    add_css(content,  "eth-shell");

    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroller),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_overlay_scrolling(GTK_SCROLLED_WINDOW(scroller), TRUE);
    gtk_container_add(GTK_CONTAINER(scroller), content);
    g_signal_connect(scroller, "realize", G_CALLBACK(on_realize), NULL);

    /* Outer section label */
    GtkWidget *section_card = create_card();
    GtkWidget *section_box  = gtk_bin_get_child(GTK_BIN(section_card));

    gtk_box_pack_start(GTK_BOX(section_box),
                       group_label("ETHERNET ADAPTERS"), FALSE, FALSE, 0);

    /* Placeholder: actual adapter cards are inserted here by populate */
    page->adapters_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start(GTK_BOX(section_box), page->adapters_box, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(content), section_card, FALSE, FALSE, 0);

    page->root = scroller;
    return page;
}

void dc_ethernet_page_free(DcEthernetPage *page) {
    if (page) g_free(page);
}

GtkWidget *dc_ethernet_page_get_widget(DcEthernetPage *page) {
    return page ? page->root : NULL;
}

void dc_ethernet_page_set_callbacks(DcEthernetPage          *page,
                                    DcEthernetPageCallbacks *cb,
                                    gpointer                 user_data)
{
    if (!page || !cb) return;
    page->cb        = *cb;
    page->user_data = user_data;
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Populate / refresh adapter list
 * ═══════════════════════════════════════════════════════════════════════════ */

void dc_ethernet_page_populate_devices(DcEthernetPage *page,
                                       GList          *devices,
                                       GList          *ip_infos)
{
    if (!page || !page->adapters_box) return;

    /* Clear old content */
    GList *children = gtk_container_get_children(GTK_CONTAINER(page->adapters_box));
    for (GList *it = children; it; it = it->next)
        gtk_widget_destroy(GTK_WIDGET(it->data));
    g_list_free(children);

    if (!devices) {
        GtkWidget *empty_lbl =
            gtk_label_new("No Ethernet adapters detected.");
        add_css(empty_lbl, "eth-desc");
        gtk_widget_set_margin_top(empty_lbl,    16);
        gtk_widget_set_margin_bottom(empty_lbl, 16);
        gtk_widget_set_margin_start(empty_lbl,  18);
        gtk_box_pack_start(GTK_BOX(page->adapters_box), empty_lbl, FALSE, FALSE, 0);
        gtk_widget_show_all(page->adapters_box);
        return;
    }

    GList *ip_it = ip_infos;

    for (GList *dev_it = devices; dev_it; dev_it = dev_it->next, ip_it = ip_it ? ip_it->next : NULL)
    {
        DcNetworkDevice *dev  = dev_it->data;
        DcNetworkIpInfo *info = ip_it ? ip_it->data : NULL;

        gboolean connected = (dev->state == 100); /* NM_DEVICE_STATE_ACTIVATED */

        /* Per-adapter card */
        GtkWidget *adapter_card = create_card();
        GtkWidget *adapter_box  = gtk_bin_get_child(GTK_BIN(adapter_card));

        /* ── Header row: icon + interface name + status badge ─────────── */
        GtkWidget *header_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 14);
        add_css(header_row, "eth-row");

        GtkWidget *icon_lbl = gtk_label_new(connected ? "⬡" : "○");
        add_css(icon_lbl, "eth-icon");
        if (connected) add_css(icon_lbl, "eth-icon-connected");
        gtk_widget_set_valign(icon_lbl, GTK_ALIGN_CENTER);

        GtkWidget *name_vtxt  = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
        GtkWidget *name_title = gtk_label_new(dev->interface && *dev->interface
                                               ? dev->interface : "Ethernet");
        GtkWidget *status_badge =
            gtk_label_new(connected ? "● CONNECTED" : "○ DISCONNECTED");

        gtk_widget_set_halign(name_title,    GTK_ALIGN_START);
        gtk_widget_set_halign(status_badge,  GTK_ALIGN_START);
        gtk_widget_set_hexpand(name_vtxt,    TRUE);
        add_css(name_title,   "eth-title");
        add_css(status_badge, connected ? "eth-badge-connected" : "eth-badge-disconnected");

        gtk_box_pack_start(GTK_BOX(name_vtxt), name_title,   FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(name_vtxt), status_badge, FALSE, FALSE, 0);

        gtk_box_pack_start(GTK_BOX(header_row), icon_lbl,   FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(header_row), name_vtxt,  TRUE,  TRUE,  0);

        /* Speed pill (only when carrier detected) */
        if (dev->carrier && dev->speed > 0) {
            char speed_str[32];
            g_snprintf(speed_str, sizeof(speed_str),
                       "%u Mbps", dev->speed);
            GtkWidget *speed_pill = gtk_label_new(speed_str);
            add_css(speed_pill, "eth-speed-pill");
            gtk_widget_set_valign(speed_pill, GTK_ALIGN_CENTER);
            gtk_box_pack_start(GTK_BOX(header_row), speed_pill, FALSE, FALSE, 0);
        }

        gtk_box_pack_start(GTK_BOX(adapter_box), header_row, FALSE, FALSE, 0);

        /* ── Details section label ─────────────────────────────────────── */
        gtk_box_pack_start(GTK_BOX(adapter_box),
                           group_label("DETAILS"), FALSE, FALSE, 0);

        /* MAC address */
        gtk_box_pack_start(GTK_BOX(adapter_box),
                           info_row("MAC Address",
                                    dev->hw_address && *dev->hw_address
                                        ? dev->hw_address : "—"),
                           FALSE, FALSE, 0);

        /* Carrier status */
        gtk_box_pack_start(GTK_BOX(adapter_box),
                           info_row("Cable",
                                    dev->carrier ? "Plugged in" : "Unplugged"),
                           FALSE, FALSE, 0);

        /* IP information (if available) */
        if (info && info->address) {
            char addr_with_prefix[64];
            if (info->prefix > 0) {
                g_snprintf(addr_with_prefix, sizeof(addr_with_prefix),
                           "%s / %u", info->address, info->prefix);
            } else {
                g_snprintf(addr_with_prefix, sizeof(addr_with_prefix),
                           "%s", info->address);
            }
            gtk_box_pack_start(GTK_BOX(adapter_box),
                               info_row("IP Address", addr_with_prefix),
                               FALSE, FALSE, 0);

            if (info->gateway && *info->gateway &&
                g_strcmp0(info->gateway, "—") != 0)
            {
                gtk_box_pack_start(GTK_BOX(adapter_box),
                                   info_row("Gateway", info->gateway),
                                   FALSE, FALSE, 0);
            }
        } else {
            gtk_box_pack_start(GTK_BOX(adapter_box),
                               info_row("IP Address", "—"),
                               FALSE, FALSE, 0);
        }

        /* ── Action row ────────────────────────────────────────────────── */
        if (connected) {
            GtkWidget *action_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
            add_css(action_row, "eth-row");

            GtkWidget *spacer = gtk_label_new(NULL);
            gtk_widget_set_hexpand(spacer, TRUE);
            gtk_box_pack_start(GTK_BOX(action_row), spacer, TRUE, TRUE, 0);

            GtkWidget *disc_btn = gtk_button_new_with_label("Disconnect");
            add_css(disc_btn, "eth-btn");
            add_css(disc_btn, "eth-btn-danger");
            gtk_widget_set_valign(disc_btn, GTK_ALIGN_CENTER);
            g_object_set_data(G_OBJECT(disc_btn), "page", page);
            g_object_set_data_full(G_OBJECT(disc_btn), "dev-path",
                                   g_strdup(dev->object_path), g_free);
            g_signal_connect(disc_btn, "clicked",
                             G_CALLBACK(on_disconnect_btn_clicked), NULL);
            gtk_box_pack_start(GTK_BOX(action_row), disc_btn, FALSE, FALSE, 0);

            gtk_box_pack_start(GTK_BOX(adapter_box), action_row, FALSE, FALSE, 0);
        }

        gtk_box_pack_start(GTK_BOX(page->adapters_box), adapter_card,
                           FALSE, FALSE, 8);
    }

    gtk_widget_show_all(page->adapters_box);
}
