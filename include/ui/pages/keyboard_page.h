#ifndef DC_KEYBOARD_PAGE_H
#define DC_KEYBOARD_PAGE_H

#include <gtk/gtk.h>

typedef struct _DcKeyboardPage DcKeyboardPage;

DcKeyboardPage *dc_keyboard_page_new(void);
void dc_keyboard_page_free(DcKeyboardPage *page);

GtkWidget *dc_keyboard_page_get_widget(DcKeyboardPage *page);

/* Active Layouts ListBox */
GtkWidget *dc_keyboard_page_get_layouts_listbox(DcKeyboardPage *page);
void dc_keyboard_page_add_layout_row(DcKeyboardPage *page, const char *code, const char *name, GCallback on_remove_clicked, gpointer user_data);
void dc_keyboard_page_clear_layouts(DcKeyboardPage *page);

/* Add Layout */
GtkWidget *dc_keyboard_page_get_add_combo(DcKeyboardPage *page);
GtkWidget *dc_keyboard_page_get_add_button(DcKeyboardPage *page);
const char *dc_keyboard_page_get_selected_add_layout(DcKeyboardPage *page);

/* Hardware Settings */
GtkWidget *dc_keyboard_page_get_model_combo(DcKeyboardPage *page);
GtkWidget *dc_keyboard_page_get_options_entry(DcKeyboardPage *page);
GtkWidget *dc_keyboard_page_get_apply_options_button(DcKeyboardPage *page);

const char *dc_keyboard_page_lookup_layout_name(const char *code);

#endif // DC_KEYBOARD_PAGE_H
