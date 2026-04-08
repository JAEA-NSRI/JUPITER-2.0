#include "mask_geometry.h"
#include "cell.h"
#include "comparator.h"
#include "data_array.h"
#include "error.h"
#include "executive.h"
#include "extent.h"
#include "field_variable.h"
#include "geometry.h"
#include "information.h"
#include "input.h"
#include "output.h"
#include "shared_object.h"
#include "shared_object_priv.h"
#include "struct_data.h"
#include "mask_function.h"
#include "executive_data.h"
#include "mask_function_priv.h"
#include "defs.h"

struct jcntrl_mask_geometry_function
{
  jcntrl_mask_function func;
  jcntrl_geometry *geometry;
  jcntrl_comparator vof_comp;
  double vof_threshold; /// TODO: support field function
};
#define jcntrl_mask_geometry_function__ancestor jcntrl_mask_function
#define jcntrl_mask_geometry_function__dnmem func.jcntrl_mask_function__dnmem
JCNTRL_VTABLE_NONE(jcntrl_mask_geometry_function);

static jcntrl_mask_geometry_function *
jcntrl_mask_geometry_function_downcast_impl(jcntrl_shared_object *obj)
{
  return JCNTRL_DOWNCAST_IMPL(jcntrl_mask_geometry_function, obj);
}

static void *jcntrl_mask_geometry_function_downcast_v(jcntrl_shared_object *obj)
{
  return jcntrl_mask_geometry_function_downcast_impl(obj);
}

static int jcntrl_mask_geometry_function_initializer(jcntrl_shared_object *obj)
{
  jcntrl_mask_geometry_function *f;
  f = jcntrl_mask_geometry_function_downcast_impl(obj);
  f->geometry = NULL;
  f->vof_comp = JCNTRL_COMP_INVALID;
  f->vof_threshold = 0.0;
  return 1;
}

static void jcntrl_mask_geometry_function_destructor(jcntrl_shared_object *obj)
{
  jcntrl_mask_geometry_function *f;
  f = jcntrl_mask_geometry_function_downcast_impl(obj);
  if (f->geometry)
    jcntrl_geometry_delete(f->geometry);
}

static jcntrl_shared_object *jcntrl_mask_geometry_function_allocator(void)
{
  jcntrl_mask_geometry_function *f;
  f = jcntrl_shared_object_default_allocator(jcntrl_mask_geometry_function);
  return f ? jcntrl_mask_geometry_function_object(f) : NULL;
}

static void jcntrl_mask_geometry_function_deleter(jcntrl_shared_object *obj)
{
  jcntrl_shared_object_default_deleter(obj);
}

static int jcntrl_mask_geometry_function_eval_impl(jcntrl_shared_object *obj,
                                                   jcntrl_cell *cell, int i,
                                                   int j, int k)
{
  jcntrl_mask_geometry_function *f;
  f = jcntrl_mask_geometry_function_downcast_impl(obj);
  if (!f->geometry)
    return 0;

  if (jcntrl_geometry_has_shape(f->geometry)) {
    double pnt[3];
    int r, ret;
    jcntrl_cell_center(cell, pnt);
    r = jcntrl_geometry_shape_insideout_at(f->geometry, &ret, pnt[0], pnt[1],
                                           pnt[2]);
    if (!r)
      return 0;
    return !ret;
  }

  if (jcntrl_geometry_has_file(f->geometry)) {
    jcntrl_extent fextent;
    const int *file_extent;
    jcntrl_data_array *file_data;
    jcntrl_size_type jj;
    double vof_val;

    if (!jcntrl_comparator_is_valid(f->vof_comp))
      return 0;

    file_data = jcntrl_geometry_file_data(f->geometry);
    if (!file_data) {
      file_data = jcntrl_geometry_file_load(f->geometry);
      if (!file_data)
        return 0;
    }

    file_extent = jcntrl_geometry_file_extent(f->geometry);
    if (!file_extent)
      return 0;

    fextent = jcntrl_extent_c(file_extent);
    jj = jcntrl_extent_addr(fextent, i, j, k);
    if (jj < 0) {
      jcntrl_raise_argument_error(__FILE__, __LINE__,
                                  "Index does not included in the loaded file");
      return 0;
    }

    vof_val = jcntrl_data_array_get_value(file_data, jj);
    return !jcntrl_compare_d(f->vof_comp, vof_val, f->vof_threshold);
  }

  return 0;
}

JCNTRL_VIRTUAL_WRAP(jcntrl_mask_geometry_function, jcntrl_mask_function, eval)

static void
jcntrl_mask_geometry_function_init_func(jcntrl_shared_object_funcs *p)
{
  p->downcast = jcntrl_mask_geometry_function_downcast_v;
  p->initializer = jcntrl_mask_geometry_function_initializer;
  p->destructor = jcntrl_mask_geometry_function_destructor;
  p->allocator = jcntrl_mask_geometry_function_allocator;
  p->deleter = jcntrl_mask_geometry_function_deleter;
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_mask_geometry_function,
                          jcntrl_mask_function, eval);
}

JCNTRL_SHARED_METADATA_INIT_DEFINE(jcntrl_mask_geometry_function,
                                   jcntrl_mask_geometry_function_init_func)

struct jcntrl_mask_geometry
{
  jcntrl_executive exec;
  jcntrl_comparator vof_comp;
  double default_vof_threshold;
};
#define jcntrl_mask_geometry__ancestor jcntrl_executive
#define jcntrl_mask_geometry__dnmem exec.jcntrl_executive__dnmem
JCNTRL_VTABLE_NONE(jcntrl_mask_geometry);

static inline jcntrl_mask_geometry *
jcntrl_mask_geometry_downcast_impl(jcntrl_shared_object *obj)
{
  return JCNTRL_DOWNCAST_IMPL(jcntrl_mask_geometry, obj);
}

static void *jcntrl_mask_geometry_downcast_v(jcntrl_shared_object *obj)
{
  return jcntrl_mask_geometry_downcast_impl(obj);
}

static int jcntrl_mask_geometry_initializer(jcntrl_shared_object *obj)
{
  jcntrl_mask_geometry *f;
  jcntrl_executive *exec;
  jcntrl_output *output;
  jcntrl_input *input;

  f = jcntrl_mask_geometry_downcast_impl(obj);
  f->default_vof_threshold = 0.0;
  f->vof_comp = JCNTRL_COMP_INVALID;

  exec = jcntrl_mask_geometry_executive(f);
  input = jcntrl_executive_get_input(exec);
  output = jcntrl_executive_get_output(exec);

  if (!jcntrl_input_set_number_of_ports(input, 2))
    return 0;
  if (!jcntrl_output_set_number_of_ports(output, 1))
    return 0;
  return 1;
}

static void jcntrl_mask_geometry_destructor(jcntrl_shared_object *obj)
{
  /* nop */
}

static jcntrl_shared_object *jcntrl_mask_geometry_allocator(void)
{
  jcntrl_mask_geometry *p;
  p = jcntrl_shared_object_default_allocator(jcntrl_mask_geometry);
  return p ? jcntrl_mask_geometry_object(p) : NULL;
}

static void jcntrl_mask_geometry_deleter(jcntrl_shared_object *obj)
{
  jcntrl_shared_object_default_deleter(obj);
}

static int jcntrl_mask_geometry_fill_input_port_information_impl(
  jcntrl_shared_object *obj, int index, jcntrl_input *input)
{
  jcntrl_information *info;

  info = jcntrl_input_information(input);
  switch (index) {
  case 0:
    if (!jcntrl_information_set_datatype(info, JCNTRL_INFO_REQUIRED_DATATYPE,
                                         JCNTRL_DATATYPE_GEOMETRY))
      return 0;
    break;
  case 1:
    /* TODO: Currently field function is not accepted */
    if (!jcntrl_information_set_datatype(info, JCNTRL_INFO_REQUIRED_DATATYPE,
                                         JCNTRL_DATATYPE_FIELD_VAR))
      return 0;
    break;
  }
  return 1;
}

JCNTRL_VIRTUAL_WRAP(jcntrl_mask_geometry, jcntrl_executive,
                    fill_input_port_information)

static int jcntrl_mask_geometry_fill_output_port_information_impl(
  jcntrl_shared_object *obj, int index, jcntrl_output *output)
{
  jcntrl_information *info;

  info = jcntrl_output_information(output);
  if (!jcntrl_information_set_objecttype(info, JCNTRL_INFO_DATATYPE,
                                         jcntrl_mask_geometry_function))
    return 0;
  return 1;
}

JCNTRL_VIRTUAL_WRAP(jcntrl_mask_geometry, jcntrl_executive,
                    fill_output_port_information)

static int jcntrl_mask_geometry_process_update_data_impl(
  jcntrl_information *request, jcntrl_input *input_head,
  jcntrl_output *output_head, jcntrl_shared_object *obj)
{
  jcntrl_mask_geometry *g = jcntrl_mask_geometry_downcast_impl(obj);
  jcntrl_information *info;
  jcntrl_input *input;
  jcntrl_output *output;
  jcntrl_shared_object *iobj;
  jcntrl_shared_object *oobj;
  jcntrl_geometry *geom;
  jcntrl_mask_geometry_function *func;
  jcntrl_executive *exec;
  jcntrl_field_variable *ifvar;
  jcntrl_data_array *fary;
  double threshold;

  input = jcntrl_mask_geometry_geometry_input(g);
  JCNTRL_ASSERT(input);

  info = jcntrl_input_upstream_information(input);
  JCNTRL_ASSERT(info);

  iobj = jcntrl_information_get_object(info, JCNTRL_INFO_DATA_OBJECT);
  geom = NULL;
  if (iobj)
    geom = jcntrl_geometry_downcast(iobj);

  input = jcntrl_mask_geometry_vof_threshold_input(g);
  JCNTRL_ASSERT(info);

  ifvar = NULL;
  info = jcntrl_input_upstream_information(input);
  if (info) {
    iobj = jcntrl_information_get_object(info, JCNTRL_INFO_DATA_OBJECT);
    if (iobj)
      ifvar = jcntrl_field_variable_downcast(iobj);
  }

  exec = jcntrl_mask_geometry_executive(g);
  output = jcntrl_executive_output_port(exec, 0);

  info = jcntrl_output_information(output);
  JCNTRL_ASSERT(info);

  oobj = jcntrl_information_get_object(info, JCNTRL_INFO_DATA_OBJECT);
  func = NULL;
  if (oobj)
    func = jcntrl_mask_geometry_function_downcast(oobj);
  if (!func)
    return 0;

  fary = NULL;
  if (ifvar)
    fary = jcntrl_field_variable_array(ifvar);

  if (fary) {
    if (jcntrl_data_array_get_ntuple(fary) != 1) {
      jcntrl_raise_argument_error(
        __FILE__, __LINE__, "Number of elements in field variable is not 1");
      return 0;
    }

    threshold = jcntrl_data_array_get_value(fary, 0);
  } else {
    threshold = g->default_vof_threshold;
  }

  jcntrl_mask_geometry_function_set_geometry(func, geom);
  jcntrl_mask_geometry_function_set_vof_threshold(func, threshold);
  return 1;
}

JCNTRL_VIRTUAL_WRAP(jcntrl_mask_geometry, jcntrl_executive, process_update_data)

static void jcntrl_mask_geometry_init_func(jcntrl_shared_object_funcs *p)
{
  p->downcast = jcntrl_mask_geometry_downcast_v;
  p->initializer = jcntrl_mask_geometry_initializer;
  p->destructor = jcntrl_mask_geometry_destructor;
  p->allocator = jcntrl_mask_geometry_allocator;
  p->deleter = jcntrl_mask_geometry_deleter;
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_mask_geometry, jcntrl_executive,
                          fill_input_port_information);
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_mask_geometry, jcntrl_executive,
                          fill_output_port_information);
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_mask_geometry, jcntrl_executive,
                          process_update_data);
}

JCNTRL_SHARED_METADATA_INIT_DEFINE(jcntrl_mask_geometry,
                                   jcntrl_mask_geometry_init_func)

jcntrl_mask_geometry *jcntrl_mask_geometry_new(void)
{
  return jcntrl_shared_object_new(jcntrl_mask_geometry);
}

void jcntrl_mask_geometry_delete(jcntrl_mask_geometry *m)
{
  jcntrl_shared_object_delete(jcntrl_mask_geometry_object(m));
}

jcntrl_shared_object *jcntrl_mask_geometry_object(jcntrl_mask_geometry *m)
{
  return jcntrl_executive_object(jcntrl_mask_geometry_executive(m));
}

jcntrl_executive *jcntrl_mask_geometry_executive(jcntrl_mask_geometry *m)
{
  return &m->exec;
}

jcntrl_mask_geometry *jcntrl_mask_geometry_downcast(jcntrl_shared_object *obj)
{
  return jcntrl_shared_object_downcast(jcntrl_mask_geometry, obj);
}

jcntrl_input *jcntrl_mask_geometry_geometry_input(jcntrl_mask_geometry *m)
{
  return jcntrl_executive_input_port(jcntrl_mask_geometry_executive(m), 0);
}

jcntrl_input *jcntrl_mask_geometry_vof_threshold_input(jcntrl_mask_geometry *m)
{
  return jcntrl_executive_input_port(jcntrl_mask_geometry_executive(m), 1);
}

double jcntrl_mask_geometry_default_vof_threshold(jcntrl_mask_geometry *m)
{
  return m->default_vof_threshold;
}

void jcntrl_mask_geometry_set_default_vof_threshold(jcntrl_mask_geometry *m,
                                                    double value)
{
  m->default_vof_threshold = value;
}

jcntrl_comparator jcntrl_mask_geometry_comparator(jcntrl_mask_geometry *m)
{
  return m->vof_comp;
}

void jcntrl_mask_geometry_set_comparator(jcntrl_mask_geometry *m,
                                         jcntrl_comparator comp)
{
  m->vof_comp = comp;
}

jcntrl_shared_object *
jcntrl_mask_geometry_function_object(jcntrl_mask_geometry_function *f)
{
  return jcntrl_mask_function_object(&f->func);
}

jcntrl_mask_geometry_function *
jcntrl_mask_geometry_function_downcast(jcntrl_shared_object *obj)
{
  return jcntrl_shared_object_downcast(jcntrl_mask_geometry_function, obj);
}

jcntrl_geometry *
jcntrl_mask_geometry_function_geometry(jcntrl_mask_geometry_function *f)
{
  return f->geometry;
}

void jcntrl_mask_geometry_function_set_geometry(
  jcntrl_mask_geometry_function *f, jcntrl_geometry *geometry)
{
  if (f->geometry)
    jcntrl_geometry_delete(f->geometry);
  f->geometry = geometry;
  if (f->geometry)
    jcntrl_shared_object_take_ownership(jcntrl_geometry_object(geometry));
}

double
jcntrl_mask_geometry_function_vof_threshold(jcntrl_mask_geometry_function *f)
{
  return f->vof_threshold;
}

void jcntrl_mask_geometry_function_set_vof_threshold(
  jcntrl_mask_geometry_function *f, double value)
{
  f->vof_threshold = value;
}

jcntrl_comparator
jcntrl_mask_geometry_function_comparator(jcntrl_mask_geometry_function *f)
{
  return f->vof_comp;
}

void jcntrl_mask_geometry_function_set_comparator(
  jcntrl_mask_geometry_function *f, jcntrl_comparator comp)
{
  f->vof_comp = comp;
}
