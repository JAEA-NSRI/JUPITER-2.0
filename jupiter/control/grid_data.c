#include "grid_data.h"
#include "cell_data.h"
#include "data_array.h"
#include "data_object.h"
#include "defs.h"
#include "error.h"
#include "mask_data.h"
#include "shared_object.h"
#include "shared_object_priv.h"
#include "struct_data.h"
#include "struct_grid.h"

#include <stdlib.h>

static jcntrl_grid_data *
jcntrl_grid_data_downcast_impl(jcntrl_shared_object *obj)
{
  return JCNTRL_DOWNCAST_IMPL(jcntrl_grid_data, obj);
}

static void *jcntrl_grid_data_downcast_v(jcntrl_shared_object *obj)
{
  return jcntrl_grid_data_downcast_impl(obj);
}

static int jcntrl_grid_data_initializer(jcntrl_shared_object *obj)
{
  jcntrl_grid_data *g;

  g = jcntrl_grid_data_downcast_impl(obj);
  g->cell_data = jcntrl_cell_data_new();
  if (!g->cell_data)
    return 0;
  g->mask = NULL;
  jcntrl_struct_grid_init(&g->coords);
  return 1;
}

static void jcntrl_grid_data_destructor(jcntrl_shared_object *obj)
{
  jcntrl_grid_data *g;

  g = jcntrl_grid_data_downcast_impl(obj);
  if (g->cell_data)
    jcntrl_cell_data_delete(g->cell_data);
  if (g->mask)
    jcntrl_mask_data_delete(g->mask);
  g->cell_data = NULL;
  g->mask = NULL;
  jcntrl_struct_grid_clear(&g->coords);
}

static jcntrl_shared_object *jcntrl_grid_data_allocator(void)
{
  jcntrl_grid_data *p;
  p = jcntrl_shared_object_default_allocator(jcntrl_grid_data);
  return p ? jcntrl_grid_data_object(p) : NULL;
}

static void jcntrl_grid_data_deleter(jcntrl_shared_object *obj)
{
  jcntrl_shared_object_default_deleter(obj);
}

static void jcntrl_grid_data_init_func(jcntrl_shared_object_funcs *p)
{
  p->allocator = jcntrl_grid_data_allocator;
  p->deleter = jcntrl_grid_data_deleter;
  p->initializer = jcntrl_grid_data_initializer;
  p->destructor = jcntrl_grid_data_destructor;
  p->downcast = jcntrl_grid_data_downcast_v;
}

JCNTRL_SHARED_METADATA_INIT_DEFINE(jcntrl_grid_data, jcntrl_grid_data_init_func)

jcntrl_grid_data *jcntrl_grid_data_new(void)
{
  return jcntrl_shared_object_new(jcntrl_grid_data);
}

void jcntrl_grid_data_delete(jcntrl_grid_data *grid)
{
  jcntrl_shared_object_delete(jcntrl_grid_data_object(grid));
}

jcntrl_shared_object *jcntrl_grid_data_object(jcntrl_grid_data *grid)
{
  return &grid->data_object.object;
}

jcntrl_grid_data *jcntrl_grid_data_downcast(jcntrl_shared_object *object)
{
  return jcntrl_shared_object_downcast(jcntrl_grid_data, object);
}

jcntrl_cell_data *jcntrl_grid_data_cell_data(jcntrl_grid_data *grid)
{
  return grid->cell_data;
}

const int *jcntrl_grid_data_extent(jcntrl_grid_data *grid)
{
  return jcntrl_struct_grid_get_extent(&grid->coords);
}

void jcntrl_grid_data_set_extent(jcntrl_grid_data *grid, int extent[6])
{
  jcntrl_struct_grid_set_extent(&grid->coords, extent);
}

jcntrl_data_array *jcntrl_grid_data_x_coords(jcntrl_grid_data *grid)
{
  return jcntrl_struct_grid_get_x_coords(&grid->coords);
}

void jcntrl_grid_data_set_x_coords(jcntrl_grid_data *grid,
                                   jcntrl_data_array *x_coords)
{
  jcntrl_struct_grid_set_x_coords(&grid->coords, x_coords);
}

jcntrl_data_array *jcntrl_grid_data_y_coords(jcntrl_grid_data *grid)
{
  return jcntrl_struct_grid_get_y_coords(&grid->coords);
}

void jcntrl_grid_data_set_y_coords(jcntrl_grid_data *grid,
                                   jcntrl_data_array *y_coords)
{
  jcntrl_struct_grid_set_y_coords(&grid->coords, y_coords);
}

jcntrl_data_array *jcntrl_grid_data_z_coords(jcntrl_grid_data *grid)
{
  return jcntrl_struct_grid_get_z_coords(&grid->coords);
}

void jcntrl_grid_data_set_z_coords(jcntrl_grid_data *grid,
                                   jcntrl_data_array *z_coords)
{
  jcntrl_struct_grid_set_z_coords(&grid->coords, z_coords);
}

const jcntrl_struct_grid *jcntrl_grid_data_struct_grid(jcntrl_grid_data *grid)
{
  return &grid->coords;
}

void jcntrl_grid_data_set_struct_grid(jcntrl_grid_data *grid,
                                      const jcntrl_struct_grid *coords)
{
  jcntrl_struct_grid_copy(&grid->coords, coords);
}

jcntrl_mask_data *jcntrl_grid_data_get_mask(jcntrl_grid_data *grid)
{
  return grid->mask;
}

void jcntrl_grid_data_set_mask(jcntrl_grid_data *grid, jcntrl_mask_data *mask)
{
  if (mask) {
    jcntrl_shared_object *obj;
    obj = jcntrl_mask_data_object(mask);
    if (!jcntrl_shared_object_take_ownership(obj))
      return;
  }
  if (grid->mask) {
    jcntrl_mask_data_delete(grid->mask);
  }
  grid->mask = mask;
}

int jcntrl_grid_data_shallow_copy(jcntrl_grid_data *dest,
                                  jcntrl_grid_data *from)
{
  jcntrl_struct_grid_copy(&dest->coords, &from->coords);
  if (!jcntrl_cell_data_shallow_copy(dest->cell_data, from->cell_data))
    return 0;

  if (from->mask) {
    if (!dest->mask) {
      dest->mask = jcntrl_mask_data_new();
      if (!dest->mask)
        return 0;
    }

    jcntrl_mask_data_set_array(dest->mask, jcntrl_mask_data_array(from->mask));
  } else {
    if (dest->mask)
      jcntrl_mask_data_set_array(dest->mask, NULL);
  }
  return 1;
}

int jcntrl_grid_data_deep_copy(jcntrl_grid_data *dest, jcntrl_grid_data *from)
{
  jcntrl_data_array *x, *y, *z;

  x = NULL;
  y = NULL;
  z = NULL;
  do {
    x = jcntrl_data_array_dup(jcntrl_struct_grid_get_x_coords(&from->coords));
    if (!x)
      break;

    y = jcntrl_data_array_dup(jcntrl_struct_grid_get_y_coords(&from->coords));
    if (!y)
      break;

    z = jcntrl_data_array_dup(jcntrl_struct_grid_get_z_coords(&from->coords));
    if (!z)
      break;

    jcntrl_struct_grid_copy(&dest->coords, &from->coords);
    jcntrl_struct_grid_set_x_coords(&dest->coords, x);
    jcntrl_struct_grid_set_y_coords(&dest->coords, y);
    jcntrl_struct_grid_set_z_coords(&dest->coords, z);
  } while (0);
  if (x)
    jcntrl_shared_object_release_ownership(jcntrl_data_array_object(x));
  if (y)
    jcntrl_shared_object_release_ownership(jcntrl_data_array_object(y));
  if (z)
    jcntrl_shared_object_release_ownership(jcntrl_data_array_object(z));
  if (!x || !y || !z)
    return 0;

  if (!jcntrl_cell_data_deep_copy(dest->cell_data, from->cell_data))
    return 0;

  if (from->mask) {
    jcntrl_size_type nt;
    jcntrl_bool_array *inp, *cpy;

    if (!dest->mask) {
      dest->mask = jcntrl_mask_data_new();
      if (!dest->mask)
        return 0;
    }

    nt = 0;
    cpy = NULL;
    inp = jcntrl_mask_data_array(from->mask);
    if (inp) {
      nt = jcntrl_bool_array_get_ntuple(inp);

      cpy = jcntrl_bool_array_new();
      if (!cpy)
        return 0;

      if (!jcntrl_bool_array_resize(cpy, nt)) {
        jcntrl_bool_array_delete(cpy);
        return 0;
      }

      if (!jcntrl_bool_array_copy(cpy, jcntrl_bool_array_data(inp), nt, 0, 0)) {
        jcntrl_bool_array_delete(cpy);
        return 0;
      }
    }

    if (cpy) {
      jcntrl_mask_data_set_array(dest->mask, cpy);
      jcntrl_shared_object_release_ownership(jcntrl_bool_array_object(cpy));
    } else {
      jcntrl_mask_data_set_array(dest->mask, NULL);
    }
  } else {
    if (dest->mask)
      jcntrl_mask_data_set_array(dest->mask, NULL);
  }
  return 1;
}
