#include "cell.h"
#include "data_array.h"
#include "defs.h"
#include "error.h"
#include "extent.h"
#include "grid_data.h"
#include "jupiter/geometry/defs.h"
#include "jupiter/geometry/util.h"
#include "jupiter/geometry/vector.h"
#include "shared_object.h"
#include "shared_object_priv.h"
#include "struct_grid.h"

#include <math.h>

static jcntrl_cell *jcntrl_cell_downcast_impl(jcntrl_shared_object *obj)
{
  return JCNTRL_DOWNCAST_IMPL(jcntrl_cell, obj);
}

static void *jcntrl_cell_downcast_v(jcntrl_shared_object *obj)
{
  return jcntrl_cell_downcast_impl(obj);
}

static int jcntrl_cell_number_of_points_impl(jcntrl_shared_object *obj)
{
  return 0;
}

JCNTRL_VIRTUAL_WRAP(jcntrl_cell, jcntrl_cell, number_of_points)

static void jcntrl_cell_center_impl(jcntrl_shared_object *obj, double pnt[3])
{
  jcntrl_raise_argument_error(__FILE__, __LINE__, "No cell center defined");
}

JCNTRL_VIRTUAL_WRAP(jcntrl_cell, jcntrl_cell, center)

static void jcntrl_cell_get_point_impl(jcntrl_shared_object *obj, int index,
                                       double pnt[3])
{
  jcntrl_raise_argument_error(__FILE__, __LINE__, "No cell points defined");
}

JCNTRL_VIRTUAL_WRAP(jcntrl_cell, jcntrl_cell, get_point)

static double jcntrl_cell_volume_impl(jcntrl_shared_object *obj) { return 0.0; }

JCNTRL_VIRTUAL_WRAP(jcntrl_cell, jcntrl_cell, volume)

static int jcntrl_cell_contain_impl(jcntrl_shared_object *obj, //
                                    double x, double y, double z)
{
  return 0;
}

JCNTRL_VIRTUAL_WRAP(jcntrl_cell, jcntrl_cell, contain)

static void jcntrl_cell_init_funcs(jcntrl_shared_object_funcs *p)
{
  p->downcast = jcntrl_cell_downcast_v;
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_cell, jcntrl_cell, number_of_points);
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_cell, jcntrl_cell, center);
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_cell, jcntrl_cell, get_point);
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_cell, jcntrl_cell, volume);
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_cell, jcntrl_cell, contain);
}

JCNTRL_SHARED_METADATA_INIT_DEFINE(jcntrl_cell, jcntrl_cell_init_funcs)

static jcntrl_cell_hex *jcntrl_cell_hex_downcast_impl(jcntrl_shared_object *obj)
{
  return JCNTRL_DOWNCAST_IMPL(jcntrl_cell_hex, obj);
}

static void *jcntrl_cell_hex_downcast_v(jcntrl_shared_object *obj)
{
  return jcntrl_cell_hex_downcast_impl(obj);
}

static int jcntrl_cell_hex_initializer(jcntrl_shared_object *obj)
{
  jcntrl_cell_hex *h = jcntrl_cell_hex_downcast_impl(obj);
  for (int i = 0; i < 3; ++i)
    h->p1[i] = 0.0;
  for (int i = 0; i < 3; ++i)
    h->p2[i] = 0.0;
  return 1;
}

static int jcntrl_cell_hex_number_of_points_impl(jcntrl_shared_object *obj)
{
  return 8;
}

JCNTRL_VIRTUAL_WRAP(jcntrl_cell_hex, jcntrl_cell, number_of_points)

static void jcntrl_cell_hex_center_impl(jcntrl_shared_object *obj,
                                        double pnt[3])
{
  jcntrl_cell_hex *h = jcntrl_cell_hex_downcast_impl(obj);
  geom_vec3 p1 = geom_vec3_c(h->p1[0], h->p1[1], h->p1[2]);
  geom_vec3 p2 = geom_vec3_c(h->p2[0], h->p2[1], h->p2[2]);
  p2 = geom_vec3_sub(p2, p1);
  p2 = geom_vec3_factor(p2, 0.5);
  p1 = geom_vec3_add(p1, p2);
  pnt[0] = geom_vec3_x(p1);
  pnt[1] = geom_vec3_y(p1);
  pnt[2] = geom_vec3_z(p1);
}

JCNTRL_VIRTUAL_WRAP(jcntrl_cell_hex, jcntrl_cell, center)

static void jcntrl_cell_hex_get_point_impl(jcntrl_shared_object *obj, int index,
                                           double pnt[3])
{
  jcntrl_cell_hex *h = jcntrl_cell_hex_downcast_impl(obj);

  if (index < 0 || index >= 8) {
    jcntrl_raise_index_error(__FILE__, __LINE__, index);
    return;
  }

  pnt[0] = (index & 1) ? h->p2[0] : h->p1[0];
  pnt[1] = (index & 2) ? h->p2[1] : h->p1[1];
  pnt[2] = (index & 4) ? h->p2[2] : h->p1[2];
}

JCNTRL_VIRTUAL_WRAP(jcntrl_cell_hex, jcntrl_cell, get_point)

static double jcntrl_cell_hex_volume_impl(jcntrl_shared_object *obj)
{
  jcntrl_cell_hex *h = jcntrl_cell_hex_downcast_impl(obj);
  double x, y, z;
  x = fabs(h->p2[0] - h->p1[0]);
  y = fabs(h->p2[1] - h->p1[1]);
  z = fabs(h->p2[2] - h->p1[2]);
  return x * y * z;
}

JCNTRL_VIRTUAL_WRAP(jcntrl_cell_hex, jcntrl_cell, volume)

static int jcntrl_cell_hex_contain_impl(jcntrl_shared_object *obj, //
                                        double x, double y, double z)
{
  jcntrl_cell_hex *h = jcntrl_cell_hex_downcast_impl(obj);
  if (h->p1[0] <= x && x <= h->p2[0] && h->p1[1] <= y && y <= h->p2[1] &&
      h->p1[2] <= z && z <= h->p2[2]) {
    if ((h->neighbors & JCNTRL_CELL_HEX_NEIGHBOR_E) && h->p2[0] <= x)
      return 0;
    if ((h->neighbors & JCNTRL_CELL_HEX_NEIGHBOR_N) && h->p2[1] <= y)
      return 0;
    if ((h->neighbors & JCNTRL_CELL_HEX_NEIGHBOR_T) && h->p2[2] <= z)
      return 0;
    return 1;
  }
  return 0;
}

JCNTRL_VIRTUAL_WRAP(jcntrl_cell_hex, jcntrl_cell, contain)

static void jcntrl_cell_hex_init_funcs(jcntrl_shared_object_funcs *p)
{
  p->downcast = jcntrl_cell_hex_downcast_v;
  p->initializer = jcntrl_cell_hex_initializer;
  p->destructor = NULL;
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_cell_hex, jcntrl_cell, number_of_points);
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_cell_hex, jcntrl_cell, center);
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_cell_hex, jcntrl_cell, get_point);
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_cell_hex, jcntrl_cell, volume);
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_cell_hex, jcntrl_cell, contain);
}

JCNTRL_SHARED_METADATA_INIT_DEFINE(jcntrl_cell_hex, jcntrl_cell_hex_init_funcs)

int jcntrl_cell_number_of_points(jcntrl_cell *c)
{
  struct jcntrl_cell_number_of_points_args d = {.number_of_points = 0};
  jcntrl_shared_object_call_virtual(jcntrl_cell_object(c), jcntrl_cell,
                                    number_of_points, &d);
  return d.number_of_points;
}

int jcntrl_cell_number_of_points__super(
  const jcntrl_shared_object_data *ancestor)
{
  struct jcntrl_cell_number_of_points_args d = {.number_of_points = 0};
  jcntrl_shared_object_call_super(ancestor, jcntrl_cell, number_of_points, &d);
  return d.number_of_points;
}

void jcntrl_cell_center(jcntrl_cell *c, double pnt[3])
{
  struct jcntrl_cell_center_args d = {.pnt = pnt};
  jcntrl_shared_object_call_virtual(jcntrl_cell_object(c), jcntrl_cell, center,
                                    &d);
}

void jcntrl_cell_center__super(const jcntrl_shared_object_data *ancestor,
                               double pnt[3])
{
  struct jcntrl_cell_center_args d = {.pnt = pnt};
  jcntrl_shared_object_call_super(ancestor, jcntrl_cell, center, &d);
}

void jcntrl_cell_get_point(jcntrl_cell *c, int index, double pnt[3])
{
  struct jcntrl_cell_get_point_args d = {.index = index, .pnt = pnt};
  jcntrl_shared_object_call_virtual(jcntrl_cell_object(c), jcntrl_cell,
                                    get_point, &d);
}

void jcntrl_cell_get_point__super(const jcntrl_shared_object_data *ancestor,
                                  int index, double pnt[3])
{
  struct jcntrl_cell_get_point_args d = {.index = index, .pnt = pnt};
  jcntrl_shared_object_call_super(ancestor, jcntrl_cell, get_point, &d);
}

double jcntrl_cell_volume(jcntrl_cell *c)
{
  struct jcntrl_cell_volume_args d = {.volume = 0.0};
  jcntrl_shared_object_call_virtual(jcntrl_cell_object(c), jcntrl_cell, volume,
                                    &d);
  return d.volume;
}

double jcntrl_cell_volume__super(const jcntrl_shared_object_data *ancestor)
{
  struct jcntrl_cell_volume_args d = {.volume = 0.0};
  jcntrl_shared_object_call_super(ancestor, jcntrl_cell, volume, &d);
  return d.volume;
}

int jcntrl_cell_contain(jcntrl_cell *c, double x, double y, double z)
{
  struct jcntrl_cell_contain_args d = {.ret = 0, .x = x, .y = y, .z = z};
  jcntrl_shared_object_call_virtual(jcntrl_cell_object(c), jcntrl_cell, contain,
                                    &d);
  return d.ret;
}

int jcntrl_cell_coatain__super(const jcntrl_shared_object_data *ancestor,
                               double x, double y, double z)
{
  struct jcntrl_cell_contain_args d = {.ret = 0, .x = x, .y = y, .z = z};
  jcntrl_shared_object_call_super(ancestor, jcntrl_cell, contain, &d);
  return d.ret;
}

jcntrl_shared_object *jcntrl_cell_object(jcntrl_cell *c) { return &c->object; }

jcntrl_cell *jcntrl_cell_downcast(jcntrl_shared_object *obj)
{
  return jcntrl_shared_object_downcast(jcntrl_cell, obj);
}

// hex (hexahedron cell)

int jcntrl_cell_hex_init_g(jcntrl_cell_hex *h, //
                           jcntrl_grid_data *g, int i, int j, int k,
                           const int whole_extent[6])
{
  return jcntrl_cell_hex_init_s(h, jcntrl_grid_data_struct_grid(g), i, j, k,
                                whole_extent);
}

int jcntrl_cell_hex_init_s(jcntrl_cell_hex *h, //
                           const jcntrl_struct_grid *s, int i, int j, int k,
                           const int whole_extent[6])
{
  int neighbors;
  jcntrl_shared_object_static_init(jcntrl_cell_hex_object(h),
                                   jcntrl_cell_hex_metadata_init());

  if (!jcntrl_irange_include(jcntrl_irange_c(s->extent[0], s->extent[1]), i))
    return 0;
  if (!jcntrl_irange_include(jcntrl_irange_c(s->extent[2], s->extent[3]), j))
    return 0;
  if (!jcntrl_irange_include(jcntrl_irange_c(s->extent[4], s->extent[5]), k))
    return 0;

  if (!whole_extent)
    whole_extent = s->extent;

  neighbors = 0;
  if (i > whole_extent[0])
    neighbors |= JCNTRL_CELL_HEX_NEIGHBOR_W;
  if (j > whole_extent[2])
    neighbors |= JCNTRL_CELL_HEX_NEIGHBOR_S;
  if (k > whole_extent[4])
    neighbors |= JCNTRL_CELL_HEX_NEIGHBOR_B;
  if (i < whole_extent[1] - 1)
    neighbors |= JCNTRL_CELL_HEX_NEIGHBOR_E;
  if (j < whole_extent[3] - 1)
    neighbors |= JCNTRL_CELL_HEX_NEIGHBOR_N;
  if (k < whole_extent[5] - 1)
    neighbors |= JCNTRL_CELL_HEX_NEIGHBOR_T;

  i -= s->extent[0];
  j -= s->extent[2];
  k -= s->extent[4];

  h->p1[0] = jcntrl_data_array_get_value(s->x_coords, i);
  h->p1[1] = jcntrl_data_array_get_value(s->y_coords, j);
  h->p1[2] = jcntrl_data_array_get_value(s->z_coords, k);
  h->p2[0] = jcntrl_data_array_get_value(s->x_coords, i + 1);
  h->p2[1] = jcntrl_data_array_get_value(s->y_coords, j + 1);
  h->p2[2] = jcntrl_data_array_get_value(s->z_coords, k + 1);
  h->neighbors = neighbors;
  return 1;
}

int jcntrl_cell_hex_number_of_points(jcntrl_cell_hex *c)
{
  return jcntrl_cell_number_of_points(jcntrl_cell_hex_cell(c));
}

void jcntrl_cell_hex_center(jcntrl_cell_hex *c, double pnt[3])
{
  jcntrl_cell_center(jcntrl_cell_hex_cell(c), pnt);
}

void jcntrl_cell_hex_get_point(jcntrl_cell_hex *c, int index, double pnt[3])
{
  jcntrl_cell_get_point(jcntrl_cell_hex_cell(c), index, pnt);
}

double jcntrl_cell_hex_volume(jcntrl_cell_hex *c)
{
  return jcntrl_cell_volume(jcntrl_cell_hex_cell(c));
}

int jcntrl_cell_hex_contain(jcntrl_cell_hex *c, double x, double y, double z)
{
  return jcntrl_cell_contain(jcntrl_cell_hex_cell(c), x, y, z);
}

jcntrl_shared_object *jcntrl_cell_hex_object(jcntrl_cell_hex *c)
{
  return jcntrl_cell_object(jcntrl_cell_hex_cell(c));
}

jcntrl_cell *jcntrl_cell_hex_cell(jcntrl_cell_hex *c) { return &c->cell; }

jcntrl_cell_hex *jcntrl_cell_hex_downcast(jcntrl_shared_object *obj)
{
  return jcntrl_shared_object_downcast(jcntrl_cell_hex, obj);
}
