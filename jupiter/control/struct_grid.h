/**
 * Common implementation for jcntrl_struct_coords
 *
 * This is private module.
 */

#ifndef JUPITER_CONTROL_STRUCT_GRID_H
#define JUPITER_CONTROL_STRUCT_GRID_H

#include "defs.h"
#include "error.h"
#include "struct_data.h"

JUPITER_CONTROL_DECL_BEGIN

static inline void jcntrl_struct_grid_init(jcntrl_struct_grid *coords)
{
  JCNTRL_ASSERT(coords);

  coords->x_coords = NULL;
  coords->y_coords = NULL;
  coords->z_coords = NULL;
  coords->extent[0] = 0;
  coords->extent[2] = 0;
  coords->extent[4] = 0;
  coords->extent[1] = -1;
  coords->extent[3] = -1;
  coords->extent[5] = -1;
}

JUPITER_CONTROL_DECL
void jcntrl_struct_grid_clear(jcntrl_struct_grid *coords);

/**
 * Copy makes shallow copy of arrays.
 *
 * If you do not want to do so (e.g. for automatic variable), you can use just
 * assignment struct.
 */
JUPITER_CONTROL_DECL
void jcntrl_struct_grid_copy(jcntrl_struct_grid *dest,
                             const jcntrl_struct_grid *src);

static inline jcntrl_data_array *
jcntrl_struct_grid_get_x_coords(jcntrl_struct_grid *coords)
{
  return coords->x_coords;
}

static inline jcntrl_data_array *
jcntrl_struct_grid_get_y_coords(jcntrl_struct_grid *coords)
{
  return coords->y_coords;
}

static inline jcntrl_data_array *
jcntrl_struct_grid_get_z_coords(jcntrl_struct_grid *coords)
{
  return coords->z_coords;
}

static inline const int *
jcntrl_struct_grid_get_extent(jcntrl_struct_grid *coords)
{
  return coords->extent;
}

static inline void jcntrl_struct_grid_set_extent(jcntrl_struct_grid *coords,
                                                 const int extent[6])
{
  for (int i = 0; i < 6; ++i) {
    coords->extent[i] = extent[i];
  }
}

/*
 * Setting coords in following functions make shallow copy of @p array. Use
 * jcntrl_struct_grid_clear() to delete them.
 *
 * If you do not to want to do so (e.g. for automatic variable), you can set it
 * directly, and do not use jcntrl_struct_grid_clear() function.
 */

JUPITER_CONTROL_DECL
void jcntrl_struct_grid_set_x_coords(jcntrl_struct_grid *coords,
                                     jcntrl_data_array *array);

JUPITER_CONTROL_DECL
void jcntrl_struct_grid_set_y_coords(jcntrl_struct_grid *coords,
                                     jcntrl_data_array *array);

JUPITER_CONTROL_DECL
void jcntrl_struct_grid_set_z_coords(jcntrl_struct_grid *coords,
                                     jcntrl_data_array *array);

JUPITER_CONTROL_DECL_END

#endif
