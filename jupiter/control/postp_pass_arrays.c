#include "postp_pass_arrays.h"
#include "cell_data.h"
#include "data_array.h"
#include "error.h"
#include "executive.h"
#include "executive_data.h"
#include "defs.h"
#include "grid_data.h"
#include "information.h"
#include "input.h"
#include "output.h"
#include "shared_object.h"
#include "shared_object_priv.h"
#include "string_array.h"

struct jcntrl_postp_pass_arrays
{
  jcntrl_executive exec;
  jcntrl_postp_pass_array_mode mode;
  jcntrl_string_array *vars;
};
#define jcntrl_postp_pass_arrays__ancestor jcntrl_executive
#define jcntrl_postp_pass_arrays__dnmem exec.jcntrl_executive__dnmem
JCNTRL_VTABLE_NONE(jcntrl_postp_pass_arrays);

static jcntrl_postp_pass_arrays *
jcntrl_postp_pass_arrays_downcast_impl(jcntrl_shared_object *obj)
{
  return JCNTRL_DOWNCAST_IMPL(jcntrl_postp_pass_arrays, obj);
}

static void *jcntrl_postp_pass_arrays_downcast_v(jcntrl_shared_object *obj)
{
  return jcntrl_postp_pass_arrays_downcast_impl(obj);
}

static int jcntrl_postp_pass_arrays_initializer(jcntrl_shared_object *obj)
{
  jcntrl_postp_pass_arrays *p;
  jcntrl_executive *exec;
  jcntrl_input *input;
  jcntrl_output *output;

  p = jcntrl_postp_pass_arrays_downcast_impl(obj);
  p->vars = NULL;
  p->mode = JCNTRL_POSTP_PASS_DELETE;

  exec = jcntrl_postp_pass_arrays_executive(p);
  input = jcntrl_executive_get_input(exec);
  if (!jcntrl_input_set_number_of_ports(input, 1))
    return 0;

  output = jcntrl_executive_get_output(exec);
  if (!jcntrl_output_set_number_of_ports(output, 1))
    return 0;

  p->vars = jcntrl_string_array_new();
  if (!p->vars)
    return 0;
  return 1;
}

static void jcntrl_postp_pass_arrays_destructor(jcntrl_shared_object *obj)
{
  jcntrl_postp_pass_arrays *p;

  p = jcntrl_postp_pass_arrays_downcast_impl(obj);
  if (p->vars)
    jcntrl_string_array_delete(p->vars);
}

static jcntrl_shared_object *jcntrl_postp_pass_arrays_allocator(void)
{
  jcntrl_postp_pass_arrays *p;
  p = jcntrl_shared_object_default_allocator(jcntrl_postp_pass_arrays);
  return p ? jcntrl_postp_pass_arrays_object(p) : NULL;
}

static void jcntrl_postp_pass_arrays_deleter(jcntrl_shared_object *obj)
{
  jcntrl_shared_object_default_deleter(obj);
}

static int jcntrl_postp_pass_arrays_fill_input_port_information_impl(
  jcntrl_shared_object *obj, int index, jcntrl_input *input)
{
  jcntrl_information *info;
  info = jcntrl_input_information(input);
  if (!jcntrl_information_set_datatype(info, JCNTRL_INFO_REQUIRED_DATATYPE,
                                       JCNTRL_DATATYPE_GRID))
    return 0;
  return 1;
}

JCNTRL_VIRTUAL_WRAP(jcntrl_postp_pass_arrays, jcntrl_executive,
                    fill_input_port_information)

static int jcntrl_postp_pass_arrays_fill_output_port_information_impl(
  jcntrl_shared_object *obj, int index, jcntrl_output *output)
{
  jcntrl_information *info;
  info = jcntrl_output_information(output);
  if (!jcntrl_information_set_objecttype(info, JCNTRL_INFO_DATATYPE,
                                         jcntrl_grid_data))
    return 0;
  return 1;
}

JCNTRL_VIRTUAL_WRAP(jcntrl_postp_pass_arrays, jcntrl_executive,
                    fill_output_port_information)

static int jcntrl_postp_pass_arrays_process_update_extent_impl(
  jcntrl_information *request, jcntrl_input *input_head,
  jcntrl_output *output_head, jcntrl_shared_object *obj)
{
  const int *ext;
  jcntrl_input *input;
  jcntrl_output *output;
  jcntrl_information *iinfo;
  jcntrl_information *oinfo;

  input = jcntrl_input_next_port(input_head);
  output = jcntrl_output_next_port(output_head);
  JCNTRL_ASSERT(input);
  JCNTRL_ASSERT(output);

  iinfo = jcntrl_input_information(input);
  oinfo = jcntrl_output_information(output);
  JCNTRL_ASSERT(iinfo);
  JCNTRL_ASSERT(oinfo);

  ext = jcntrl_information_get_extent(iinfo, JCNTRL_INFO_WHOLE_EXTENT);
  if (ext) {
    if (!jcntrl_information_set_extent(oinfo, JCNTRL_INFO_WHOLE_EXTENT, ext))
      return 0;
  } else {
    jcntrl_information_remove(oinfo, JCNTRL_INFO_WHOLE_EXTENT);
  }

  ext = jcntrl_information_get_extent(iinfo, JCNTRL_INFO_DATA_EXTENT);
  if (ext) {
    if (!jcntrl_information_set_extent(oinfo, JCNTRL_INFO_DATA_EXTENT, ext))
      return 0;
  } else {
    jcntrl_information_remove(oinfo, JCNTRL_INFO_DATA_EXTENT);
  }

  ext = jcntrl_information_get_extent(iinfo, JCNTRL_INFO_PIECE_EXTENT);
  if (ext) {
    if (!jcntrl_information_set_extent(oinfo, JCNTRL_INFO_PIECE_EXTENT, ext))
      return 0;
  } else {
    jcntrl_information_remove(oinfo, JCNTRL_INFO_PIECE_EXTENT);
  }
  return 1;
}

JCNTRL_VIRTUAL_WRAP(jcntrl_postp_pass_arrays, jcntrl_executive,
                    process_update_extent)

static int jcntrl_postp_pass_arrays_process_update_data_impl(
  jcntrl_information *request, jcntrl_input *input_head,
  jcntrl_output *output_head, jcntrl_shared_object *obj)
{
  jcntrl_postp_pass_arrays *p;
  jcntrl_grid_data *igrid, *ogrid;
  jcntrl_cell_data *icdata, *ocdata;
  jcntrl_size_type n;

  p = jcntrl_postp_pass_arrays_downcast_impl(obj);

  igrid = jcntrl_input_get_data_object_at_as(input_head, 0, jcntrl_grid_data);
  ogrid = jcntrl_output_get_data_object_at_as(output_head, 0, jcntrl_grid_data);
  JCNTRL_ASSERT(igrid);
  JCNTRL_ASSERT(ogrid);

  if (!jcntrl_grid_data_shallow_copy(ogrid, igrid))
    return 0;

  n = jcntrl_postp_pass_arrays_get_number_of_variables(p);
  if (n <= 0)
    return 1;

  icdata = jcntrl_grid_data_cell_data(igrid);
  ocdata = jcntrl_grid_data_cell_data(ogrid);
  if (p->mode == JCNTRL_POSTP_PASS_KEEP) {
    jcntrl_cell_data_remove_all_arrays(ocdata);
  }

  for (jcntrl_size_type i = 0; i < n; ++i) {
    jcntrl_char_array *name;
    const char *cname;
    jcntrl_size_type clen;
    jcntrl_data_array *ary;

    name = jcntrl_string_array_get(p->vars, i);
    if (!name)
      continue;

    cname = jcntrl_get_string_c(name, &clen);
    if (!cname)
      continue;

    if (p->mode == JCNTRL_POSTP_PASS_KEEP) {
      ary = jcntrl_cell_data_get_array_by_name(icdata, cname, clen);
      if (!ary) {
        jcntrl_raise_argument_error(
          __FILE__, __LINE__,
          "One of arrays specified not found in source (ignored)");
        continue;
      }

      if (!jcntrl_cell_data_add_array(ocdata, ary))
        return 0;

    } else if (p->mode == JCNTRL_POSTP_PASS_DELETE) {
      if (!jcntrl_cell_data_remove_array_by_name(ocdata, cname, clen)) {
        jcntrl_raise_argument_error(
          __FILE__, __LINE__,
          "One of arrays specified not found in source (ignored)");
        continue;
      }
    }
  }
  return 1;
}

JCNTRL_VIRTUAL_WRAP(jcntrl_postp_pass_arrays, jcntrl_executive,
                    process_update_data)

static void jcntrl_postp_pass_arrays_init_func(jcntrl_shared_object_funcs *p)
{
  p->downcast = jcntrl_postp_pass_arrays_downcast_v;
  p->initializer = jcntrl_postp_pass_arrays_initializer;
  p->destructor = jcntrl_postp_pass_arrays_destructor;
  p->allocator = jcntrl_postp_pass_arrays_allocator;
  p->deleter = jcntrl_postp_pass_arrays_deleter;
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_postp_pass_arrays, jcntrl_executive,
                          fill_input_port_information);
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_postp_pass_arrays, jcntrl_executive,
                          fill_output_port_information);
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_postp_pass_arrays, jcntrl_executive,
                          process_update_extent);
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_postp_pass_arrays, jcntrl_executive,
                          process_update_data);
}

JCNTRL_SHARED_METADATA_INIT_DEFINE(jcntrl_postp_pass_arrays,
                                   jcntrl_postp_pass_arrays_init_func)

struct jcntrl_postp_del_variable
{
  jcntrl_postp_pass_arrays arys;
};
#define jcntrl_postp_del_variable__ancestor jcntrl_postp_pass_arrays
#define jcntrl_postp_del_variable__dnmem arys.jcntrl_postp_pass_arrays__dnmem
JCNTRL_VTABLE_NONE(jcntrl_postp_del_variable);

static jcntrl_postp_del_variable *
jcntrl_postp_del_variable_downcast_impl(jcntrl_shared_object *obj)
{
  return JCNTRL_DOWNCAST_IMPL(jcntrl_postp_del_variable, obj);
}

static void *jcntrl_postp_del_variable_downcast_v(jcntrl_shared_object *obj)
{
  return jcntrl_postp_del_variable_downcast_impl(obj);
}

static int jcntrl_postp_del_variable_initializer(jcntrl_shared_object *obj)
{
  jcntrl_postp_del_variable *d;
  d = jcntrl_postp_del_variable_downcast_impl(obj);
  d->arys.mode = JCNTRL_POSTP_PASS_DELETE;
  return 1;
}

static jcntrl_shared_object *jcntrl_postp_del_variable_allocator(void)
{
  jcntrl_postp_del_variable *d;
  d = jcntrl_shared_object_default_allocator(jcntrl_postp_del_variable);
  return d ? jcntrl_postp_del_variable_object(d) : NULL;
}

static void jcntrl_postp_del_variable_deleter(jcntrl_shared_object *obj)
{
  jcntrl_shared_object_default_deleter(obj);
}

static void jcntrl_postp_del_variable_init_func(jcntrl_shared_object_funcs *p)
{
  p->downcast = jcntrl_postp_del_variable_downcast_v;
  p->initializer = jcntrl_postp_del_variable_initializer;
  p->allocator = jcntrl_postp_del_variable_allocator;
  p->deleter = jcntrl_postp_del_variable_deleter;
}

JCNTRL_SHARED_METADATA_INIT_DEFINE(jcntrl_postp_del_variable,
                                   jcntrl_postp_del_variable_init_func)

struct jcntrl_postp_del_variable_except
{
  jcntrl_postp_pass_arrays arys;
};
#define jcntrl_postp_del_variable_except__ancestor jcntrl_postp_pass_arrays
#define jcntrl_postp_del_variable_except__dnmem \
  arys.jcntrl_postp_pass_arrays__dnmem
JCNTRL_VTABLE_NONE(jcntrl_postp_del_variable_except);

static jcntrl_postp_del_variable_except *
jcntrl_postp_del_variable_except_downcast_impl(jcntrl_shared_object *obj)
{
  return JCNTRL_DOWNCAST_IMPL(jcntrl_postp_del_variable_except, obj);
}

static void *
jcntrl_postp_del_variable_except_downcast_v(jcntrl_shared_object *obj)
{
  return jcntrl_postp_del_variable_except_downcast_impl(obj);
}

static int
jcntrl_postp_del_variable_except_initializer(jcntrl_shared_object *obj)
{
  jcntrl_postp_del_variable_except *d;
  d = jcntrl_postp_del_variable_except_downcast_impl(obj);
  d->arys.mode = JCNTRL_POSTP_PASS_KEEP;
  return 1;
}

static jcntrl_shared_object *jcntrl_postp_del_variable_except_allocator(void)
{
  jcntrl_postp_del_variable_except *d;
  d = jcntrl_shared_object_default_allocator(jcntrl_postp_del_variable_except);
  return d ? jcntrl_postp_del_variable_except_object(d) : NULL;
}

static void jcntrl_postp_del_variable_except_deleter(jcntrl_shared_object *obj)
{
  jcntrl_shared_object_default_deleter(obj);
}

static void
jcntrl_postp_del_variable_except_init_func(jcntrl_shared_object_funcs *p)
{
  p->downcast = jcntrl_postp_del_variable_except_downcast_v;
  p->initializer = jcntrl_postp_del_variable_except_initializer;
  p->allocator = jcntrl_postp_del_variable_except_allocator;
  p->deleter = jcntrl_postp_del_variable_except_deleter;
}

JCNTRL_SHARED_METADATA_INIT_DEFINE(jcntrl_postp_del_variable_except,
                                   jcntrl_postp_del_variable_except_init_func)

//----

jcntrl_postp_pass_arrays *jcntrl_postp_pass_arrays_new(void)
{
  return jcntrl_shared_object_new(jcntrl_postp_pass_arrays);
}

void jcntrl_postp_pass_arrays_delete(jcntrl_postp_pass_arrays *p)
{
  jcntrl_shared_object_delete(jcntrl_postp_pass_arrays_object(p));
}

jcntrl_shared_object *
jcntrl_postp_pass_arrays_object(jcntrl_postp_pass_arrays *p)
{
  return jcntrl_executive_object(jcntrl_postp_pass_arrays_executive(p));
}

jcntrl_executive *
jcntrl_postp_pass_arrays_executive(jcntrl_postp_pass_arrays *p)
{
  return &p->exec;
}

int jcntrl_postp_pass_arrays_set_number_of_variables(
  jcntrl_postp_pass_arrays *p, jcntrl_size_type nvars)
{
  return jcntrl_string_array_resize(p->vars, nvars);
}

jcntrl_size_type
jcntrl_postp_pass_arrays_get_number_of_variables(jcntrl_postp_pass_arrays *p)
{
  return jcntrl_string_array_get_ntuple(p->vars);
}

int jcntrl_postp_pass_arrays_set_variable(jcntrl_postp_pass_arrays *p,
                                          jcntrl_size_type index,
                                          jcntrl_data_array *name)
{
  return jcntrl_string_array_set_copy(p->vars, index, name);
}

int jcntrl_postp_pass_arrays_set_variable_c(jcntrl_postp_pass_arrays *p,
                                            jcntrl_size_type index,
                                            const char *name,
                                            jcntrl_size_type len)
{
  return jcntrl_string_array_set_cstr(p->vars, index, name, len);
}

jcntrl_char_array *
jcntrl_postp_pass_arrays_get_variable(jcntrl_postp_pass_arrays *p,
                                      jcntrl_size_type index)
{
  return jcntrl_string_array_get(p->vars, index);
}

jcntrl_postp_pass_array_mode
jcntrl_postp_pass_arrays_get_mode(jcntrl_postp_pass_arrays *p)
{
  return p->mode;
}

int jcntrl_postp_pass_arrays_set_mode(jcntrl_postp_pass_arrays *p,
                                      jcntrl_postp_pass_array_mode mode)
{
  jcntrl_shared_object *obj;

  obj = jcntrl_postp_pass_arrays_object(p);
  if (jcntrl_shared_object_is_a(jcntrl_postp_del_variable, obj) ||
      jcntrl_shared_object_is_a(jcntrl_postp_del_variable_except, obj)) {
    jcntrl_raise_argument_error(
      __FILE__, __LINE__,
      "Cannot set mode of this jcntrl_postp_pass_arrays object");
    return 0;
  }

  p->mode = mode;
  return 1;
}

//---

jcntrl_postp_del_variable *jcntrl_postp_del_variable_new(void)
{
  return jcntrl_shared_object_new(jcntrl_postp_del_variable);
}

void jcntrl_postp_del_variable_delete(jcntrl_postp_del_variable *p)
{
  jcntrl_shared_object_delete(jcntrl_postp_del_variable_object(p));
}

jcntrl_shared_object *
jcntrl_postp_del_variable_object(jcntrl_postp_del_variable *p)
{
  return jcntrl_executive_object(jcntrl_postp_del_variable_executive(p));
}

jcntrl_executive *
jcntrl_postp_del_variable_executive(jcntrl_postp_del_variable *p)
{
  return jcntrl_postp_pass_arrays_executive(
    jcntrl_postp_del_variable_pass_arrays(p));
}

jcntrl_postp_pass_arrays *
jcntrl_postp_del_variable_pass_arrays(jcntrl_postp_del_variable *p)
{
  return &p->arys;
}

jcntrl_postp_del_variable *
jcntrl_postp_del_variable_downcast(jcntrl_shared_object *obj)
{
  return jcntrl_shared_object_downcast(jcntrl_postp_del_variable, obj);
}

//---

jcntrl_postp_del_variable_except *jcntrl_postp_del_variable_except_new(void)
{
  return jcntrl_shared_object_new(jcntrl_postp_del_variable_except);
}

void jcntrl_postp_del_variable_except_delete(
  jcntrl_postp_del_variable_except *p)
{
  jcntrl_shared_object_delete(jcntrl_postp_del_variable_except_object(p));
}

jcntrl_shared_object *
jcntrl_postp_del_variable_except_object(jcntrl_postp_del_variable_except *p)
{
  return jcntrl_executive_object(jcntrl_postp_del_variable_except_executive(p));
}

jcntrl_executive *
jcntrl_postp_del_variable_except_executive(jcntrl_postp_del_variable_except *p)
{
  return jcntrl_postp_pass_arrays_executive(
    jcntrl_postp_del_variable_except_pass_arrays(p));
}

jcntrl_postp_pass_arrays *jcntrl_postp_del_variable_except_pass_arrays(
  jcntrl_postp_del_variable_except *p)
{
  return &p->arys;
}

jcntrl_postp_del_variable_except *
jcntrl_postp_del_variable_except_downcast(jcntrl_shared_object *obj)
{
  return jcntrl_shared_object_downcast(jcntrl_postp_del_variable_except, obj);
}
