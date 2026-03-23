#include "ui/pages/keyboard_page.h"

typedef struct {
    const char *code;
    const char *name_en;
    const char *name_local;
} LayoutEntry;

static const LayoutEntry AVAILABLE_LAYOUTS[] = {
    { "us",  "English (US)",          "English (US)"  },
    { "gb",  "English (UK)",          "English (UK)"  },
    { "ara", "Arabic",                "العربية"       },
    { "ar",  "Arabic (ar)",           "العربية"       },
    { "fr",  "French",                "Français"      },
    { "de",  "German",                "Deutsch"       },
    { "es",  "Spanish",               "Español"       },
    { "ru",  "Russian",               "Русский"       },
    { "tr",  "Turkish",               "Türkçe"        },
    { "it",  "Italian",               "Italiano"      },
    { "pt",  "Portuguese",            "Português"     },
    { "ja",  "Japanese",              "日本語"        },
    { "zh",  "Chinese (Simplified)",  "中文(简体)"    },
    { "ko",  "Korean",                "한국어"        },
    { "pl",  "Polish",                "Polski"        },
    { "nl",  "Dutch",                 "Nederlands"    },
    { "sv",  "Swedish",               "Svenska"       },
    { "no",  "Norwegian",             "Norsk"         },
    { "da",  "Danish",                "Dansk"         },
    { "fi",  "Finnish",               "Suomi"         },
    { "he",  "Hebrew",                "עברית"         },
    { "fa",  "Persian",               "فارسی"         },
    { "ur",  "Urdu",                  "اردو"          },
    { "hi",  "Hindi",                 "हिन्दी"        },
    { "el",  "Greek",                 "Ελληνικά"      },
    { "cs",  "Czech",                 "Čeština"       },
    { "hu",  "Hungarian",             "Magyar"        },
    { "ro",  "Romanian",              "Română"        },
    { "uk",  "Ukrainian",             "Українська"    },
    { "th",  "Thai",                  "ไทย"           },
    { "vi",  "Vietnamese",            "Tiếng Việt"    },
    { NULL,  NULL,                    NULL            }
};

const char *dc_keyboard_page_lookup_layout_name(const char *code) {
    for (int i = 0; AVAILABLE_LAYOUTS[i].code != NULL; i++) {
        if (g_strcmp0(AVAILABLE_LAYOUTS[i].code, code) == 0) {
            return AVAILABLE_LAYOUTS[i].name_local;
        }
    }
    return code; // Fallback to code if not found
}

struct _DcKeyboardPage {
    GtkWidget *root;

    /* Active Layouts */
    GtkWidget *layouts_listbox;

    /* Add Layout */
    GtkWidget *add_combo;
    GtkWidget *add_button;

    /* Hardware Settings */
    GtkWidget *model_combo;
    GtkWidget *options_entry;
    GtkWidget *apply_options_button;
};

/* ------------------------------------------------------------------ */
/*  CSS (Dark Glassmorphism)                                          */
/* ------------------------------------------------------------------ */
static const char *KEYBOARD_CSS =
    ".comp-scroll-hidden scrollbar { opacity: 0; min-width: 0; min-height: 0; }"
    ".comp-scroll-hidden scrollbar slider { min-width: 0; min-height: 0; }"
    ".comp-shell { padding: 20px 18px 48px; }"
    ".comp-group-label { color: rgba(255,255,255,0.28); font-size: 10px; font-weight: 700; letter-spacing: 0.09em; padding: 10px 18px 2px; }"
    ".comp-row { padding: 10px 18px; border-bottom: 1px solid rgba(255,255,255,0.05); }"
    ".comp-setting-title { color: rgba(255,255,255,0.92); font-size: 13px; font-weight: 600; }"
    ".comp-setting-desc { color: rgba(255,255,255,0.45); font-size: 11.5px; }"
    ".comp-card { background-color: rgba(14,14,14,0.72); border: 1px solid rgba(255,255,255,0.10); border-radius: 18px; }"
    "combobox button { background-color: rgba(255,255,255,0.08); background-image: none; border: 1px solid rgba(255,255,255,0.12); border-radius: 10px; padding: 6px 10px; color: rgba(255,255,255,0.90); font-size: 12px; }"
    "combobox button:hover { background-color: rgba(255,255,255,0.14); }"
    "entry { background-color: rgba(255,255,255,0.08); border: 1px solid rgba(255,255,255,0.12); border-radius: 10px; padding: 6px 10px; color: rgba(255,255,255,0.95); font-size: 12px; min-height: 32px; }"
    ".comp-btn { background-color: rgba(255,255,255,0.10); background-image: none; border: 1px solid rgba(255,255,255,0.12); border-radius: 12px; padding: 8px 16px; color: rgba(255,255,255,0.92); font-size: 13px; font-weight: 500; }"
    ".comp-btn:hover { background-color: rgba(255,255,255,0.16); }"
    ".comp-btn-danger { background-color: rgba(230,50,50,0.20); border-color: rgba(230,50,50,0.30); color: rgba(255,200,200,0.95); }"
    ".comp-btn-danger:hover { background-color: rgba(230,50,50,0.35); }";

static void on_realize(GtkWidget *widget, gpointer user_data) {
    (void)user_data;
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider, KEYBOARD_CSS, -1, NULL);
    gtk_style_context_add_provider_for_screen(
        gtk_widget_get_screen(widget),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
    );
    g_object_unref(provider);
}

/* ------------------------------------------------------------------ */
/*  Helper Functions                                                  */
/* ------------------------------------------------------------------ */
static void add_css_class(GtkWidget *widget, const char *class_name) {
    gtk_style_context_add_class(gtk_widget_get_style_context(widget), class_name);
}

static GtkWidget *create_group_label(const char *text) {
    GtkWidget *label = gtk_label_new(text);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    add_css_class(label, "comp-group-label");
    return label;
}

static GtkWidget *create_setting_row_impl(const char *title, const char *description, GtkWidget *control) {
    GtkWidget *row      = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    GtkWidget *text_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 3);
    GtkWidget *t_label  = gtk_label_new(title);
    GtkWidget *d_label  = gtk_label_new(description);

    gtk_widget_set_halign(t_label,  GTK_ALIGN_START);
    gtk_widget_set_halign(d_label,  GTK_ALIGN_START);
    gtk_widget_set_hexpand(text_box, TRUE);
    gtk_label_set_line_wrap(GTK_LABEL(d_label), TRUE);
    if (control) gtk_widget_set_valign(control,  GTK_ALIGN_CENTER);

    add_css_class(t_label, "comp-setting-title");
    add_css_class(d_label, "comp-setting-desc");
    add_css_class(row,     "comp-row");

    gtk_box_pack_start(GTK_BOX(text_box), t_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(text_box), d_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(row), text_box, TRUE,  TRUE,  0);
    if (control) gtk_box_pack_end(GTK_BOX(row), control,  FALSE, FALSE, 0);
    
    return row;
}

static GtkWidget *create_card(void) {
    GtkWidget *frame      = gtk_frame_new(NULL);
    GtkWidget *outer_box  = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);
    gtk_container_set_border_width(GTK_CONTAINER(frame), 0);
    add_css_class(frame, "comp-card");

    gtk_container_add(GTK_CONTAINER(frame), outer_box);
    return frame;
}

/* ------------------------------------------------------------------ */
/*  Main Builder                                                      */
/* ------------------------------------------------------------------ */
DcKeyboardPage *dc_keyboard_page_new(void) {
    DcKeyboardPage *page = g_new0(DcKeyboardPage, 1);

    GtkWidget *content  = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);
    GtkWidget *scroller = gtk_scrolled_window_new(NULL, NULL);
    add_css_class(content, "comp-shell");
    add_css_class(scroller, "comp-scroll-hidden");

    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroller),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_overlay_scrolling(GTK_SCROLLED_WINDOW(scroller), TRUE);
    gtk_container_add(GTK_CONTAINER(scroller), content);
    g_signal_connect(scroller, "realize", G_CALLBACK(on_realize), NULL);

    /* ================================================================ */
    /*  Card 1 & 2: Layouts                                             */
    /* ================================================================ */
    GtkWidget *layouts_card = create_card();
    GtkWidget *layouts_box = gtk_bin_get_child(GTK_BIN(layouts_card));

    page->layouts_listbox = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(page->layouts_listbox), GTK_SELECTION_NONE);
    add_css_class(page->layouts_listbox, "comp-card"); 
    // We make the listbox look borderless inside the card
    gtk_widget_set_margin_bottom(page->layouts_listbox, 6);

    page->add_combo = gtk_combo_box_text_new();
    for (int i = 0; AVAILABLE_LAYOUTS[i].code != NULL; i++) {
        char display_name[128];
        snprintf(display_name, sizeof(display_name), "%s (%s)", AVAILABLE_LAYOUTS[i].name_local, AVAILABLE_LAYOUTS[i].code);
        gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(page->add_combo), AVAILABLE_LAYOUTS[i].code, display_name);
    }

    page->add_button = gtk_button_new_with_label("Add");
    add_css_class(page->add_button, "comp-btn");

    GtkWidget *add_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_box_pack_start(GTK_BOX(add_box), page->add_combo, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(add_box), page->add_button, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(layouts_box), create_group_label("ACTIVE LAYOUTS"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(layouts_box), page->layouts_listbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(layouts_box), create_setting_row_impl("Add New Layout", "Select a layout from the list to add it to your system", add_box), FALSE, FALSE, 0);

    /* ================================================================ */
    /*  Card 3: Hardware Settings                                       */
    /* ================================================================ */
    GtkWidget *hw_card = create_card();
    GtkWidget *hw_box = gtk_bin_get_child(GTK_BIN(hw_card));

    page->model_combo = gtk_combo_box_text_new();
    const char *models[] = {"pc104", "pc105", "macintosh", "applealu_ansi", NULL};
    for (int i = 0; models[i]; i++) {
        gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(page->model_combo), models[i], models[i]);
    }

    page->options_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(page->options_entry), "e.g., grp:alt_shift_toggle");
    
    page->apply_options_button = gtk_button_new_with_label("Apply");
    add_css_class(page->apply_options_button, "comp-btn");

    GtkWidget *options_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_box_pack_start(GTK_BOX(options_box), page->options_entry, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(options_box), page->apply_options_button, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(hw_box), create_group_label("ADVANCED HARDWARE"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hw_box), create_setting_row_impl("Keyboard Model", "Set the underlying X11 keyboard hardware model", page->model_combo), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hw_box), create_setting_row_impl("X11 Options", "Custom layout switching shortcut and other options", options_box), FALSE, FALSE, 0);


    /* Add cards to main box */
    gtk_box_pack_start(GTK_BOX(content), layouts_card, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(content), hw_card, FALSE, FALSE, 0);

    page->root = scroller;
    return page;
}

void dc_keyboard_page_free(DcKeyboardPage *page) {
    g_free(page);
}

void dc_keyboard_page_clear_layouts(DcKeyboardPage *page) {
    if (!page->layouts_listbox) return;
    GList *children, *iter;
    children = gtk_container_get_children(GTK_CONTAINER(page->layouts_listbox));
    for (iter = children; iter != NULL; iter = g_list_next(iter)) {
        gtk_widget_destroy(GTK_WIDGET(iter->data));
    }
    g_list_free(children);
}

void dc_keyboard_page_add_layout_row(DcKeyboardPage *page, const char *code, const char *name, GCallback on_remove_clicked, gpointer user_data) {
    GtkWidget *row_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    GtkWidget *text_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    
    GtkWidget *title_label = gtk_label_new(name);
    gtk_widget_set_halign(title_label, GTK_ALIGN_START);
    add_css_class(title_label, "comp-setting-title");
    
    char desc_buf[128];
    snprintf(desc_buf, sizeof(desc_buf), "Code: %s", code);
    GtkWidget *desc_label = gtk_label_new(desc_buf);
    gtk_widget_set_halign(desc_label, GTK_ALIGN_START);
    add_css_class(desc_label, "comp-setting-desc");

    gtk_box_pack_start(GTK_BOX(text_box), title_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(text_box), desc_label, FALSE, FALSE, 0);

    GtkWidget *remove_btn = gtk_button_new_with_label("Remove");
    add_css_class(remove_btn, "comp-btn");
    add_css_class(remove_btn, "comp-btn-danger");
    
    // Attach code as object data so the callback knows which layout to remove
    g_object_set_data_full(G_OBJECT(remove_btn), "layout_code", g_strdup(code), g_free);
    g_signal_connect(remove_btn, "clicked", on_remove_clicked, user_data);

    gtk_box_pack_start(GTK_BOX(row_box), text_box, TRUE, TRUE, 0);
    gtk_box_pack_end(GTK_BOX(row_box), remove_btn, FALSE, FALSE, 0);
    add_css_class(row_box, "comp-row");

    GtkWidget *list_row = gtk_list_box_row_new();
    gtk_container_add(GTK_CONTAINER(list_row), row_box);
    gtk_widget_show_all(list_row);

    gtk_list_box_insert(GTK_LIST_BOX(page->layouts_listbox), list_row, -1);
}

GtkWidget *dc_keyboard_page_get_widget(DcKeyboardPage *page) { return page->root; }
GtkWidget *dc_keyboard_page_get_layouts_listbox(DcKeyboardPage *page) { return page->layouts_listbox; }
GtkWidget *dc_keyboard_page_get_add_combo(DcKeyboardPage *page) { return page->add_combo; }
GtkWidget *dc_keyboard_page_get_add_button(DcKeyboardPage *page) { return page->add_button; }
const char *dc_keyboard_page_get_selected_add_layout(DcKeyboardPage *page) { return gtk_combo_box_get_active_id(GTK_COMBO_BOX(page->add_combo)); }
GtkWidget *dc_keyboard_page_get_model_combo(DcKeyboardPage *page) { return page->model_combo; }
GtkWidget *dc_keyboard_page_get_options_entry(DcKeyboardPage *page) { return page->options_entry; }
GtkWidget *dc_keyboard_page_get_apply_options_button(DcKeyboardPage *page) { return page->apply_options_button; }
