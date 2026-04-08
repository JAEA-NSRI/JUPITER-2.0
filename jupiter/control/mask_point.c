#include "mask_point.h"
#include "cell.h"
#include "data_array.h"
#include "error.h"
#include "executive.h"
#include "executive_data.h"
#include "defs.h"
#include "field_variable.h"
#include "information.h"
#include "input.h"
#include "mask_function.h"
#include "mask_function_priv.h"
#include "output.h"
#include "shared_object.h"
#include "shared_object_priv.h"

struct jcntrl_mask_point_function
{
  jcntrl_mask_function func;
  double pnt[3];
};
#define jcntrl_mask_point_function__ancestor jcntrl_mask_function
#define jcntrl_mask_point_function__dnmem func.jcntrl_mask_function__dnmem
JCNTRL_VTABLE_NONE(jcntrl_mask_point_function);

static jcntrl_mask_point_function *
jcntrl_mask_point_function_downcast_impl(jcntrl_shared_object *obj)
{
  return JCNTRL_DOWNCAST_IMPL(jcntrl_mask_point_function, obj);
}

static void *jcntrl_mask_point_function_downcast_v(jcntrl_shared_object *obj)
{
  return jcntrl_mask_point_function_downcast_impl(obj);
}

static int jcntrl_mask_point_function_initializer(jcntrl_shared_object *obj)
{
  jcntrl_mask_point_function *f;
  f = jcntrl_mask_point_function_downcast_impl(obj);
  f->pnt[0] = 0.0;
  f->pnt[1] = 0.0;
  f->pnt[2] = 0.0;
  return 1;
}

static void jcntrl_mask_point_function_desctructor(jcntrl_shared_object *obj)
{
  /* nop */
}

static jcntrl_shared_object *jcntrl_mask_point_function_allocator(void)
{
  jcntrl_mask_point_function *f;
  f = jcntrl_shared_object_default_allocator(jcntrl_mask_point_function);
  return f ? jcntrl_mask_point_function_object(f) : NULL;
}

static void jcntrl_mask_point_function_deleter(jcntrl_shared_object *obj)
{
  jcntrl_shared_object_default_deleter(obj);
}

static int jcntrl_mask_point_function_eval_impl(jcntrl_shared_object *obj,
                                                jcntrl_cell *c, int i, int j,
                                                int k)
{
  jcntrl_mask_point_function *f;
  f = jcntrl_mask_point_function_downcast_impl(obj);
  return !jcntrl_cell_contain(c, f->pnt[0], f->pnt[1], f->pnt[2]);
}

JCNTRL_VIRTUAL_WRAP(jcntrl_mask_point_function, jcntrl_mask_function, eval)

static void jcntrl_mask_point_function_init_func(jcntrl_shared_object_funcs *p)
{
  p->downcast = jcntrl_mask_point_function_downcast_v;
  p->initializer = jcntrl_mask_point_function_initializer;
  p->destructor = jcntrl_mask_point_function_desctructor;
  p->allocator = jcntrl_mask_point_function_allocator;
  p->deleter = jcntrl_mask_point_function_deleter;
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_mask_point_function, jcntrl_mask_function,
                          eval);
}

JCNTRL_SHARED_METADATA_INIT_DEFINE(jcntrl_mask_point_function,
                                   jcntrl_mask_point_function_init_func)

//---

struct jcntrl_mask_point
{
  jcntrl_executive exec;
  double default_pnt[3];
};
#define jcntrl_mask_point__ancestor jcntrl_executive
#define jcntrl_mask_point__dnmem exec.jcntrl_executive__dnmem
JCNTRL_VTABLE_NONE(jcntrl_mask_point);

static jcntrl_mask_point *
jcntrl_mask_point_downcast_impl(jcntrl_shared_object *obj)
{
  return JCNTRL_DOWNCAST_IMPL(jcntrl_mask_point, obj);
}

static void *jcntrl_mask_point_downcast_v(jcntrl_shared_object *obj)
{
  return jcntrl_mask_point_downcast_impl(obj);
}

static int jcntrl_mask_point_initializer(jcntrl_shared_object *obj)
{
  jcntrl_mask_point *p;
  jcntrl_executive *exec;
  jcntrl_input *input;
  jcntrl_output *output;

  p = jcntrl_mask_point_downcast_impl(obj);
  p->default_pnt[0] = 0.0;
  p->default_pnt[1] = 0.0;
  p->default_pnt[2] = 0.0;

  exec = jcntrl_mask_point_executive(p);
  input = jcntrl_executive_get_input(exec);
  if (!jcntrl_input_set_number_of_ports(input, 3))
    return 0;

  output = jcntrl_executive_get_output(exec);
  if (!jcntrl_output_set_number_of_ports(output, 1))
    return 0;

  return 1;
}

static void jcntrl_mask_point_destructor(jcntrl_shared_object *obj)
{
  /* nop */
}

static jcntrl_shared_object *jcntrl_mask_point_allocator(void)
{
  jcntrl_mask_point *p;
  p = jcntrl_shared_object_default_allocator(jcntrl_mask_point);
  return p ? jcntrl_mask_point_object(p) : NULL;
}

static void jcntrl_mask_point_deleter(jcntrl_shared_object *obj)
{
  jcntrl_shared_object_default_deleter(obj);
}

static int jcntrl_mask_point_fill_input_port_information_impl(
  jcntrl_shared_object *data, int index, jcntrl_input *input)
{
  jcntrl_information *info;
  info = jcntrl_input_information(input);
  if (!jcntrl_information_set_datatype(info, JCNTRL_INFO_REQUIRED_DATATYPE,
                                       JCNTRL_DATATYPE_FIELD_VAR))
    return 0;
  return 1;
}

JCNTRL_VIRTUAL_WRAP(jcntrl_mask_point, jcntrl_executive,
                    fill_input_port_information)

static int jcntrl_mask_point_fill_output_port_information_impl(
  jcntrl_shared_object *data, int index, jcntrl_output *output)
{
  jcntrl_information *info;
  info = jcntrl_output_information(output);
  if (!jcntrl_information_set_objecttype(info, JCNTRL_INFO_DATATYPE,
                                         jcntrl_mask_point_function))
    return 0;
  return 1;
}

JCNTRL_VIRTUAL_WRAP(jcntrl_mask_point, jcntrl_executive,
                    fill_output_port_information)

static int jcntrl_mask_point_get_pntval(double *dest, jcntrl_input *input,
                                        double defval)
{
  jcntrl_information *info;
  jcntrl_shared_object *obj;
  jcntrl_field_variable *fvar;
  jcntrl_data_array *ary;
  JCNTRL_ASSERT(input);

  info = jcntrl_input_upstream_information(input);
  if (!info) {
    *dest = defval;
    return 1;
  }

  obj = jcntrl_information_get_object(info, JCNTRL_INFO_DATA_OBJECT);
  if (!obj)
    return 0;

  fvar = jcntrl_field_variable_downcast(obj);
  if (!fvar)
    return 0;

  ary = jcntrl_field_variable_array(fvar);
  if (!ary || jcntrl_data_array_get_ntuple(ary) != 1) {
    jcntrl_raise_argument_error(
      __FILE__, __LINE__,
      "Number of elements in input field variable must be 1");
    return 0;
  }

  *dest = jcntrl_data_array_get_value(ary, 0);
  return 1;
}

static int jcntrl_mask_point_process_update_data_impl(
  jcntrl_information *request, jcntrl_input *input_head,
  jcntrl_output *output_head, jcntrl_shared_object *obj)
{
  jcntrl_mask_point *p;
  jcntrl_mask_point_function *f;
  jcntrl_information *info;
  jcntrl_shared_object *oobj;
  jcntrl_input *input;
  jcntrl_output *output;
  double pntval;
  double defval;

  p = jcntrl_mask_point_downcast_impl(obj);

  output = jcntrl_output_next_port(output_head);
  JCNTRL_ASSERT(output);

  info = jcntrl_output_information(output);
  JCNTRL_ASSERT(info);

  oobj = jcntrl_information_get_object(info, JCNTRL_INFO_DATA_OBJECT);
  JCNTRL_ASSERT(oobj);

  f = jcntrl_mask_point_function_downcast(oobj);
  JCNTRL_ASSERT(f);

  input = jcntrl_mask_point_x_point_port(p);
  defval = jcntrl_mask_point_default_point_x(p);
  if (!jcntrl_mask_point_get_pntval(&pntval, input, defval))
    return 0;

  jcntrl_mask_point_function_set_point_x(f, pntval);

  input = jcntrl_mask_point_y_point_port(p);
  defval = jcntrl_mask_point_default_point_y(p);
  if (!jcntrl_mask_point_get_pntval(&pntval, input, defval))
    return 0;

  jcntrl_mask_point_function_set_point_y(f, pntval);

  input = jcntrl_mask_point_z_point_port(p);
  defval = jcntrl_mask_point_default_point_z(p);
  if (!jcntrl_mask_point_get_pntval(&pntval, input, defval))
    return 0;

  jcntrl_mask_point_function_set_point_z(f, pntval);

  return 1;
}

JCNTRL_VIRTUAL_WRAP(jcntrl_mask_point, jcntrl_executive, process_update_data)

static void jcntrl_mask_point_init_func(jcntrl_shared_object_funcs *p)
{
  p->downcast = jcntrl_mask_point_downcast_v;
  p->initializer = jcntrl_mask_point_initializer;
  p->destructor = jcntrl_mask_point_destructor;
  p->allocator = jcntrl_mask_point_allocator;
  p->deleter = jcntrl_mask_point_deleter;
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_mask_point, jcntrl_executive,
                          fill_input_port_information);
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_mask_point, jcntrl_executive,
                          fill_output_port_information);
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_mask_point, jcntrl_executive,
                          process_update_data);
}

JCNTRL_SHARED_METADATA_INIT_DEFINE(jcntrl_mask_point,
                                   jcntrl_mask_point_init_func)

//----

jcntrl_mask_point *jcntrl_mask_point_new(void)
{
  return jcntrl_shared_object_new(jcntrl_mask_point);
}

void jcntrl_mask_point_delete(jcntrl_mask_point *p)
{
  jcntrl_shared_object_delete(jcntrl_mask_point_object(p));
}

jcntrl_shared_object *jcntrl_mask_point_object(jcntrl_mask_point *p)
{
  return jcntrl_executive_object(jcntrl_mask_point_executive(p));
}

jcntrl_executive *jcntrl_mask_point_executive(jcntrl_mask_point *p)
{
  return &p->exec;
}

jcntrl_mask_point *jcntrl_mask_point_downcast(jcntrl_shared_object *obj)
{
  return jcntrl_shared_object_downcast(jcntrl_mask_point, obj);
}

void jcntrl_mask_point_set_default_point_x(jcntrl_mask_point *p, double x)
{
  JCNTRL_ASSERT(p);
  p->default_pnt[0] = x;
}

void jcntrl_mask_point_set_default_point_y(jcntrl_mask_point *p, double y)
{
  JCNTRL_ASSERT(p);
  p->default_pnt[1] = y;
}

void jcntrl_mask_point_set_default_point_z(jcntrl_mask_point *p, double z)
{
  JCNTRL_ASSERT(p);
  p->default_pnt[2] = z;
}

double jcntrl_mask_point_default_point_x(jcntrl_mask_point *p)
{
  JCNTRL_ASSERT(p);
  return p->default_pnt[0];
}

double jcntrl_mask_point_default_point_y(jcntrl_mask_point *p)
{
  JCNTRL_ASSERT(p);
  return p->default_pnt[1];
}

double jcntrl_mask_point_default_point_z(jcntrl_mask_point *p)
{
  JCNTRL_ASSERT(p);
  return p->default_pnt[2];
}

jcntrl_input *jcntrl_mask_point_x_point_port(jcntrl_mask_point *p)
{
  JCNTRL_ASSERT(p);
  return jcntrl_executive_input_port(jcntrl_mask_point_executive(p), 0);
}

jcntrl_input *jcntrl_mask_point_y_point_port(jcntrl_mask_point *p)
{
  JCNTRL_ASSERT(p);
  return jcntrl_executive_input_port(jcntrl_mask_point_executive(p), 1);
}

jcntrl_input *jcntrl_mask_point_z_point_port(jcntrl_mask_point *p)
{
  JCNTRL_ASSERT(p);
  return jcntrl_executive_input_port(jcntrl_mask_point_executive(p), 2);
}

jcntrl_shared_object *
jcntrl_mask_point_function_object(jcntrl_mask_point_function *p)
{
  return jcntrl_mask_function_object(&p->func);
}

jcntrl_mask_point_function *
jcntrl_mask_point_function_downcast(jcntrl_shared_object *obj)
{
  return jcntrl_shared_object_downcast(jcntrl_mask_point_function, obj);
}

void jcntrl_mask_point_function_set_point_x(jcntrl_mask_point_function *f,
                                            double x)
{
  JCNTRL_ASSERT(f);
  f->pnt[0] = x;
}

void jcntrl_mask_point_function_set_point_y(jcntrl_mask_point_function *f,
                                            double y)
{
  JCNTRL_ASSERT(f);
  f->pnt[1] = y;
}

void jcntrl_mask_point_function_set_point_z(jcntrl_mask_point_function *f,
                                            double z)
{
  JCNTRL_ASSERT(f);
  f->pnt[2] = z;
}

double jcntrl_mask_point_function_point_x(jcntrl_mask_point_function *f)
{
  JCNTRL_ASSERT(f);
  return f->pnt[0];
}

double jcntrl_mask_point_function_point_y(jcntrl_mask_point_function *f)
{
  JCNTRL_ASSERT(f);
  return f->pnt[1];
}

double jcntrl_mask_point_function_point_z(jcntrl_mask_point_function *f)
{
  JCNTRL_ASSERT(f);
  return f->pnt[2];
}
