
#include <jupiter/control/field_variable.h>
#include <jupiter/control/struct_data.h>
#include <jupiter/control/executive.h>
#include <jupiter/control/input.h>
#include <jupiter/control/output.h>
#include <jupiter/control/field_variable.h>
#include <jupiter/control/information.h>
#include <jupiter/control/defs.h>

#include "control_test_field_value_feeder.h"
#include "jupiter/control/data_array.h"
#include "jupiter/control/shared_object.h"
#include "jupiter/control/shared_object_priv.h"
#include "jupiter/control/static_array.h"
#include "jupiter/geometry/util.h"

static int fill_input(jcntrl_shared_object *data, int index,
                      jcntrl_input *input)
{
  return 1;
}

JCNTRL_VIRTUAL_WRAP_FN(field_value_feeder, fill_input, jcntrl_executive,
                       fill_input_port_information)

static int fill_output(jcntrl_shared_object *data, int index,
                       jcntrl_output *output)
{
  jcntrl_information *info;

  info = jcntrl_output_information(output);
  if (!info)
    return 0;

  if (!jcntrl_information_set_objecttype(info, JCNTRL_INFO_DATATYPE,
                                         jcntrl_field_variable))
    return 0;

  return 1;
}

JCNTRL_VIRTUAL_WRAP_FN(field_value_feeder, fill_output, jcntrl_executive,
                       fill_output_port_information)

static int upd_info(jcntrl_information *request, jcntrl_input *input,
                    jcntrl_output *output, jcntrl_shared_object *data)
{
  return 1;
}

JCNTRL_VIRTUAL_WRAP_FN(field_value_feeder, upd_info, jcntrl_executive,
                       process_update_information)

static int upd_extent(jcntrl_information *request, jcntrl_input *input,
                      jcntrl_output *output, jcntrl_shared_object *data)
{
  return 1;
}

JCNTRL_VIRTUAL_WRAP_FN(field_value_feeder, upd_extent, jcntrl_executive,
                       process_update_extent)

static int upd_data(jcntrl_information *request, jcntrl_input *input,
                    jcntrl_output *output, jcntrl_shared_object *data)
{
  jcntrl_information *info;
  jcntrl_shared_object *object;
  jcntrl_field_variable *fvar;
  jcntrl_data_array *d;
  jcntrl_double_array *ary;
  jcntrl_size_type ntuple;

  struct field_value_feeder *fvf;
  fvf = (struct field_value_feeder *)data;

  output = jcntrl_output_next_port(output);
  if (!output)
    return 0;

  info = jcntrl_output_information(output);
  if (!info)
    return 0;

  object = jcntrl_information_get_object(info, JCNTRL_INFO_DATA_OBJECT);
  if (!object)
    return 0;

  fvar = jcntrl_field_variable_downcast(object);
  if (!fvar)
    return 0;

  ary = NULL;
  d = jcntrl_field_variable_array(fvar);
  if (d) {
    jcntrl_shared_object *o;
    o = jcntrl_data_array_object(d);
    ary = jcntrl_double_array_downcast(o);
  }
  if (!ary) {
    ary = jcntrl_double_array_new();
    if (!ary)
      return 0;

    d = jcntrl_double_array_data(ary);
    jcntrl_field_variable_set_array(fvar, d);
    jcntrl_double_array_delete(ary); // Release ownership
  }

  ntuple = jcntrl_static_double_array_get_ntuple(&fvf->array);
  if (!jcntrl_double_array_resize(ary, ntuple))
    return 0;

  if (ntuple > 0) {
    d = jcntrl_static_double_array_data(&fvf->array);
    if (!jcntrl_double_array_copy(ary, d, ntuple, 0, 0))
      return 0;
  }

  return 1;
}

JCNTRL_VIRTUAL_WRAP_FN(field_value_feeder, upd_data, jcntrl_executive,
                       process_update_data)

static struct field_value_feeder *
field_value_feeder_dn(jcntrl_shared_object *obj)
{
  return JCNTRL_DOWNCAST_IMPL_T(field_value_feeder, struct field_value_feeder,
                                obj);
}

static void *field_value_feeder_dnv(jcntrl_shared_object *obj)
{
  return field_value_feeder_dn(obj);
}

static int field_value_feeder_init_impl(jcntrl_shared_object *obj)
{
  struct field_value_feeder *f;
  jcntrl_executive *exe;

  f = field_value_feeder_dn(obj);
  exe = &f->executive;

  jcntrl_static_double_array_init_base(&f->array, NULL, 0);

  if (!jcntrl_input_set_number_of_ports(jcntrl_executive_get_input(exe), 0))
    return 0;

  if (!jcntrl_output_set_number_of_ports(jcntrl_executive_get_output(exe), 1))
    return 0;

  return 1;
}

static void field_value_feeder_initf(jcntrl_shared_object_funcs *p)
{
  p->initializer = field_value_feeder_init_impl;
  p->downcast = field_value_feeder_dnv;
  JCNTRL_VIRTUAL_WRAP_SET(p, field_value_feeder, jcntrl_executive,
                          fill_input_port_information);
  JCNTRL_VIRTUAL_WRAP_SET(p, field_value_feeder, jcntrl_executive,
                          fill_output_port_information);
  JCNTRL_VIRTUAL_WRAP_SET(p, field_value_feeder, jcntrl_executive,
                          process_update_information);
  JCNTRL_VIRTUAL_WRAP_SET(p, field_value_feeder, jcntrl_executive,
                          process_update_extent);
  JCNTRL_VIRTUAL_WRAP_SET(p, field_value_feeder, jcntrl_executive,
                          process_update_data);
}

JCNTRL_SHARED_METADATA_INIT_DEFINE(field_value_feeder, field_value_feeder_initf)

int field_value_feeder_init(struct field_value_feeder *exe)
{
  return jcntrl_executive_init(&exe->executive, field_value_feeder_metadata_init());
}

void field_value_feeder_clean(struct field_value_feeder *exe)
{
  jcntrl_executive_delete(&exe->executive);
}
