#ifndef JUPITER_CONTROL_GRID_DATA_H
#define JUPITER_CONTROL_GRID_DATA_H

#include "defs.h"
#include "shared_object.h"

JUPITER_CONTROL_DECL_BEGIN

JUPITER_CONTROL_DECL
JCNTRL_SHARED_METADATA_INIT_DECL(jcntrl_grid_data);

JUPITER_CONTROL_DECL
jcntrl_grid_data *jcntrl_grid_data_new(void);

JUPITER_CONTROL_DECL
void jcntrl_grid_data_delete(jcntrl_grid_data *grid);

JUPITER_CONTROL_DECL
jcntrl_shared_object *jcntrl_grid_data_object(jcntrl_grid_data *grid);

JUPITER_CONTROL_DECL
jcntrl_grid_data *jcntrl_grid_data_downcast(jcntrl_shared_object *object);

JUPITER_CONTROL_DECL
jcntrl_cell_data *jcntrl_grid_data_cell_data(jcntrl_grid_data *grid);

JUPITER_CONTROL_DECL
int jcntrl_grid_data_shallow_copy(jcntrl_grid_data *dest,
                                  jcntrl_grid_data *from);

JUPITER_CONTROL_DECL
int jcntrl_grid_data_deep_copy(jcntrl_grid_data *dest, jcntrl_grid_data *from);

JUPITER_CONTROL_DECL
const int *jcntrl_grid_data_extent(jcntrl_grid_data *grid);
JUPITER_CONTROL_DECL
void jcntrl_grid_data_set_extent(jcntrl_grid_data *grid, int extent[6]);

/**
 * Setting coords does not update extent. Please update it with
 * jcntrl_grid_data_set_extent().
 */

JUPITER_CONTROL_DECL
jcntrl_data_array *jcntrl_grid_data_x_coords(jcntrl_grid_data *grid);
JUPITER_CONTROL_DECL
void jcntrl_grid_data_set_x_coords(jcntrl_grid_data *grid,
                                   jcntrl_data_array *x_coords);

JUPITER_CONTROL_DECL
jcntrl_data_array *jcntrl_grid_data_y_coords(jcntrl_grid_data *grid);
JUPITER_CONTROL_DECL
void jcntrl_grid_data_set_y_coords(jcntrl_grid_data *grid,
                                   jcntrl_data_array *y_coords);

JUPITER_CONTROL_DECL
jcntrl_data_array *jcntrl_grid_data_z_coords(jcntrl_grid_data *grid);
JUPITER_CONTROL_DECL
void jcntrl_grid_data_set_z_coords(jcntrl_grid_data *grid,
                                   jcntrl_data_array *z_coords);

JUPITER_CONTROL_DECL
const jcntrl_struct_grid *jcntrl_grid_data_struct_grid(jcntrl_grid_data *grid);
JUPITER_CONTROL_DECL
void jcntrl_grid_data_set_struct_grid(jcntrl_grid_data *grid,
                                      const jcntrl_struct_grid *coords);

JUPITER_CONTROL_DECL
jcntrl_mask_data *jcntrl_grid_data_get_mask(jcntrl_grid_data *grid);
JUPITER_CONTROL_DECL
void jcntrl_grid_data_set_mask(jcntrl_grid_data *grid, jcntrl_mask_data *mask);

JUPITER_CONTROL_DECL_END

#endif
