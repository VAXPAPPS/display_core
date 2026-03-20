#ifndef DC_PREVIEW_CANVAS_H
#define DC_PREVIEW_CANVAS_H

#include "ui/output_row.h"

typedef struct _DcPreviewCanvas DcPreviewCanvas;

DcPreviewCanvas *dc_preview_canvas_new(void);
void dc_preview_canvas_free(DcPreviewCanvas *preview);

GtkWidget *dc_preview_canvas_get_widget(DcPreviewCanvas *preview);
void dc_preview_canvas_set_rows(DcPreviewCanvas *preview, GPtrArray *rows);
void dc_preview_canvas_queue_draw(DcPreviewCanvas *preview);

#endif
