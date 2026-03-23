#ifndef DC_AUDIO_PAGE_H
#define DC_AUDIO_PAGE_H

#include <gtk/gtk.h>

typedef struct _DcAudioPage DcAudioPage;

DcAudioPage *dc_audio_page_new(void);
void dc_audio_page_free(DcAudioPage *page);
GtkWidget *dc_audio_page_get_widget(DcAudioPage *page);

GtkWidget *dc_audio_page_get_output_combo(DcAudioPage *page);
GtkWidget *dc_audio_page_get_output_volume_scale(DcAudioPage *page);
GtkWidget *dc_audio_page_get_output_mute_switch(DcAudioPage *page);
GtkWidget *dc_audio_page_get_input_combo(DcAudioPage *page);
GtkWidget *dc_audio_page_get_input_volume_scale(DcAudioPage *page);
GtkWidget *dc_audio_page_get_input_mute_switch(DcAudioPage *page);
GtkWidget *dc_audio_page_get_overamplification_switch(DcAudioPage *page);
GtkWidget *dc_audio_page_get_apps_box(DcAudioPage *page);

#endif
