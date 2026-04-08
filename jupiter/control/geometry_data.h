#ifndef JUPITER_CONTROL_GEOMETRY_DATA_H
#define JUPITER_CONTROL_GEOMETRY_DATA_H

#include "defs.h"
#include "jupiter/geometry/defs.h"
#include "shared_object_priv.h"
#include "struct_data.h"
#include "geometry.h"

JUPITER_CONTROL_DECL_BEGIN

//-- has_shape

typedef int jcntrl_geometry_has_shape_func(jcntrl_shared_object *obj);

struct jcntrl_geometry_has_shape_args
{
  int ret;
};

static inline void
jcntrl_geometry_has_shape__wrapper(jcntrl_shared_object *obj, void *arg,
                                   jcntrl_geometry_has_shape_func *func)
{
  struct jcntrl_geometry_has_shape_args *p;
  p = (struct jcntrl_geometry_has_shape_args *)arg;
  p->ret = func(obj);
}

static inline int
jcntrl_geometry_has_shape__super(const jcntrl_shared_object_data *ancestor)
{
  struct jcntrl_geometry_has_shape_args d = {.ret = 0};
  jcntrl_shared_object_call_super(ancestor, jcntrl_geometry, has_shape, &d);
  return d.ret;
}

//--- has_file

typedef int jcntrl_geometry_has_file_func(jcntrl_shared_object *obj);

struct jcntrl_geometry_has_file_args
{
  int ret;
};

static inline void
jcntrl_geometry_has_file__wrapper(jcntrl_shared_object *obj, void *arg,
                                  jcntrl_geometry_has_file_func *func)
{
  struct jcntrl_geometry_has_file_args *p;
  p = (struct jcntrl_geometry_has_file_args *)arg;
  p->ret = func(obj);
}

static inline int
jcntrl_geometry_has_file__super(const jcntrl_shared_object_data *ancestor)
{
  struct jcntrl_geometry_has_file_args d = {.ret = 0};
  jcntrl_shared_object_call_super(ancestor, jcntrl_geometry, has_file, &d);
  return d.ret;
}

//--- file_load

typedef int jcntrl_geometry_file_load_func(jcntrl_shared_object *obj,
                                           jcntrl_data_array **ret_array,
                                           int ret_extent[6]);

struct jcntrl_geometry_file_load_args
{
  jcntrl_data_array **ret_array;
  int *ret_extent;
  int ret;
};

static inline void
jcntrl_geometry_file_load__wrapper(jcntrl_shared_object *obj, void *arg,
                                   jcntrl_geometry_file_load_func *func)
{
  struct jcntrl_geometry_file_load_args *p;
  p = (struct jcntrl_geometry_file_load_args *)arg;
  p->ret = func(obj, p->ret_array, p->ret_extent);
}

static inline int
jcntrl_geometry_file_load__super(const jcntrl_shared_object_data *ancestor,
                                 jcntrl_data_array **ret_array,
                                 int ret_extent[6])
{
  struct jcntrl_geometry_file_load_args d = {
    .ret_array = ret_array,
    .ret_extent = ret_extent,
    .ret = 0,
  };
  jcntrl_shared_object_call_super(ancestor, jcntrl_geometry, file_load, &d);
  return d.ret;
}

JUPITER_CONTROL_DECL_END

#endif
