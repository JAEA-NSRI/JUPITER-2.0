
#include "fv_time.h"
#include "control/defs.h"
#include "control/executive.h"
#include "control/executive_data.h"
#include "control/information.h"
#include "control/input.h"
#include "control/output.h"
#include "control/field_variable.h"
#include "control/shared_object.h"
#include "control/shared_object_priv.h"
#include "struct.h"
#include "csvutil.h"

#define jupiter_fv_time__ancestor jcntrl_executive
#define jupiter_fv_delta_t__ancestor jcntrl_executive
#define jupiter_fv_time__dnmem fv_time.jcntrl_executive__dnmem
#define jupiter_fv_delta_t__dnmem fv_delta_t.jcntrl_executive__dnmem
JCNTRL_VTABLE_NONE(jupiter_fv_time);
JCNTRL_VTABLE_NONE(jupiter_fv_delta_t);

static domain *jupiter_fv_time_downcast_impl(jcntrl_shared_object *obj)
{
  return JCNTRL_DOWNCAST_IMPL_T(jupiter_fv_time, domain, obj);
}

static domain *jupiter_fv_delta_t_downcast_impl(jcntrl_shared_object *obj)
{
  return JCNTRL_DOWNCAST_IMPL_T(jupiter_fv_delta_t, domain, obj);
}

static int jupiter_fv_time_initializer(jcntrl_shared_object *obj)
{
  jcntrl_input *input;
  jcntrl_output *output;
  domain *cdo;

  cdo = jupiter_fv_time_downcast_impl(obj);
  input = jcntrl_executive_get_input(&cdo->fv_time);
  output = jcntrl_executive_get_output(&cdo->fv_time);

  if (!jcntrl_input_set_number_of_ports(input, 0)) {
    return 0;
  }
  if (!jcntrl_output_set_number_of_ports(output, 1)) {
    return 0;
  }
  if (!jcntrl_executive_set_name(&cdo->fv_time, CONTROL_KEYCHAR_FVAR "time")) {
    return 0;
  }
  return 1;
}

static void jupiter_fv_time_destructor(jcntrl_shared_object *obj) {}

static int jupiter_fv_delta_t_initializer(jcntrl_shared_object *obj)
{
  jcntrl_input *input;
  jcntrl_output *output;
  domain *cdo;

  cdo = jupiter_fv_delta_t_downcast_impl(obj);
  input = jcntrl_executive_get_input(&cdo->fv_delta_t);
  output = jcntrl_executive_get_output(&cdo->fv_delta_t);

  if (!jcntrl_input_set_number_of_ports(input, 0)) {
    return 0;
  }
  if (!jcntrl_output_set_number_of_ports(output, 1)) {
    return 0;
  }
  if (!jcntrl_executive_set_name(&cdo->fv_delta_t,
                                 CONTROL_KEYCHAR_FVAR "delta-t")) {
    return 0;
  }
  return 1;
}

static void jupiter_fv_delta_t_destructor(jcntrl_shared_object *obj) {}

int jupiter_fv_time_init(domain *cdo)
{
  CSVASSERT(cdo);

  return jcntrl_executive_init(&cdo->fv_time, jupiter_fv_time_metadata_init());
}

int jupiter_fv_delta_t_init(domain *cdo)
{
  CSVASSERT(cdo);

  return jcntrl_executive_init(&cdo->fv_delta_t,
                               jupiter_fv_delta_t_metadata_init());
}

void jupiter_fv_time_clean(domain *cdo)
{
  jcntrl_executive_delete(&cdo->fv_time);
}

void jupiter_fv_delta_t_clean(domain *cdo)
{
  jcntrl_executive_delete(&cdo->fv_delta_t);
}

static int
jupiter_fv_time_fill_input_port_information_impl(jcntrl_shared_object *obj,
                                                 int index, jcntrl_input *input)
{
  return 1;
}

JCNTRL_VIRTUAL_WRAP(jupiter_fv_time, jcntrl_executive,
                    fill_input_port_information)

static int jupiter_fv_delta_t_fill_input_port_information_impl(
  jcntrl_shared_object *obj, int index, jcntrl_input *input)
{
  return 1;
}

JCNTRL_VIRTUAL_WRAP(jupiter_fv_delta_t, jcntrl_executive,
                    fill_input_port_information)

static int jupiter_fv_domain_output_port_information(jcntrl_output *output)
{
  jcntrl_information *info;

  info = jcntrl_output_information(output);
  if (!jcntrl_information_set_objecttype(info, JCNTRL_INFO_DATATYPE,
                                         jcntrl_field_variable)) {
    return 0;
  }
  return 1;
}

static int jupiter_fv_time_fill_output_port_information_impl(
  jcntrl_shared_object *obj, int index, jcntrl_output *output)
{
  return jupiter_fv_domain_output_port_information(output);
}

JCNTRL_VIRTUAL_WRAP(jupiter_fv_time, jcntrl_executive,
                    fill_output_port_information)

static int jupiter_fv_delta_t_fill_output_port_information_impl(
  jcntrl_shared_object *obj, int index, jcntrl_output *output)
{
  return jupiter_fv_domain_output_port_information(output);
}

JCNTRL_VIRTUAL_WRAP(jupiter_fv_delta_t, jcntrl_executive,
                    fill_output_port_information)

static int jupiter_fv_set_output_double(jcntrl_output *output, double value)
{
  jcntrl_information *info;
  jcntrl_shared_object *object;
  jcntrl_field_variable *fvar;

  info = jcntrl_output_information(output);
  if (!info) {
    return 0;
  }

  object = jcntrl_information_get_object(info, JCNTRL_INFO_DATA_OBJECT);
  if (!object) {
    return 0;
  }

  fvar = jcntrl_field_variable_downcast(object);
  if (!fvar) {
    return 0;
  }

  if (!jcntrl_field_variable_set_value(fvar, value)) {
    return 0;
  }

  return 1;
}

static int jupiter_fv_time_process_update_data_impl(jcntrl_information *request,
                                                    jcntrl_input *input,
                                                    jcntrl_output *output,
                                                    jcntrl_shared_object *obj)
{
  domain *cdo;

  cdo = jupiter_fv_time_downcast_impl(obj);

  output = jcntrl_output_next_port(output);
  if (!output) {
    return 0;
  }

  return jupiter_fv_set_output_double(output, cdo->time);
}

JCNTRL_VIRTUAL_WRAP(jupiter_fv_time, jcntrl_executive, process_update_data)

static int jupiter_fv_delta_t_process_update_data_impl(
  jcntrl_information *request, jcntrl_input *input, jcntrl_output *output,
  jcntrl_shared_object *obj)
{
  domain *cdo;

  cdo = jupiter_fv_delta_t_downcast_impl(obj);

  output = jcntrl_output_next_port(output);
  if (!output) {
    return 0;
  }

  return jupiter_fv_set_output_double(output, cdo->dt);
}

JCNTRL_VIRTUAL_WRAP(jupiter_fv_delta_t, jcntrl_executive, process_update_data)

domain *jupiter_fv_time_downcast(jcntrl_executive *exe)
{
  return (domain *)
    jcntrl_shared_object_downcast_by_meta(jupiter_fv_time_metadata_init(),
                                          &exe->object);
}

domain *jupiter_fv_delta_t_downcast(jcntrl_executive *exe)
{
  return (domain *)
    jcntrl_shared_object_downcast_by_meta(jupiter_fv_delta_t_metadata_init(),
                                          &exe->object);
}

static void *jupiter_fv_time_downcast_v(jcntrl_shared_object *obj)
{
  return jupiter_fv_time_downcast_impl(obj);
}

static void *jupiter_fv_delta_t_downcast_v(jcntrl_shared_object *obj)
{
  return jupiter_fv_delta_t_downcast_impl(obj);
}

static void jupiter_fv_time_initf(jcntrl_shared_object_funcs *p)
{
  p->initializer = jupiter_fv_time_initializer;
  p->destructor = jupiter_fv_time_destructor;
  p->downcast = jupiter_fv_time_downcast_v;
  JCNTRL_VIRTUAL_WRAP_SET(p, jupiter_fv_time, jcntrl_executive,
                          fill_input_port_information);
  JCNTRL_VIRTUAL_WRAP_SET(p, jupiter_fv_time, jcntrl_executive,
                          fill_output_port_information);
  JCNTRL_VIRTUAL_WRAP_SET(p, jupiter_fv_time, jcntrl_executive,
                          process_update_data);
}

static void jupiter_fv_delta_t_initf(jcntrl_shared_object_funcs *p)
{
  p->initializer = jupiter_fv_delta_t_initializer;
  p->destructor = jupiter_fv_delta_t_destructor;
  p->downcast = jupiter_fv_delta_t_downcast_v;
  JCNTRL_VIRTUAL_WRAP_SET(p, jupiter_fv_delta_t, jcntrl_executive,
                          fill_input_port_information);
  JCNTRL_VIRTUAL_WRAP_SET(p, jupiter_fv_delta_t, jcntrl_executive,
                          fill_output_port_information);
  JCNTRL_VIRTUAL_WRAP_SET(p, jupiter_fv_delta_t, jcntrl_executive,
                          process_update_data);
}

JCNTRL_SHARED_METADATA_INIT_DEFINE(jupiter_fv_time, jupiter_fv_time_initf)
JCNTRL_SHARED_METADATA_INIT_DEFINE(jupiter_fv_delta_t, jupiter_fv_delta_t_initf)
