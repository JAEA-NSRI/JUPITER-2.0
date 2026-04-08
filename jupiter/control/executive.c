#include "executive.h"
#include "defs.h"
#include "error.h"
#include "field_function.h"
#include "field_object.h"
#include "field_variable.h"
#include "geometry.h"
#include "global.h"
#include "grid_data.h"
#include "information.h"
#include "input.h"
#include "logical_variable.h"
#include "mask_data.h"
#include "mask_function.h"
#include "mask_object.h"
#include "output.h"
#include "executive_data.h"

#include "shared_object.h"
#include "shared_object_priv.h"
#include "struct_data.h"

#include <stdlib.h>
#include <string.h>

static void jcntrl_input_init(jcntrl_input *input, jcntrl_executive *exe);

static void jcntrl_output_init(jcntrl_output *output, jcntrl_executive *exe);

static jcntrl_executive *
jcntrl_executive_downcast_impl(jcntrl_shared_object *obj)
{
  return JCNTRL_DOWNCAST_IMPL(jcntrl_executive, obj);
}

static void *jcntrl_executive_downcast_v(jcntrl_shared_object *obj)
{
  return jcntrl_executive_downcast_impl(obj);
}

static int jcntrl_executive_initializer(jcntrl_shared_object *obj)
{
  jcntrl_executive *exe;
  exe = jcntrl_executive_downcast_impl(obj);
  exe->lname = 0;
  exe->name = NULL;
  jcntrl_input_init(&exe->input_head, exe);
  jcntrl_output_init(&exe->output_head, exe);
  return 1;
}

static void jcntrl_executive_destructor(jcntrl_shared_object *obj)
{
  jcntrl_input *input;
  jcntrl_output *output;
  jcntrl_executive *exe;

  exe = jcntrl_executive_downcast_impl(obj);

  input = jcntrl_executive_get_input(exe);
  output = jcntrl_executive_get_output(exe);

  jcntrl_input_set_number_of_ports(input, 0);
  jcntrl_output_set_number_of_ports(output, 0);

  free(exe->name);
  exe->name = NULL;
  exe->lname = 0;
}

struct jcntrl_executive_process_request_args
{
  jcntrl_information *info;
  int ret;
};

void jcntrl_executive_process_request__wrapper(jcntrl_shared_object *obj,
                                               void *p,
                                               jcntrl_process_request *func)
{
  jcntrl_input *input;
  jcntrl_output *output;
  jcntrl_executive *exe;
  struct jcntrl_executive_process_request_args *args = p;
  exe = jcntrl_executive_downcast_impl(obj);
  input = jcntrl_executive_get_input(exe);
  output = jcntrl_executive_get_output(exe);
  args->ret = func(args->info, input, output, obj);
}

void jcntrl_executive_process_update_information__wrapper(
  jcntrl_shared_object *obj, void *p, jcntrl_update_information *func)
{
  jcntrl_input *input;
  jcntrl_output *output;
  jcntrl_executive *exe;
  struct jcntrl_executive_process_request_args *args = p;
  exe = jcntrl_executive_downcast_impl(obj);
  input = jcntrl_executive_get_input(exe);
  output = jcntrl_executive_get_output(exe);
  args->ret = func(args->info, input, output, obj);
}

void jcntrl_executive_process_update_extent__wrapper(jcntrl_shared_object *obj,
                                                     void *p,
                                                     jcntrl_update_extent *func)
{
  jcntrl_input *input;
  jcntrl_output *output;
  jcntrl_executive *exe;
  struct jcntrl_executive_process_request_args *args = p;
  exe = jcntrl_executive_downcast_impl(obj);
  input = jcntrl_executive_get_input(exe);
  output = jcntrl_executive_get_output(exe);
  args->ret = func(args->info, input, output, obj);
}

void jcntrl_executive_process_update_data__wrapper(jcntrl_shared_object *obj,
                                                   void *p,
                                                   jcntrl_update_data *func)
{
  jcntrl_input *input;
  jcntrl_output *output;
  jcntrl_executive *exe;
  struct jcntrl_executive_process_request_args *args = p;
  exe = jcntrl_executive_downcast_impl(obj);
  input = jcntrl_executive_get_input(exe);
  output = jcntrl_executive_get_output(exe);
  args->ret = func(args->info, input, output, obj);
}

struct jcntrl_executive_fill_input_ports_args
{
  int index;
  jcntrl_input *port;
  int ret;
};

void jcntrl_executive_fill_input_port_information__wrapper(
  jcntrl_shared_object *obj, void *p, jcntrl_fill_input_port_information *func)
{
  struct jcntrl_executive_fill_input_ports_args *args = p;
  args->ret = func(obj, args->index, args->port);
}

struct jcntrl_executive_fill_output_ports_args
{
  int index;
  jcntrl_output *port;
  int ret;
};

void jcntrl_executive_fill_output_port_information__wrapper(
  jcntrl_shared_object *obj, void *p, jcntrl_fill_output_port_information *func)
{
  struct jcntrl_executive_fill_output_ports_args *args = p;
  args->ret = func(obj, args->index, args->port);
}

int jcntrl_executive_process_request__super(
  const jcntrl_shared_object_data *ancestor, jcntrl_information *request)
{
  struct jcntrl_executive_process_request_args args = {
    .info = request,
    .ret = 0,
  };
  jcntrl_shared_object_call_super(ancestor, jcntrl_executive, process_request,
                                  &args);
  return args.ret;
}

int jcntrl_executive_process_update_information__super(
  const jcntrl_shared_object_data *ancestor, jcntrl_information *request)
{
  struct jcntrl_executive_process_request_args args = {
    .info = request,
    .ret = 0,
  };
  jcntrl_shared_object_call_super(ancestor, jcntrl_executive,
                                  process_update_information, &args);
  return args.ret;
}

int jcntrl_executive_process_update_extent__super(
  const jcntrl_shared_object_data *ancestor, jcntrl_information *request)
{
  struct jcntrl_executive_process_request_args args = {
    .info = request,
    .ret = 0,
  };
  jcntrl_shared_object_call_super(ancestor, jcntrl_executive,
                                  process_update_extent, &args);
  return args.ret;
}

int jcntrl_executive_process_update_data__super(
  const jcntrl_shared_object_data *ancestor, jcntrl_information *request)
{
  struct jcntrl_executive_process_request_args args = {
    .info = request,
    .ret = 0,
  };
  jcntrl_shared_object_call_super(ancestor, jcntrl_executive,
                                  process_update_data, &args);
  return args.ret;
}

int jcntrl_executive_process_fill_input_port_information__super(
  const jcntrl_shared_object_data *ancestor, int index, jcntrl_input *port)
{
  struct jcntrl_executive_fill_input_ports_args args = {
    .index = index,
    .port = port,
    .ret = 0,
  };
  jcntrl_shared_object_call_super(ancestor, jcntrl_executive,
                                  fill_input_port_information, &args);
  return args.ret;
}

int jcntrl_executive_process_fill_output_port_information__super(
  const jcntrl_shared_object_data *ancestor, int index, jcntrl_output *port)
{
  struct jcntrl_executive_fill_output_ports_args args = {
    .index = index,
    .port = port,
    .ret = 0,
  };
  jcntrl_shared_object_call_super(ancestor, jcntrl_executive,
                                  fill_output_port_information, &args);
  return args.ret;
}

static int jcntrl_executive_process_request_impl(jcntrl_information *request,
                                                 jcntrl_input *input,
                                                 jcntrl_output *output,
                                                 jcntrl_shared_object *obj)
{
  jcntrl_executive *exe;

  exe = jcntrl_executive_downcast_impl(obj);
  if (!jcntrl_executive_process_update_information(exe, request)) {
    return 0;
  }
  if (!jcntrl_executive_process_update_extent(exe, request)) {
    return 0;
  }
  if (!jcntrl_executive_process_update_data(exe, request)) {
    return 0;
  }

  return 1;
}

JCNTRL_VIRTUAL_WRAP(jcntrl_executive, jcntrl_executive, process_request)

static int jcntrl_executive_process_update_information_impl(
  jcntrl_information *request, jcntrl_input *input, jcntrl_output *output,
  jcntrl_shared_object *obj)
{
  return 1;
}

JCNTRL_VIRTUAL_WRAP(jcntrl_executive, jcntrl_executive,
                    process_update_information)

static int jcntrl_executive_process_update_extent_impl(
  jcntrl_information *request, jcntrl_input *input, jcntrl_output *output,
  jcntrl_shared_object *obj)
{
  return 1;
}

JCNTRL_VIRTUAL_WRAP(jcntrl_executive, jcntrl_executive, process_update_extent)

static int jcntrl_executive_process_update_data_impl(
  jcntrl_information *request, jcntrl_input *input, jcntrl_output *output,
  jcntrl_shared_object *obj)
{
  return 1;
}

JCNTRL_VIRTUAL_WRAP(jcntrl_executive, jcntrl_executive, process_update_data)

static int jcntrl_executive_fill_input_port_information_impl(
  jcntrl_shared_object *obj, int index, jcntrl_input *input)
{
  return 1;
}

JCNTRL_VIRTUAL_WRAP(jcntrl_executive, jcntrl_executive,
                    fill_input_port_information)

static int jcntrl_executive_fill_output_port_information_impl(
  jcntrl_shared_object *object, int index, jcntrl_output *output)
{
  return 1;
}

JCNTRL_VIRTUAL_WRAP(jcntrl_executive, jcntrl_executive,
                    fill_output_port_information)

static void jcntrl_executive_init_func(jcntrl_shared_object_funcs *p)
{
  p->initializer = jcntrl_executive_initializer;
  p->destructor = jcntrl_executive_destructor;
  p->allocator = NULL;
  p->deleter = NULL;
  p->downcast = jcntrl_executive_downcast_v;
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_executive, jcntrl_executive,
                          process_request);
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_executive, jcntrl_executive,
                          process_update_information);
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_executive, jcntrl_executive,
                          process_update_extent);
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_executive, jcntrl_executive,
                          process_update_data);
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_executive, jcntrl_executive,
                          fill_input_port_information);
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_executive, jcntrl_executive,
                          fill_output_port_information);
}

JCNTRL_SHARED_METADATA_INIT_DEFINE(jcntrl_executive, jcntrl_executive_init_func)

jcntrl_executive *jcntrl_executive_new(int executive_id)
{
  const jcntrl_shared_object_data *cls;
  jcntrl_executive *exe;

  cls = jcntrl_executive_get_from_id(executive_id);
  return jcntrl_shared_object_new_by_meta(cls);
}

int jcntrl_executive_init(jcntrl_executive *exe,
                          const jcntrl_shared_object_data *cls)
{
  JCNTRL_ASSERT(exe);
  JCNTRL_ASSERT(cls);
  return jcntrl_shared_object_static_init(jcntrl_executive_object(exe), cls);
}

int jcntrl_executive_is_a(jcntrl_executive *exe,
                          const jcntrl_shared_object_data *cls)
{
  JCNTRL_ASSERT(exe);
  JCNTRL_ASSERT(cls);
  return jcntrl_shared_object_is_a_by_meta(cls, jcntrl_executive_object(exe));
}

jcntrl_executive *jcntrl_executive_downcast(jcntrl_shared_object *object)
{
  return jcntrl_shared_object_downcast(jcntrl_executive, object);
}

jcntrl_shared_object *jcntrl_executive_object(jcntrl_executive *exe)
{
  JCNTRL_ASSERT(exe);
  return &exe->object;
}

void jcntrl_executive_delete(jcntrl_executive *exe)
{
  jcntrl_shared_object_delete(jcntrl_executive_object(exe));
}

const char *jcntrl_executive_set_name(jcntrl_executive *exe, const char *name)
{
  int lname;
  char *new_name;

  JCNTRL_ASSERT(exe);

  lname = strlen(name);
  if (!exe->name || lname > exe->lname) {
    new_name = (char *)malloc((size_t)lname + 1);
    if (new_name) {
      strcpy(new_name, name);
      free(exe->name);
      exe->name = new_name;
      exe->lname = lname;
    }
  } else {
    strcpy(exe->name, name);
    new_name = exe->name;
  }
  return new_name;
}

const char *jcntrl_executive_get_name(jcntrl_executive *exe)
{
  JCNTRL_ASSERT(exe);
  return exe->name;
}

jcntrl_input *jcntrl_executive_get_input(jcntrl_executive *exe)
{
  JCNTRL_ASSERT(exe);
  return &exe->input_head;
}

jcntrl_output *jcntrl_executive_get_output(jcntrl_executive *exe)
{
  JCNTRL_ASSERT(exe);
  return &exe->output_head;
}

const char *jcntrl_executive_get_class_name(jcntrl_executive *exe)
{
  JCNTRL_ASSERT(exe);
  return jcntrl_shared_object_class_name(jcntrl_executive_object(exe));
}

int jcntrl_executive_process_request(jcntrl_executive *exe,
                                     jcntrl_information *request)
{
  JCNTRL_ASSERT(exe);

  struct jcntrl_executive_process_request_args args = {
    .info = request,
    .ret = 0,
  };
  jcntrl_shared_object_call_virtual(jcntrl_executive_object(exe),
                                    jcntrl_executive, process_request, &args);
  return args.ret;
}

int jcntrl_executive_process_update_information(jcntrl_executive *exe,
                                                jcntrl_information *request)
{
  JCNTRL_ASSERT(exe);

  if (jcntrl_information_has(request, JCNTRL_INFO_REQUEST_UPDATE_INFORMATION)) {
    struct jcntrl_executive_process_request_args args = {
      .info = request,
      .ret = 0,
    };
    jcntrl_shared_object_call_virtual(jcntrl_executive_object(exe),
                                      jcntrl_executive,
                                      process_update_information, &args);
    return args.ret;
  }
  return 1;
}

int jcntrl_executive_process_update_extent(jcntrl_executive *exe,
                                           jcntrl_information *request)
{
  JCNTRL_ASSERT(exe);

  if (jcntrl_information_has(request, JCNTRL_INFO_REQUEST_UPDATE_EXTENT)) {
    struct jcntrl_executive_process_request_args args = {
      .info = request,
      .ret = 0,
    };
    jcntrl_shared_object_call_virtual(jcntrl_executive_object(exe),
                                      jcntrl_executive, process_update_extent,
                                      &args);
    return args.ret;
  }
  return 1;
}

int jcntrl_executive_process_update_data(jcntrl_executive *exe,
                                         jcntrl_information *request)
{
  JCNTRL_ASSERT(exe);

  if (jcntrl_information_has(request, JCNTRL_INFO_REQUEST_UPDATE_DATA)) {
    struct jcntrl_executive_process_request_args args = {
      .info = request,
      .ret = 0,
    };
    jcntrl_shared_object_call_virtual(jcntrl_executive_object(exe),
                                      jcntrl_executive, process_update_data,
                                      &args);
    return args.ret;
  }
  return 1;
}

static int jcntrl_executive_fill_input_port_info(jcntrl_executive *exe,
                                                 int index, jcntrl_input *port)
{
  jcntrl_information *info;

  JCNTRL_ASSERT(exe);
  JCNTRL_ASSERT(!jcntrl_input_is_head(port));

  if (jcntrl_input_is_head(port)) {
    jcntrl_raise_argument_error(__FILE__, __LINE__,
                                "Input port must not be HEAD");
    return 0;
  }

  info = jcntrl_input_information(port);
  if (jcntrl_information_is_falsey(info, JCNTRL_INFO_REQUIREMENTS_FILLED)) {
    struct jcntrl_executive_fill_input_ports_args args = {
      .index = index,
      .port = port,
      .ret = 1,
    };
    jcntrl_shared_object_call_virtual(jcntrl_executive_object(exe),
                                      jcntrl_executive,
                                      fill_input_port_information, &args);
    if (args.ret) {
      args.ret =
        jcntrl_information_set_bool(info, JCNTRL_INFO_REQUIREMENTS_FILLED, 1);
    }
    return args.ret;
  }

  return 1;
}

static int jcntrl_executive_fill_output_port_info(jcntrl_executive *exe,
                                                  int index,
                                                  jcntrl_output *port)
{
  jcntrl_information *info;

  JCNTRL_ASSERT(exe);
  JCNTRL_ASSERT(!jcntrl_output_is_head(port));

  if (jcntrl_output_is_head(port)) {
    jcntrl_raise_argument_error(__FILE__, __LINE__,
                                "Output port must not be HEAD");
    return 0;
  }

  info = jcntrl_output_information(port);
  if (jcntrl_information_is_falsey(info, JCNTRL_INFO_REQUIREMENTS_FILLED)) {
    struct jcntrl_executive_fill_output_ports_args args = {
      .index = index,
      .port = port,
      .ret = 1,
    };
    jcntrl_shared_object_call_virtual(jcntrl_executive_object(exe),
                                      jcntrl_executive,
                                      fill_output_port_information, &args);
    if (args.ret) {
      args.ret =
        jcntrl_information_set_bool(info, JCNTRL_INFO_REQUIREMENTS_FILLED, 1);
    }
    return args.ret;
  }

  return 1;
}

static int jcntrl_executive_set_loop_marker(jcntrl_executive *exe)
{
  jcntrl_output *p;

  JCNTRL_ASSERT(exe);

  p = jcntrl_executive_get_output(exe);
  p = jcntrl_output_next_port(p);
  for (int index = 0; p; p = jcntrl_output_next_port(p), ++index) {
    jcntrl_information *info;

    if (!jcntrl_executive_fill_output_port_info(exe, index, p)) {
      return 0;
    }

    info = jcntrl_output_information(p);
    if (!jcntrl_information_set_bool(info, JCNTRL_INFO_LOOP_MARKER, 1)) {
      return 0;
    }
  }
  return 1;
}

static int jcntrl_executive_clear_loop_marker(jcntrl_executive *exe)
{
  int ret;
  jcntrl_output *p;

  ret = 1;
  p = jcntrl_executive_get_output(exe);
  p = jcntrl_output_next_port(p);
  for (int index = 0; p; p = jcntrl_output_next_port(p), ++index) {
    jcntrl_information *info;

    if (!jcntrl_executive_fill_output_port_info(exe, index, p)) {
      ret = 0;
      continue;
    }

    info = jcntrl_output_information(p);
    if (jcntrl_information_has(info, JCNTRL_INFO_LOOP_MARKER)) {
      if (!jcntrl_information_set_bool(info, JCNTRL_INFO_LOOP_MARKER, 0)) {
        ret = 0;
      }
    }
  }

  return ret;
}

static int jcntrl_executive_find_loop_marker_in_inputs(jcntrl_executive *exe)
{
  jcntrl_input *input, *p;

  input = jcntrl_executive_get_input(exe);
  p = jcntrl_input_next_port(input);
  for (int index = 0; p; p = jcntrl_input_next_port(p), ++index) {
    jcntrl_information *info;

    info = jcntrl_input_upstream_information(p);
    if (info) {
      if (jcntrl_information_is_true(info, JCNTRL_INFO_LOOP_MARKER)) {
        jcntrl_executive *upstream;

        upstream = jcntrl_input_upstream_executive(p);
        jcntrl_raise_loop_detected_error(__FILE__, __LINE__,
                                         jcntrl_executive_get_name(upstream),
                                         jcntrl_executive_get_name(exe));
        return 0;
      }
    }
  }
  return 1;
}

static jcntrl_datatype
jcntrl_executive_get_type_for_meta(jcntrl_datatype req_type,
                                   const jcntrl_shared_object_data *p)
{
  if (req_type == JCNTRL_DATATYPE_ANY_FIELD)
    if (jcntrl_shared_object_data_is_a(jcntrl_field_object_metadata_init(), p))
      return req_type;

  if (req_type == JCNTRL_DATATYPE_ANY_MASK)
    if (jcntrl_shared_object_data_is_a(jcntrl_mask_object_metadata_init(), p))
      return req_type;

  if (jcntrl_shared_object_data_is_a(jcntrl_grid_data_metadata_init(), p))
    return JCNTRL_DATATYPE_GRID;
  if (jcntrl_shared_object_data_is_a(jcntrl_geometry_metadata_init(), p))
    return JCNTRL_DATATYPE_GEOMETRY;
  if (jcntrl_shared_object_data_is_a(jcntrl_field_variable_metadata_init(), p))
    return JCNTRL_DATATYPE_FIELD_VAR;
  if (jcntrl_shared_object_data_is_a(jcntrl_field_function_metadata_init(), p))
    return JCNTRL_DATATYPE_FIELD_FUN;
  if (jcntrl_shared_object_data_is_a(jcntrl_mask_data_metadata_init(), p))
    return JCNTRL_DATATYPE_MASK;
  if (jcntrl_shared_object_data_is_a(jcntrl_mask_function_metadata_init(), p))
    return JCNTRL_DATATYPE_MASK_FUN;
  if (jcntrl_shared_object_data_is_a(jcntrl_logical_variable_metadata_init(),
                                     p))
    return JCNTRL_DATATYPE_LOGICAL_VAR;
  return JCNTRL_DATATYPE_INVALID;
}

/**
 * @brief Checks the input type of upstream output data
 * @param exe Executive to handle
 * @param index index of the @p input
 * @param input input port to handle
 * @return 1 if success or no upstream executive, 0 if other error.
 *
 * To call this function, upstream's information needs to be updated.
 * This function does nothing for that.
 */
static int jcntrl_executive_check_input_type(jcntrl_executive *exe, int index,
                                             jcntrl_input *input)
{
  jcntrl_executive *upstream;
  jcntrl_information *upinfo, *ininfo;
  jcntrl_datatype req_type, given_type;
  const jcntrl_shared_object_data *given_otype;

  JCNTRL_ASSERT(jcntrl_input_owner(input) == exe);
  JCNTRL_ASSERT(!jcntrl_input_is_head(input));

  upstream = jcntrl_input_upstream_executive(input);
  if (!upstream)
    return 1;

  upinfo = jcntrl_input_upstream_information(input);
  if ((!upinfo) || !jcntrl_information_has(upinfo, JCNTRL_INFO_DATATYPE)) {
    jcntrl_raise_argument_error(
      __FILE__, __LINE__,
      "Upstream executive did not set JCNTRL_INFO_DATATYPE attribute");
    return 0;
  }

  if (!jcntrl_executive_fill_input_port_info(exe, index, input)) {
    return 0;
  }

  ininfo = jcntrl_input_information(input);
  if ((!ininfo) ||
      !jcntrl_information_has(ininfo, JCNTRL_INFO_REQUIRED_DATATYPE)) {
    jcntrl_raise_argument_error(__FILE__, __LINE__,
                                "Input port does not have required datatype");
    return 0;
  }

  req_type =
    jcntrl_information_get_datatype(ininfo, JCNTRL_INFO_REQUIRED_DATATYPE);
  given_otype = jcntrl_information_get_objecttype(upinfo, JCNTRL_INFO_DATATYPE);
  given_type = jcntrl_executive_get_type_for_meta(req_type, given_otype);
  if (given_type == req_type)
    return 1;

  // error
  given_type =
    jcntrl_executive_get_type_for_meta(JCNTRL_DATATYPE_INVALID, given_otype);
  jcntrl_raise_upstream_type_error(__FILE__, __LINE__,
                                   jcntrl_executive_get_name(upstream),
                                   jcntrl_executive_get_name(exe), req_type,
                                   given_type, given_otype);
  return 0;
}

/**
 * @brief Make output data for specified port.
 * @param exe Executive to set
 * @param output output port to generate
 * @return 1 if success, 0 if failed.
 *
 * To make data, information must be up to date. This function does
 * not check this. This function won't report error if the information
 * is not porperly set. Just data will not be generated and return
 * success.
 */
static int jcntrl_executive_make_output_data(jcntrl_executive *exe,
                                             jcntrl_output *output)
{
  jcntrl_information *info;
  const jcntrl_shared_object_data *p;
  jcntrl_shared_object *obj;
  jcntrl_datatype type;

  JCNTRL_ASSERT(jcntrl_output_owner(output) == exe);
  JCNTRL_ASSERT(!jcntrl_output_is_head(output));

  info = jcntrl_output_information(output);
  if (!info) {
    return 1;
  }

  obj = NULL;
  p = jcntrl_information_get_objecttype(info, JCNTRL_INFO_DATATYPE);
  if (p) {
    obj = jcntrl_information_get_object(info, JCNTRL_INFO_DATA_OBJECT);
    if (!obj || !jcntrl_shared_object_is_a_by_meta(p, obj)) {
      obj = jcntrl_shared_object_new_by_meta(p);
      if (!obj)
        return 0;

      if (!jcntrl_information_set_object(info, JCNTRL_INFO_DATA_OBJECT, obj)) {
        jcntrl_shared_object_delete(obj);
        return 0;
      }
    }
  }
  return 1;
}

int jcntrl_executive_update_inputs(jcntrl_executive *exe,
                                   jcntrl_information *request)
{
  jcntrl_input *input, *p;

  input = jcntrl_executive_get_input(exe);
  if (jcntrl_information_has(request, JCNTRL_INFO_REQUEST_CHECK_LOOP)) {
    if (!jcntrl_executive_find_loop_marker_in_inputs(exe)) {
      return 0;
    }
  }

  p = jcntrl_input_next_port(input);
  for (int index = 0; p; p = jcntrl_input_next_port(p), ++index) {
    jcntrl_executive *upstream;

    upstream = jcntrl_input_upstream_executive(p);
    if (upstream) {
      if (!jcntrl_executive_update_by_request(upstream, request)) {
        return 0;
      }

      if (jcntrl_information_has(request,
                                 JCNTRL_INFO_REQUEST_UPDATE_INFORMATION)) {
        if (!jcntrl_executive_fill_input_port_info(exe, index, p)) {
          return 0;
        }

        if (!jcntrl_executive_check_input_type(exe, index, p)) {
          return 0;
        }
      }
    }
  }

  return 1;
}

int jcntrl_executive_update_by_request(jcntrl_executive *exe,
                                       jcntrl_information *request)
{
  int loop_checking;
  int ret;

  loop_checking = 0;
  if (jcntrl_information_has(request, JCNTRL_INFO_REQUEST_CHECK_LOOP)) {
    if (!jcntrl_executive_set_loop_marker(exe)) {
      return 0;
    }
    loop_checking = 1;
  }

  ret = jcntrl_executive_update_inputs(exe, request);

  if (ret) {
    jcntrl_output *op;

    op = jcntrl_output_next_port(jcntrl_executive_get_output(exe));
    for (int index = 0; op; op = jcntrl_output_next_port(op), ++index) {
      ret = jcntrl_executive_fill_output_port_info(exe, index, op);
      if (!ret) {
        break;
      }

      ret = jcntrl_executive_make_output_data(exe, op);
      if (!ret) {
        break;
      }
    }
  }

  if (ret) {
    ret = jcntrl_executive_process_request(exe, request);
  }

  if (loop_checking) {
    if (!jcntrl_executive_clear_loop_marker(exe)) {
      ret = 0;
    }
  }
  return ret;
}

jcntrl_input *jcntrl_executive_input_port(jcntrl_executive *exe, int nth)
{
  return jcntrl_input_at(jcntrl_executive_get_input(exe), nth);
}

jcntrl_output *jcntrl_executive_output_port(jcntrl_executive *exe, int nth)
{
  return jcntrl_output_at(jcntrl_executive_get_output(exe), nth);
}

/* Breaks formatter */
#undef jcntrl_executive_get_input_data_object_as
#undef jcntrl_executive_get_output_data_object_as
#undef jcntrl_input_get_data_object_as
#undef jcntrl_output_get_data_object_as
#undef jcntrl_input_get_data_object_at_as
#undef jcntrl_output_get_data_object_at_as

jcntrl_shared_object *
jcntrl_executive_get_input_data_object(jcntrl_executive *exec, int nth)
{
  return jcntrl_input_get_data_object_at(jcntrl_executive_get_input(exec), nth);
}

void *
jcntrl_executive_get_input_data_object_as(jcntrl_executive *exec, int nth,
                                          const jcntrl_shared_object_data *cls)
{
  return jcntrl_input_get_data_object_at_as(jcntrl_executive_get_input(exec),
                                            nth, cls);
}

jcntrl_shared_object *
jcntrl_executive_get_output_data_object(jcntrl_executive *exec, int nth)
{
  return jcntrl_output_get_data_object_at(jcntrl_executive_get_output(exec),
                                          nth);
}

void *
jcntrl_executive_get_output_data_object_as(jcntrl_executive *exec, int nth,
                                           const jcntrl_shared_object_data *cls)
{
  return jcntrl_output_get_data_object_at_as(jcntrl_executive_get_output(exec),
                                             nth, cls);
}

/* input port functions */

static void jcntrl_connection_init(jcntrl_connection *conn,
                                   jcntrl_output *upstream);

static int jcntrl_connection_connect(jcntrl_connection *conn,
                                     jcntrl_output *upstream);

static int jcntrl_connection_disconnect(jcntrl_connection *conn);

static int jcntrl_connection_is_connected(jcntrl_connection *conn);

static void jcntrl_input_init(jcntrl_input *input, jcntrl_executive *exe)
{
  JCNTRL_ASSERT(input);
  JCNTRL_ASSERT(exe);
  geom_list_init(&input->list);
  jcntrl_connection_init(&input->port, NULL);
  input->information = NULL;
  input->owner = exe;
}

jcntrl_input *jcntrl_input_add(jcntrl_input *next)
{
  jcntrl_input *new_item;
  JCNTRL_ASSERT(next);
  JCNTRL_ASSERT(next->owner);
  new_item = (jcntrl_input *)malloc(sizeof(jcntrl_input));
  if (!new_item) {
    jcntrl_raise_allocation_failed(__FILE__, __LINE__);
    return NULL;
  }
  jcntrl_input_init(new_item, next->owner);
  new_item->information = jcntrl_information_new();
  if (!new_item->information) {
    jcntrl_input_delete(new_item);
    return NULL;
  }
  geom_list_insert_prev(&next->list, &new_item->list);
  return new_item;
}

jcntrl_input *jcntrl_input_set_number_of_ports(jcntrl_input *item, int num)
{
  struct geom_list *lp, *lh;
  jcntrl_input *head;
  JCNTRL_ASSERT(item);
  JCNTRL_ASSERT(num >= 0);

  head = jcntrl_input_rewind(item);
  lh = &head->list;
  if (num > 0) {
    geom_list_foreach (lp, lh) {
      if (num == 0)
        break;
      num--;
    }
  } else {
    lp = geom_list_next(lh);
  }
  if (num > 0) {
    jcntrl_input new_head;
    jcntrl_input_init(&new_head, head->owner);
    while (num > 0) {
      jcntrl_input *new_item;
      new_item = jcntrl_input_add(&new_head);
      if (!new_item) {
        jcntrl_raise_allocation_failed(__FILE__, __LINE__);
        head = NULL;
        break;
      }
      --num;
    }
    if (head) {
      geom_list_insert_list_prev(&head->list, &new_head.list);
      geom_list_delete(&new_head.list);
    } else {
      struct geom_list *lp, *ln;
      geom_list_foreach_safe (lp, ln, &new_head.list) {
        jcntrl_input *added_item;
        added_item = jcntrl_input_entry(lp);
        jcntrl_input_delete(added_item);
      }
      return NULL;
    }
  } else {
    struct geom_list *ln;
    struct geom_list *ls = lp;
    geom_list_foreach_range_safe (lp, ln, ls, lh) {
      item = jcntrl_input_entry(lp);
      jcntrl_input_delete(item);
    }
  }
  return head;
}

int jcntrl_input_connect(jcntrl_input *input, jcntrl_output *upstream)
{
  JCNTRL_ASSERT(input);
  if (!jcntrl_input_disconnect(input))
    return 0;
  return jcntrl_connection_connect(&input->port, upstream);
}

int jcntrl_input_disconnect(jcntrl_input *input)
{
  JCNTRL_ASSERT(input);
  return jcntrl_connection_disconnect(&input->port);
}

int jcntrl_input_is_connected(jcntrl_input *input)
{
  JCNTRL_ASSERT(input);
  return jcntrl_connection_is_connected(&input->port);
}

void jcntrl_input_delete(jcntrl_input *item)
{
  JCNTRL_ASSERT(item);

  jcntrl_input_disconnect(item);
  geom_list_delete(&item->list);
  if (item->information) {
    jcntrl_information_unlink(item->information);
  }
  free(item);
}

jcntrl_output *jcntrl_output_add(jcntrl_output *next)
{
  jcntrl_output *new_item;
  JCNTRL_ASSERT(next);
  JCNTRL_ASSERT(next->owner);
  new_item = (jcntrl_output *)malloc(sizeof(jcntrl_input));
  if (!new_item) {
    jcntrl_raise_allocation_failed(__FILE__, __LINE__);
    return NULL;
  }
  jcntrl_output_init(new_item, next->owner);
  new_item->information = jcntrl_information_new();
  if (!new_item->information) {
    jcntrl_output_delete(new_item);
    return NULL;
  }
  geom_list_insert_prev(&next->list, &new_item->list);
  return new_item;
}

jcntrl_output *jcntrl_output_set_number_of_ports(jcntrl_output *item, int num)
{
  struct geom_list *lp, *lh;
  jcntrl_output *head;

  JCNTRL_ASSERT(item);
  JCNTRL_ASSERT(num >= 0);

  head = jcntrl_output_rewind(item);
  lh = &head->list;
  if (num > 0) {
    geom_list_foreach (lp, lh) {
      if (num == 0)
        break;
      num--;
    }
  } else {
    lp = geom_list_next(lh);
  }
  if (num > 0) {
    jcntrl_output new_head;
    jcntrl_output_init(&new_head, head->owner);
    while (num > 0) {
      jcntrl_output *new_item;
      new_item = jcntrl_output_add(&new_head);
      if (!new_item) {
        jcntrl_raise_allocation_failed(__FILE__, __LINE__);
        head = NULL;
        break;
      }
      --num;
    }
    if (head) {
      geom_list_insert_list_prev(&head->list, &new_head.list);
      geom_list_delete(&new_head.list);
    } else {
      struct geom_list *lp, *ln;
      geom_list_foreach_safe (lp, ln, &new_head.list) {
        jcntrl_output *added_item;
        added_item = jcntrl_output_entry(lp);
        jcntrl_output_delete(added_item);
      }
      return NULL;
    }
  } else {
    struct geom_list *ln;
    struct geom_list *ls = lp;
    geom_list_foreach_range_safe (lp, ln, ls, lh) {
      item = jcntrl_output_entry(lp);
      jcntrl_output_delete(item);
    }
  }
  return head;
}

void jcntrl_output_delete(jcntrl_output *item)
{
  JCNTRL_ASSERT(item);

  jcntrl_output_disconnect_all(item);
  geom_list_delete(&item->list);
  if (item->information) {
    jcntrl_information_unlink(item->information);
  }
  free(item);
}

/* output port management functions */

static void jcntrl_output_init(jcntrl_output *output, jcntrl_executive *exe)
{
  JCNTRL_ASSERT(output);
  JCNTRL_ASSERT(exe);
  geom_list_init(&output->list);
  jcntrl_connection_init(&output->port, output);
  output->owner = exe;
  output->information = NULL;
}

/* connection management functions */

/**
 * @brief Initialize connection port
 * @param conn Connection struct to init
 * @param input upstream output port
 *
 * @p input is allowed to be NULL, to create a dangling input
 * connection, which does not have an upstream port.
 */
static void jcntrl_connection_init(jcntrl_connection *conn,
                                   jcntrl_output *input)
{
  JCNTRL_ASSERT(conn);
  geom_list_init(&conn->list);
  conn->upstream = input;
}

static int jcntrl_connection_connect(jcntrl_connection *conn,
                                     jcntrl_output *upstream)
{
  JCNTRL_ASSERT(conn);
  JCNTRL_ASSERT(upstream);
  JCNTRL_ASSERT(conn != &upstream->port);

  if (conn->upstream) {
    if (conn->upstream == upstream) {
      return 1;
    }
    if (!jcntrl_connection_disconnect(conn)) {
      return 0;
    }
  }
  conn->upstream = upstream;
  geom_list_insert_prev(&upstream->port.list, &conn->list);
  return 1;
}

static int jcntrl_connection_disconnect(jcntrl_connection *conn)
{
  JCNTRL_ASSERT(conn);
  if (!conn->upstream)
    return 1;

  JCNTRL_ASSERT(&conn->upstream->port != conn);
  geom_list_delete(&conn->list);
  conn->upstream = NULL;
  return 1;
}

/**
 * This function is only for implementing jcntrl_input_is_connected()
 */
static int jcntrl_connection_is_connected(jcntrl_connection *conn)
{
  JCNTRL_ASSERT(conn);
  return !!conn->upstream;
}
