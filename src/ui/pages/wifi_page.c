#include "ui/pages/wifi_page.h"

#include <string.h>

/* ═══════════════════════════════════════════════════════════════════════════
 * CSS — same visual vocabulary as bluetooth_page / audio_page
 * ═══════════════════════════════════════════════════════════════════════════ */

static const char *WIFI_CSS =
    /* scrollbar hidden */
    ".wifi-scroll-hidden scrollbar { opacity:0; min-width:0; min-height:0; }"
    ".wifi-scroll-hidden scrollbar slider { min-width:0; min-height:0; }"

    /* shell padding */
    ".wifi-shell { padding: 40px 18px 48px; }"

    /* group label */
    ".wifi-group-label {"
    "  color: rgba(255,255,255,0.28);"
    "  font-size: 10px;"
    "  font-weight: 700;"
    "  letter-spacing: 0.09em;"
    "  padding: 10px 18px 2px;"
    "}"

    /* card */
    ".wifi-card {"
    "  background-color: rgba(14,14,14,0.72);"
    "  border: 1px solid rgba(255,255,255,0.10);"
    "  border-radius: 18px;"
    "  margin-bottom: 24px;"
    "  box-shadow: 0 4px 12px rgba(0,0,0,0.10);"
    "}"

    /* row */
    ".wifi-row {"
    "  padding: 12px 18px;"
    "  border-bottom: 1px solid rgba(255,255,255,0.05);"
    "}"
    ".wifi-row:last-child { border-bottom: none; }"

    /* text */
    ".wifi-title { color: rgba(255,255,255,0.92); font-size: 13px; font-weight: 600; }"
    ".wifi-desc  { color: rgba(255,255,255,0.55); font-size: 11px; margin-top: 3px; }"

    /* active AP highlight */
    ".wifi-row-active { background-color: rgba(33,150,243,0.10); }"
    ".wifi-active-badge {"
    "  color: #4FC3F7;"
    "  font-size: 10px;"
    "  font-weight: 700;"
    "  letter-spacing: 0.06em;"
    "}"

    /* signal bar labels */
    ".wifi-signal-strong { color: #66BB6A; font-size: 12px; font-weight: 700; }"
    ".wifi-signal-medium { color: #FFA726; font-size: 12px; font-weight: 700; }"
    ".wifi-signal-weak   { color: #EF5350; font-size: 12px; font-weight: 700; }"

    /* switch */
    ".wifi-switch { border-radius: 14px; }"
    ".wifi-switch:checked { background-color: #2196F3; }"

    /* buttons */
    ".wifi-btn {"
    "  background-color: rgba(255,255,255,0.10);"
    "  color: white;"
    "  border: 1px solid rgba(255,255,255,0.20);"
    "  border-radius: 8px;"
    "  padding: 5px 12px;"
    "  font-weight: 600;"
    "}"
    ".wifi-btn:hover { background-color: rgba(255,255,255,0.20); }"

    ".wifi-btn-primary { background-color: #2196F3; border-color: #1976D2; }"
    ".wifi-btn-primary:hover { background-color: #1976D2; }"

    ".wifi-btn-danger {"
    "  background-color: rgba(244,67,54,0.15);"
    "  color: #EF9A9A;"
    "  border-color: rgba(244,67,54,0.30);"
    "}"
    ".wifi-btn-danger:hover { background-color: rgba(244,67,54,0.30); }"

    /* scan spinner label */
    ".wifi-scan-label { color: rgba(255,255,255,0.45); font-size: 11px; }"

    /* password dialog */
    ".wifi-pwd-entry {"
    "  background-color: rgba(255,255,255,0.08);"
    "  color: rgba(255,255,255,0.90);"
    "  border: 1px solid rgba(255,255,255,0.18);"
    "  border-radius: 8px;"
    "  padding: 6px 10px;"
    "  font-size: 13px;"
    "}"
    ;

/* ═══════════════════════════════════════════════════════════════════════════
 * Internal struct
 * ═══════════════════════════════════════════════════════════════════════════ */

struct _DcWifiPage {
    GtkWidget *root;
    GtkWidget *wifi_switch;
    GtkWidget *scan_btn;
    GtkWidget *scan_label;
    GtkWidget *ap_list_box;   /* GtkBox holding AP rows */

    DcWifiPageCallbacks  cb;
    gpointer             user_data;
};

/* ── helpers ─────────────────────────────────────────────────────────────── */

static void add_css(GtkWidget *w, const char *cls) {
    gtk_style_context_add_class(gtk_widget_get_style_context(w), cls);
}

static void on_realize(GtkWidget *widget, gpointer user_data) {
    (void)user_data;
    GtkCssProvider *p = gtk_css_provider_new();
    gtk_css_provider_load_from_data(p, WIFI_CSS, -1, NULL);
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
    add_css(frame, "wifi-card");
    gtk_container_add(GTK_CONTAINER(frame), box);
    return frame;
}

static GtkWidget *group_label(const char *text) {
    GtkWidget *l = gtk_label_new(text);
    gtk_widget_set_halign(l, GTK_ALIGN_START);
    add_css(l, "wifi-group-label");
    return l;
}

/* Signal-strength helper: returns "▂▄▆█", "▂▄▆░", "▂▄░░", "▂░░░" */
static const char *signal_bars(guint8 s) {
    if (s >= 75) return "▂▄▆█";
    if (s >= 50) return "▂▄▆░";
    if (s >= 25) return "▂▄░░";
    return "▂░░░";
}

static const char *signal_css_class(guint8 s) {
    if (s >= 60) return "wifi-signal-strong";
    if (s >= 35) return "wifi-signal-medium";
    return "wifi-signal-weak";
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Password dialog
 * ═══════════════════════════════════════════════════════════════════════════ */

typedef struct {
    DcWifiPage *page;
    char       *ap_path;
    char       *ssid;
} ConnectCtx;

static void connect_ctx_free(ConnectCtx *ctx) {
    g_free(ctx->ap_path);
    g_free(ctx->ssid);
    g_free(ctx);
}

static void on_pwd_dialog_response(GtkDialog *dialog,
                                   gint       response,
                                   gpointer   user_data)
{
    ConnectCtx *ctx  = user_data;
    DcWifiPage *page = ctx->page;

    if (response == GTK_RESPONSE_OK && page->cb.on_connect) {
        GtkWidget  *entry = g_object_get_data(G_OBJECT(dialog), "pwd-entry");
        const char *pwd   = gtk_entry_get_text(GTK_ENTRY(entry));
        page->cb.on_connect(ctx->ap_path, ctx->ssid,
                            (pwd && *pwd) ? pwd : NULL,
                            page->user_data);
    }

    connect_ctx_free(ctx);
    gtk_widget_destroy(GTK_WIDGET(dialog));
}

static void show_password_dialog(DcWifiPage *page,
                                 const char *ap_path,
                                 const char *ssid)
{
    GtkWidget *toplevel = gtk_widget_get_toplevel(page->root);
    GtkWindow *parent   = GTK_IS_WINDOW(toplevel) ? GTK_WINDOW(toplevel) : NULL;

    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "Connect to Wi-Fi",
        parent,
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Connect", GTK_RESPONSE_OK,
        NULL);

    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);
    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_set_border_width(GTK_CONTAINER(content), 20);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_box_pack_start(GTK_BOX(content), vbox, TRUE, TRUE, 0);

    char prompt[256];
    g_snprintf(prompt, sizeof(prompt), "Enter password for \"%s\":", ssid ? ssid : "");
    GtkWidget *lbl = gtk_label_new(prompt);
    gtk_widget_set_halign(lbl, GTK_ALIGN_START);
    add_css(lbl, "wifi-title");
    gtk_box_pack_start(GTK_BOX(vbox), lbl, FALSE, FALSE, 0);

    GtkWidget *entry = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(entry), FALSE);
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "Password");
    gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE);
    add_css(entry, "wifi-pwd-entry");
    gtk_box_pack_start(GTK_BOX(vbox), entry, FALSE, FALSE, 0);

    g_object_set_data(G_OBJECT(dialog), "pwd-entry", entry);

    ConnectCtx *ctx = g_new0(ConnectCtx, 1);
    ctx->page    = page;
    ctx->ap_path = g_strdup(ap_path);
    ctx->ssid    = g_strdup(ssid);

    g_signal_connect(dialog, "response",
                     G_CALLBACK(on_pwd_dialog_response), ctx);

    gtk_widget_show_all(dialog);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * AP row button callbacks
 * ═══════════════════════════════════════════════════════════════════════════ */

static void on_connect_btn_clicked(GtkButton *btn, gpointer user_data)
{
    DcWifiPage *page     = g_object_get_data(G_OBJECT(btn), "page");
    char       *ap_path  = g_object_get_data(G_OBJECT(btn), "ap-path");
    char       *ssid     = g_object_get_data(G_OBJECT(btn), "ssid");
    gboolean    secured  = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(btn), "secured"));
    gboolean    has_prof = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(btn), "has-profile"));

    (void)user_data;

    if (!page || !ap_path || !ssid) return;

    /* If NM already has a saved profile, or the network is open,
     * connect immediately — no password dialog. */
    if (has_prof || !secured) {
        if (page->cb.on_connect)
            page->cb.on_connect(ap_path, ssid, NULL, page->user_data);
    } else {
        show_password_dialog(page, ap_path, ssid);
    }
}


static void on_disconnect_btn_clicked(GtkButton *btn, gpointer user_data)
{
    DcWifiPage *page = g_object_get_data(G_OBJECT(btn), "page");
    (void)user_data;
    if (page && page->cb.on_disconnect) {
        page->cb.on_disconnect(page->user_data);
    }
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Top-bar button callbacks
 * ═══════════════════════════════════════════════════════════════════════════ */

static void on_scan_btn_clicked(GtkButton *btn, gpointer user_data)
{
    (void)btn;
    DcWifiPage *page = user_data;
    if (page->cb.on_scan) page->cb.on_scan(page->user_data);
}

static void on_wifi_switch_active(GtkSwitch *sw, GParamSpec *pspec, gpointer user_data)
{
    (void)pspec;
    DcWifiPage *page    = user_data;
    gboolean    enabled = gtk_switch_get_active(sw);
    if (page->cb.on_toggle_wifi) page->cb.on_toggle_wifi(enabled, page->user_data);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Constructor
 * ═══════════════════════════════════════════════════════════════════════════ */

DcWifiPage *dc_wifi_page_new(void)
{
    DcWifiPage *page = g_new0(DcWifiPage, 1);

    GtkWidget *scroller = gtk_scrolled_window_new(NULL, NULL);
    GtkWidget *content  = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    add_css(scroller, "wifi-scroll-hidden");
    add_css(content,  "wifi-shell");

    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroller),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_overlay_scrolling(GTK_SCROLLED_WINDOW(scroller), TRUE);
    gtk_container_add(GTK_CONTAINER(scroller), content);
    g_signal_connect(scroller, "realize", G_CALLBACK(on_realize), NULL);

    /* ── Control card (switch + scan button) ───────────────────────────── */
    GtkWidget *ctrl_card = create_card();
    GtkWidget *ctrl_box  = gtk_bin_get_child(GTK_BIN(ctrl_card));

    gtk_box_pack_start(GTK_BOX(ctrl_box), group_label("WI-FI"), FALSE, FALSE, 0);

    /* Switch row */
    GtkWidget *sw_row    = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    GtkWidget *sw_vtxt   = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    GtkWidget *sw_title  = gtk_label_new("Wi-Fi");
    GtkWidget *sw_desc   = gtk_label_new("Enable or disable wireless networking");
    page->wifi_switch    = gtk_switch_new();

    gtk_widget_set_halign(sw_title, GTK_ALIGN_START);
    gtk_widget_set_halign(sw_desc,  GTK_ALIGN_START);
    gtk_widget_set_hexpand(sw_vtxt, TRUE);
    gtk_widget_set_valign(page->wifi_switch, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(sw_vtxt, GTK_ALIGN_CENTER);

    add_css(sw_title,        "wifi-title");
    add_css(sw_desc,         "wifi-desc");
    add_css(sw_row,          "wifi-row");
    add_css(page->wifi_switch, "wifi-switch");

    gtk_box_pack_start(GTK_BOX(sw_vtxt), sw_title, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(sw_vtxt), sw_desc,  FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(sw_row),  sw_vtxt,  TRUE,  TRUE,  0);
    gtk_box_pack_start(GTK_BOX(sw_row),  page->wifi_switch, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(ctrl_box), sw_row, FALSE, FALSE, 0);

    /* Scan row */
    GtkWidget *scan_row   = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    GtkWidget *scan_vtxt  = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    GtkWidget *scan_title = gtk_label_new("Scan for Networks");
    page->scan_label      = gtk_label_new("Refresh the list of available access points");
    page->scan_btn        = gtk_button_new_with_label("Scan");

    gtk_widget_set_halign(scan_title,    GTK_ALIGN_START);
    gtk_widget_set_halign(page->scan_label, GTK_ALIGN_START);
    gtk_widget_set_hexpand(scan_vtxt,    TRUE);
    gtk_widget_set_valign(page->scan_btn, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(scan_vtxt,     GTK_ALIGN_CENTER);

    add_css(scan_title,      "wifi-title");
    add_css(page->scan_label, "wifi-desc");
    add_css(scan_row,        "wifi-row");
    add_css(page->scan_btn,  "wifi-btn");

    gtk_box_pack_start(GTK_BOX(scan_vtxt), scan_title,   FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(scan_vtxt), page->scan_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(scan_row),  scan_vtxt,    TRUE,  TRUE,  0);
    gtk_box_pack_start(GTK_BOX(scan_row),  page->scan_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(ctrl_box), scan_row, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(content), ctrl_card, FALSE, FALSE, 0);

    /* ── Networks card ─────────────────────────────────────────────────── */
    GtkWidget *net_card = create_card();
    GtkWidget *net_box  = gtk_bin_get_child(GTK_BIN(net_card));

    gtk_box_pack_start(GTK_BOX(net_box), group_label("AVAILABLE NETWORKS"), FALSE, FALSE, 0);

    page->ap_list_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start(GTK_BOX(net_box), page->ap_list_box, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(content), net_card, FALSE, FALSE, 0);

    /* wire up top-bar signals */
    g_signal_connect(page->scan_btn,     "clicked",       G_CALLBACK(on_scan_btn_clicked),  page);
    g_signal_connect(page->wifi_switch,  "notify::active",G_CALLBACK(on_wifi_switch_active), page);

    page->root = scroller;
    return page;
}

void dc_wifi_page_free(DcWifiPage *page) {
    if (page) g_free(page);
}

GtkWidget *dc_wifi_page_get_widget(DcWifiPage *page) {
    return page ? page->root : NULL;
}

GtkWidget *dc_wifi_page_get_wifi_switch(DcWifiPage *page) {
    return page ? page->wifi_switch : NULL;
}

void dc_wifi_page_set_wifi_enabled(DcWifiPage *page, gboolean enabled) {
    if (!page) return;
    g_signal_handlers_block_by_func(page->wifi_switch,
                                    G_CALLBACK(on_wifi_switch_active), page);
    gtk_switch_set_active(GTK_SWITCH(page->wifi_switch), enabled);
    g_signal_handlers_unblock_by_func(page->wifi_switch,
                                      G_CALLBACK(on_wifi_switch_active), page);

    /* Show/hide scan button based on Wi-Fi enabled state */
    gtk_widget_set_sensitive(page->scan_btn, enabled);
}

void dc_wifi_page_set_scanning(DcWifiPage *page, gboolean scanning) {
    if (!page) return;
    gtk_label_set_text(GTK_LABEL(page->scan_label),
                       scanning ? "Scanning…"
                                : "Refresh the list of available access points");
    gtk_widget_set_sensitive(page->scan_btn, !scanning);
}

void dc_wifi_page_set_callbacks(DcWifiPage          *page,
                                DcWifiPageCallbacks *cb,
                                gpointer             user_data)
{
    if (!page || !cb) return;
    page->cb        = *cb;
    page->user_data = user_data;
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Populate AP list
 * ═══════════════════════════════════════════════════════════════════════════ */

void dc_wifi_page_populate_aps(DcWifiPage *page, GList *aps)
{
    if (!page || !page->ap_list_box) return;

    /* Clear existing rows */
    GList *children = gtk_container_get_children(GTK_CONTAINER(page->ap_list_box));
    for (GList *it = children; it; it = it->next)
        gtk_widget_destroy(GTK_WIDGET(it->data));
    g_list_free(children);

    if (!aps) {
        GtkWidget *empty = gtk_label_new("No networks found. Press Scan to refresh.");
        add_css(empty, "wifi-desc");
        gtk_widget_set_margin_top(empty,    16);
        gtk_widget_set_margin_bottom(empty, 16);
        gtk_widget_set_margin_start(empty,  18);
        gtk_box_pack_start(GTK_BOX(page->ap_list_box), empty, FALSE, FALSE, 0);
        gtk_widget_show_all(page->ap_list_box);
        return;
    }

    for (GList *it = aps; it; it = it->next) {
        DcAccessPoint *ap = it->data;

        GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
        add_css(row, "wifi-row");
        if (ap->active) add_css(row, "wifi-row-active");

        /* Left: SSID + status */
        GtkWidget *vtxt      = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
        GtkWidget *name_lbl  = gtk_label_new(ap->ssid ? ap->ssid : "Hidden Network");
        gtk_widget_set_halign(name_lbl, GTK_ALIGN_START);
        add_css(name_lbl, "wifi-title");
        gtk_box_pack_start(GTK_BOX(vtxt), name_lbl, FALSE, FALSE, 0);

        if (ap->active) {
            GtkWidget *badge = gtk_label_new("● CONNECTED");
            gtk_widget_set_halign(badge, GTK_ALIGN_START);
            add_css(badge, "wifi-active-badge");
            gtk_box_pack_start(GTK_BOX(vtxt), badge, FALSE, FALSE, 0);
        } else {
            /* sub-label: secured indicator */
            const char *sec_text = ap->secured ? "🔒  Secured" : "Open";
            GtkWidget *sub = gtk_label_new(sec_text);
            gtk_widget_set_halign(sub, GTK_ALIGN_START);
            add_css(sub, "wifi-desc");
            gtk_box_pack_start(GTK_BOX(vtxt), sub, FALSE, FALSE, 0);
        }

        gtk_widget_set_valign(vtxt,  GTK_ALIGN_CENTER);
        gtk_widget_set_hexpand(vtxt, TRUE);
        gtk_box_pack_start(GTK_BOX(row), vtxt, TRUE, TRUE, 0);

        /* Signal strength bars */
        GtkWidget *sig_lbl = gtk_label_new(signal_bars(ap->strength));
        add_css(sig_lbl, signal_css_class(ap->strength));
        gtk_widget_set_valign(sig_lbl, GTK_ALIGN_CENTER);
        gtk_box_pack_start(GTK_BOX(row), sig_lbl, FALSE, FALSE, 0);

        /* Connect / Disconnect button */
        if (ap->active) {
            GtkWidget *disc_btn = gtk_button_new_with_label("Disconnect");
            add_css(disc_btn, "wifi-btn");
            add_css(disc_btn, "wifi-btn-danger");
            g_object_set_data(G_OBJECT(disc_btn), "page", page);
            gtk_widget_set_valign(disc_btn, GTK_ALIGN_CENTER);
            g_signal_connect(disc_btn, "clicked",
                             G_CALLBACK(on_disconnect_btn_clicked), NULL);
            gtk_box_pack_start(GTK_BOX(row), disc_btn, FALSE, FALSE, 0);
        } else {
            GtkWidget *conn_btn = gtk_button_new_with_label("Connect");
            add_css(conn_btn, "wifi-btn");
            add_css(conn_btn, "wifi-btn-primary");
            g_object_set_data(G_OBJECT(conn_btn), "page",    page);
            g_object_set_data_full(G_OBJECT(conn_btn), "ap-path",
                                   g_strdup(ap->object_path), g_free);
            g_object_set_data_full(G_OBJECT(conn_btn), "ssid",
                                   g_strdup(ap->ssid ? ap->ssid : ""), g_free);
            g_object_set_data(G_OBJECT(conn_btn), "secured",
                              GINT_TO_POINTER((gint)ap->secured));
            g_object_set_data(G_OBJECT(conn_btn), "has-profile",
                              GINT_TO_POINTER((gint)ap->has_profile));
            gtk_widget_set_valign(conn_btn, GTK_ALIGN_CENTER);
            g_signal_connect(conn_btn, "clicked",
                             G_CALLBACK(on_connect_btn_clicked), NULL);
            gtk_box_pack_start(GTK_BOX(row), conn_btn, FALSE, FALSE, 0);
        }

        gtk_box_pack_start(GTK_BOX(page->ap_list_box), row, FALSE, FALSE, 0);
    }

    gtk_widget_show_all(page->ap_list_box);
}
