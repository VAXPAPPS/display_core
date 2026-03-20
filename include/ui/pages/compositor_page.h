#ifndef DC_COMPOSITOR_PAGE_H
#define DC_COMPOSITOR_PAGE_H

#include <gtk/gtk.h>

typedef struct _DcCompositorPage DcCompositorPage;

DcCompositorPage *dc_compositor_page_new(void);
void dc_compositor_page_free(DcCompositorPage *page);
GtkWidget *dc_compositor_page_get_widget(DcCompositorPage *page);

GtkWidget *dc_compositor_page_get_shadow_switch(DcCompositorPage *page);
GtkWidget *dc_compositor_page_get_shadow_radius_scale(DcCompositorPage *page);
GtkWidget *dc_compositor_page_get_shadow_opacity_scale(DcCompositorPage *page);
GtkWidget *dc_compositor_page_get_shadow_red_scale(DcCompositorPage *page);
GtkWidget *dc_compositor_page_get_shadow_green_scale(DcCompositorPage *page);
GtkWidget *dc_compositor_page_get_shadow_blue_scale(DcCompositorPage *page);
GtkWidget *dc_compositor_page_get_fading_switch(DcCompositorPage *page);
GtkWidget *dc_compositor_page_get_active_opacity_scale(DcCompositorPage *page);
GtkWidget *dc_compositor_page_get_inactive_opacity_scale(DcCompositorPage *page);
GtkWidget *dc_compositor_page_get_corner_radius_scale(DcCompositorPage *page);
GtkWidget *dc_compositor_page_get_detect_rounded_switch(DcCompositorPage *page);
GtkWidget *dc_compositor_page_get_blur_method_combo(DcCompositorPage *page);
GtkWidget *dc_compositor_page_get_blur_strength_scale(DcCompositorPage *page);
GtkWidget *dc_compositor_page_get_blur_background_switch(DcCompositorPage *page);
GtkWidget *dc_compositor_page_get_blur_background_frame_switch(DcCompositorPage *page);
GtkWidget *dc_compositor_page_get_backend_combo(DcCompositorPage *page);
GtkWidget *dc_compositor_page_get_vsync_switch(DcCompositorPage *page);
GtkWidget *dc_compositor_page_get_use_damage_switch(DcCompositorPage *page);

#endif
