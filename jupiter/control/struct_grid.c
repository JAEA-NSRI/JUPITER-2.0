#include "struct_grid.h"
#include "data_array.h"
#include "defs.h"

void jcntrl_struct_grid_clear(jcntrl_struct_grid *coords)
{
  if (coords->x_coords)
    jcntrl_data_array_delete(coords->x_coords);
  if (coords->y_coords)
    jcntrl_data_array_delete(coords->y_coords);
  if (coords->z_coords)
    jcntrl_data_array_delete(coords->z_coords);
  jcntrl_struct_grid_init(coords);
}

void jcntrl_struct_grid_copy(jcntrl_struct_grid *dest,
                             const jcntrl_struct_grid *src)
{
  jcntrl_struct_grid_clear(dest);

  for (int i = 0; i < 6; ++i)
    dest->extent[i] = src->extent[i];

  if (src->x_coords)
    dest->x_coords = jcntrl_data_array_take_ownership(src->x_coords);
  if (src->y_coords)
    dest->y_coords = jcntrl_data_array_take_ownership(src->y_coords);
  if (src->z_coords)
    dest->z_coords = jcntrl_data_array_take_ownership(src->z_coords);
}

static void jcntrl_struct_grid_set_setter(jcntrl_data_array **p,
                                          jcntrl_data_array *array)
{
  if (*p)
    jcntrl_data_array_delete(*p);
  *p = NULL;
  if (array)
    *p = jcntrl_data_array_take_ownership(array);
}

void jcntrl_struct_grid_set_x_coords(jcntrl_struct_grid *coords,
                                     jcntrl_data_array *array)
{
  jcntrl_struct_grid_set_setter(&coords->x_coords, array);
}

void jcntrl_struct_grid_set_y_coords(jcntrl_struct_grid *coords,
                                     jcntrl_data_array *array)
{
  jcntrl_struct_grid_set_setter(&coords->y_coords, array);
}

void jcntrl_struct_grid_set_z_coords(jcntrl_struct_grid *coords,
                                     jcntrl_data_array *array)
{
  jcntrl_struct_grid_set_setter(&coords->z_coords, array);
}
