#ifndef DC_OUTPUT_ROW_H
#define DC_OUTPUT_ROW_H

#include "domain/display_types.h"

typedef struct _DcOutputRow DcOutputRow;

typedef void (*DcOutputRowChangedFn)(gpointer user_data);
typedef void (*DcOutputRowPrimaryFn)(DcOutputRow *row, gpointer user_data);

DcOutputRow *dc_output_row_new(DcDisplayOutput *output,
                               DcOutputRowChangedFn on_changed,
                               DcOutputRowPrimaryFn on_primary_selected,
                               gpointer user_data);
void dc_output_row_free(DcOutputRow *row);

GtkWidget *dc_output_row_get_widget(DcOutputRow *row);
DcDisplayOutput *dc_output_row_get_output(DcOutputRow *row);

gboolean dc_output_row_is_enabled(DcOutputRow *row);
gboolean dc_output_row_is_primary(DcOutputRow *row);
RRMode dc_output_row_get_selected_mode(DcOutputRow *row);
Rotation dc_output_row_get_selected_rotation(DcOutputRow *row);
int dc_output_row_get_x(DcOutputRow *row);
int dc_output_row_get_y(DcOutputRow *row);
gboolean dc_output_row_get_geometry(DcOutputRow *row, int *x, int *y, int *width, int *height);

void dc_output_row_set_primary(DcOutputRow *row, gboolean primary);
void dc_output_row_set_enabled(DcOutputRow *row, gboolean enabled);
void dc_output_row_set_mode(DcOutputRow *row, RRMode mode_id);
void dc_output_row_set_rotation(DcOutputRow *row, Rotation rotation);
void dc_output_row_set_position(DcOutputRow *row, int x, int y);

#endif
