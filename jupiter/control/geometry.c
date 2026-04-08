#include "geometry.h"
#include "data_array.h"
#include "defs.h"
#include "error.h"
#include "jupiter/geometry/data.h"
#include "jupiter/geometry/defs.h"
#include "jupiter/geometry/file.h"
#include "jupiter/geometry/shape.h"
#include "jupiter/geometry/svector.h"
#include "overflow.h"
#include "shared_object.h"
#include "shared_object_priv.h"
#include "data_object.h"
#include "struct_data.h"
#include "geometry_data.h"

#include <stdlib.h>
#include <string.h>

static jcntrl_geometry *jcntrl_geometry_downcast_impl(jcntrl_shared_object *p)
{
  return JCNTRL_DOWNCAST_IMPL(jcntrl_geometry, p);
}

static void *jcntrl_geometry_downcast_v(jcntrl_shared_object *p)
{
  return jcntrl_geometry_downcast_impl(p);
}

static jcntrl_shared_object *jcntrl_geometry_allocator(void)
{
  jcntrl_geometry *p;
  p = jcntrl_shared_object_default_allocator(jcntrl_geometry);
  return p ? jcntrl_geometry_object(p) : NULL;
}

static void jcntrl_geometry_deleter(jcntrl_shared_object *obj)
{
  jcntrl_shared_object_default_deleter(obj);
}

static int jcntrl_geometry_initializer(jcntrl_shared_object *obj)
{
  jcntrl_geometry *g = jcntrl_geometry_downcast_impl(obj);
  g->data = NULL;
  g->file_data = NULL;
  return 1;
}

static void jcntrl_geometry_desctructor(jcntrl_shared_object *obj)
{
  jcntrl_geometry *g = jcntrl_geometry_downcast_impl(obj);
  if (g->file_data)
    jcntrl_data_array_delete(g->file_data);
}

int jcntrl_geometry_has_shape_impl(jcntrl_shared_object *obj)
{
  jcntrl_geometry *geometry;
  geometry = jcntrl_geometry_downcast_impl(obj);

  if (!geometry->data)
    return 0;

  return !!geom_data_element_get_shape(geometry->data);
}

JCNTRL_VIRTUAL_WRAP(jcntrl_geometry, jcntrl_geometry, has_shape)

int jcntrl_geometry_has_file_impl(jcntrl_shared_object *obj)
{
  jcntrl_geometry *geometry;
  geometry = jcntrl_geometry_downcast_impl(obj);

  if (!geometry->data)
    return 0;

  return !!geom_data_element_get_file(geometry->data);
}

JCNTRL_VIRTUAL_WRAP(jcntrl_geometry, jcntrl_geometry, has_file)

int jcntrl_geometry_file_load_impl(jcntrl_shared_object *obj,
                                   jcntrl_data_array **ret_array,
                                   int *ret_extent)
{
  jcntrl_raise_argument_error(__FILE__, __LINE__,
                              "jcntrl_geometry does not support loading files "
                              "(as Geometry library does not)");
  return 0;
}

JCNTRL_VIRTUAL_WRAP(jcntrl_geometry, jcntrl_geometry, file_load)

static void jcntrl_geometry_init_func(jcntrl_shared_object_funcs *p)
{
  p->downcast = jcntrl_geometry_downcast_v;
  p->allocator = jcntrl_geometry_allocator;
  p->deleter = jcntrl_geometry_deleter;
  p->initializer = jcntrl_geometry_initializer;
  p->destructor = jcntrl_geometry_desctructor;
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_geometry, jcntrl_geometry, has_shape);
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_geometry, jcntrl_geometry, has_file);
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_geometry, jcntrl_geometry, file_load);
}

JCNTRL_SHARED_METADATA_INIT_DEFINE(jcntrl_geometry, jcntrl_geometry_init_func)

jcntrl_geometry *jcntrl_geometry_new(void)
{
  return jcntrl_shared_object_new(jcntrl_geometry);
}

void jcntrl_geometry_delete(jcntrl_geometry *geometry)
{
  JCNTRL_ASSERT(geometry);
  jcntrl_shared_object_delete(jcntrl_geometry_object(geometry));
}

jcntrl_data_object *jcntrl_geometry_data_object(jcntrl_geometry *geometry)
{
  JCNTRL_ASSERT(geometry);
  return &geometry->data_object;
}

jcntrl_shared_object *jcntrl_geometry_object(jcntrl_geometry *geometry)
{
  JCNTRL_ASSERT(geometry);
  return jcntrl_data_object_object(jcntrl_geometry_data_object(geometry));
}

jcntrl_geometry *jcntrl_geometry_downcast(jcntrl_shared_object *object)
{
  return jcntrl_shared_object_downcast(jcntrl_geometry, object);
}

int jcntrl_geometry_has_shape(jcntrl_geometry *geometry)
{
  struct jcntrl_geometry_has_shape_args d = {.ret = 0};
  jcntrl_shared_object_call_virtual(jcntrl_geometry_object(geometry),
                                    jcntrl_geometry, has_shape, &d);
  return d.ret;
}

int jcntrl_geometry_shape_insideout_at(jcntrl_geometry *geometry, //
                                       int *ret, double x, double y, double z)
{
  geom_shape_data *s;

  JCNTRL_ASSERT(geometry);
  JCNTRL_ASSERT(ret);

  if (!geometry->data)
    return 0;

  s = geom_data_element_get_shape(geometry->data);
  if (!s)
    return 0;

  *ret = geom_shape_data_inbody_test_at(s, x, y, z);
  return 1;
}

int jcntrl_geometry_has_file(jcntrl_geometry *geometry)
{
  struct jcntrl_geometry_has_file_args d = {.ret = 0};
  jcntrl_shared_object_call_virtual(jcntrl_geometry_object(geometry),
                                    jcntrl_geometry, has_file, &d);
  return d.ret;
}

static int jcntrl_geometry_size_set(int *dest, geom_size_type value)
{
  *dest = value;
  if (*dest != value) {
    jcntrl_raise_overflow_error(__FILE__, __LINE__);
    return 0;
  }
  return 1;
}

const int *jcntrl_geometry_file_extent(jcntrl_geometry *geometry)
{
  geom_file_data *fdata;
  geom_svec3 orig, size;
  int e[6];
  JCNTRL_ASSERT(geometry);

  if (!geometry->data)
    return NULL;

  fdata = geom_data_element_get_file(geometry->data);
  if (!fdata)
    return NULL;

  orig = geom_file_data_get_origin(fdata);
  size = geom_file_data_get_size(fdata);
  if (!jcntrl_geometry_size_set(&e[0], geom_svec3_x(orig)))
    return NULL;
  if (!jcntrl_geometry_size_set(&e[1], geom_svec3_y(orig)))
    return NULL;
  if (!jcntrl_geometry_size_set(&e[2], geom_svec3_z(orig)))
    return NULL;
  if (!jcntrl_geometry_size_set(&e[3], geom_svec3_x(size)))
    return NULL;
  if (!jcntrl_geometry_size_set(&e[4], geom_svec3_y(size)))
    return NULL;
  if (!jcntrl_geometry_size_set(&e[5], geom_svec3_z(size)))
    return NULL;
  if (jcntrl_i_add_overflow(e[0], e[3], &e[3]))
    return NULL;
  if (jcntrl_i_add_overflow(e[1], e[4], &e[4]))
    return NULL;
  if (jcntrl_i_add_overflow(e[2], e[5], &e[5]))
    return NULL;

  geometry->file_extent[0] = e[0];
  geometry->file_extent[1] = e[3];
  geometry->file_extent[2] = e[1];
  geometry->file_extent[3] = e[4];
  geometry->file_extent[4] = e[2];
  geometry->file_extent[5] = e[5];
  return geometry->file_extent;
}

int jcntrl_geometry_file_repeat_count(jcntrl_geometry *geometry, int out[3])
{
  geom_file_data *fdata;
  geom_svec3 rep;
  int o[3];
  JCNTRL_ASSERT(geometry);

  if (!geometry->data)
    return 0;

  fdata = geom_data_element_get_file(geometry->data);
  if (!fdata)
    return 0;

  rep = geom_file_data_get_repeat(fdata);
  if (!jcntrl_geometry_size_set(&o[0], geom_svec3_x(rep)))
    return 0;
  if (!jcntrl_geometry_size_set(&o[1], geom_svec3_y(rep)))
    return 0;
  if (!jcntrl_geometry_size_set(&o[3], geom_svec3_z(rep)))
    return 0;
  for (int i = 0; i < 3; ++i)
    out[i] = o[i];
  return 1;
}

int jcntrl_geometry_file_repeat_offset(jcntrl_geometry *geometry, int out[3])
{
  geom_file_data *fdata;
  geom_svec3 rep;
  int o[3];
  JCNTRL_ASSERT(geometry);

  if (!geometry->data)
    return 0;

  fdata = geom_data_element_get_file(geometry->data);
  if (!fdata)
    return 0;

  rep = geom_file_data_get_offset(fdata);
  if (!jcntrl_geometry_size_set(&o[0], geom_svec3_x(rep)))
    return 0;
  if (!jcntrl_geometry_size_set(&o[1], geom_svec3_y(rep)))
    return 0;
  if (!jcntrl_geometry_size_set(&o[3], geom_svec3_z(rep)))
    return 0;
  for (int i = 0; i < 3; ++i)
    out[i] = o[i];
  return 1;
}

int jcntrl_geometry_file_value(jcntrl_geometry *geometry, //
                               double *ret, int i, int j, int k);

jcntrl_data_array *jcntrl_geometry_file_data(jcntrl_geometry *geometry)
{
  JCNTRL_ASSERT(geometry);
  return geometry->file_data;
}

jcntrl_data_array *jcntrl_geometry_file_load(jcntrl_geometry *geometry)
{
  int rext[6];
  jcntrl_data_array *ary;
  JCNTRL_ASSERT(geometry);

  ary = geometry->file_data;
    struct jcntrl_geometry_file_load_args d = {
    .ret_array = &ary,
    .ret_extent = rext,
    .ret = 0,
  };
  jcntrl_shared_object_call_virtual(jcntrl_geometry_object(geometry),
                                    jcntrl_geometry, file_load, &d);
  if (d.ret) {
    if (ary != geometry->file_data) {
      if (geometry->file_data)
        jcntrl_data_array_delete(geometry->file_data);
      geometry->file_data = ary;
    }
    for (int i = 0; i < 6; ++i)
      geometry->data_extent[i] = rext[i];
    return ary;
  } else {
    if (ary && ary != geometry->file_data)
      jcntrl_data_array_delete(ary);
  }
  return NULL;
}

int jcntrl_geometry_update_transform(jcntrl_geometry *geometry)
{
  geom_error err;
  geom_shape_data *s;
  JCNTRL_ASSERT(geometry);

  if (!geometry->data)
    return 1;

  s = geom_data_element_get_shape(geometry->data);
  if (!s)
    return 1;

  err = geom_shape_data_update_all_transform(s, NULL, NULL, NULL);
  if (err != GEOM_SUCCESS) {
    jcntrl_raise_geom_error(__FILE__, __LINE__, err, NULL);
    return 0;
  }
  return 1;
}

void jcntrl_geometry_set_data_element(jcntrl_geometry *geometry,
                                      geom_data_element *el)
{
  JCNTRL_ASSERT(geometry);
  geometry->data = el;
}

geom_data_element *jcntrl_geometry_get_data_element(jcntrl_geometry *geometry)
{
  JCNTRL_ASSERT(geometry);
  return geometry->data;
}

const int *jcntrl_geometry_data_extent(jcntrl_geometry *geometry)
{
  JCNTRL_ASSERT(geometry);
  return geometry->data_extent;
}
