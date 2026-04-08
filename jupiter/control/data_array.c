
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "data_array.h"
#include "data_array_data.h"
#include "overflow.h"
#include "shared_object.h"
#include "static_array.h"
#include "struct_data.h"
#include "defs.h"
#include "shared_object_priv.h"
#include "error.h"
#include "abstract_array.h"

static jcntrl_data_array *
jcntrl_data_array_downcast_impl(jcntrl_shared_object *obj)
{
  return JCNTRL_DOWNCAST_IMPL(jcntrl_data_array, obj);
}

static void
jcntrl_generic_data_array_discard_data(jcntrl_generic_data_array *ary)
{
  JCNTRL_ASSERT(ary);

  free(ary->allocated_data);
  ary->allocated_data = NULL;
  ary->readonly_data = NULL;
  ary->number_of_tuples = 0;
}

static int jcntrl_data_array_initializer(jcntrl_shared_object *obj)
{
  return 1;
}

static void jcntrl_data_array_destructor(jcntrl_shared_object *obj)
{
  /* nop */
}

static void *jcntrl_data_array_downcast_v(jcntrl_shared_object *obj)
{
  return jcntrl_data_array_downcast_impl(obj);
}

static jcntrl_size_type
jcntrl_data_array_element_size_impl(jcntrl_shared_object *p)
{
  return 0;
}

JCNTRL_VIRTUAL_WRAP(jcntrl_data_array, jcntrl_data_array, element_size)

jcntrl_size_type jcntrl_data_array_element_size(jcntrl_data_array *ary)
{
  struct jcntrl_data_array_element_size_args args = {0};
  jcntrl_shared_object_call_virtual(jcntrl_data_array_object(ary),
                                    jcntrl_data_array, element_size, &args);
  return args.sz;
}

jcntrl_size_type
jcntrl_data_array_element_size__super(const jcntrl_shared_object_data *ancestor)
{
  struct jcntrl_data_array_element_size_args args = {0};
  jcntrl_shared_object_call_super(ancestor, jcntrl_data_array, element_size,
                                  &args);
  return args.sz;
}

static const jcntrl_shared_object_data *
jcntrl_data_array_element_type_impl(jcntrl_shared_object *obj)
{
  return jcntrl_shared_object_class(obj);
}

JCNTRL_VIRTUAL_WRAP(jcntrl_data_array, jcntrl_data_array, element_type)

const jcntrl_shared_object_data *
jcntrl_data_array_element_type(jcntrl_data_array *ary)
{
  struct jcntrl_data_array_element_type_args args = {NULL};
  jcntrl_shared_object_call_virtual(jcntrl_data_array_object(ary),
                                    jcntrl_data_array, element_type, &args);
  return args.ret;
}

const jcntrl_shared_object_data *
jcntrl_data_array_element_type__super(const jcntrl_shared_object_data *ancestor)
{
  struct jcntrl_data_array_element_type_args args = {NULL};
  jcntrl_shared_object_call_super(ancestor, jcntrl_data_array, element_type,
                                  &args);
  return args.ret;
}

static const void *jcntrl_data_array_get_impl(jcntrl_shared_object *obj)
{
  return NULL;
}

JCNTRL_VIRTUAL_WRAP(jcntrl_data_array, jcntrl_data_array, get)

const void *jcntrl_data_array_get(jcntrl_data_array *ary)
{
  struct jcntrl_data_array_get_args args = {NULL};
  jcntrl_shared_object_call_virtual(jcntrl_data_array_object(ary),
                                    jcntrl_data_array, get, &args);
  return args.p;
}

const void *
jcntrl_data_array_get__super(const jcntrl_shared_object_data *ancestor)
{
  struct jcntrl_data_array_get_args args = {NULL};
  jcntrl_shared_object_call_super(ancestor, jcntrl_data_array, get, &args);
  return args.p;
}

static void *jcntrl_data_array_get_writable_impl(jcntrl_shared_object *obj)
{
  return NULL;
}

JCNTRL_VIRTUAL_WRAP(jcntrl_data_array, jcntrl_data_array, get_writable)

void *jcntrl_data_array_get_writable(jcntrl_data_array *ary)
{
  struct jcntrl_data_array_get_writable_args args = {NULL};
  jcntrl_shared_object_call_virtual(jcntrl_data_array_object(ary),
                                    jcntrl_data_array, get_writable, &args);
  return args.p;
}

void *
jcntrl_data_array_get_writable__super(const jcntrl_shared_object_data *ancestor)
{
  struct jcntrl_data_array_get_writable_args args = {NULL};
  jcntrl_shared_object_call_super(ancestor, jcntrl_data_array, get_writable,
                                  &args);
  return args.p;
}

static double jcntrl_data_array_get_value_impl(jcntrl_shared_object *obj,
                                               jcntrl_size_type index)
{
  return 0.0;
}

JCNTRL_VIRTUAL_WRAP(jcntrl_data_array, jcntrl_data_array, get_value)

double jcntrl_data_array_get_value(jcntrl_data_array *ary,
                                   jcntrl_size_type index)
{
  struct jcntrl_data_array_get_value_args args = {
    .index = index,
    .ret = 0.0,
  };
  jcntrl_shared_object_call_virtual(jcntrl_data_array_object(ary),
                                    jcntrl_data_array, get_value, &args);
  return args.ret;
}

double
jcntrl_data_array_get_value__super(const jcntrl_shared_object_data *ancestor,
                                   jcntrl_size_type index)
{
  struct jcntrl_data_array_get_value_args args = {
    .index = index,
    .ret = 0.0,
  };
  jcntrl_shared_object_call_super(ancestor, jcntrl_data_array, get_value,
                                  &args);
  return args.ret;
}

static int jcntrl_data_array_set_value_impl(jcntrl_shared_object *obj,
                                            jcntrl_size_type index,
                                            double value)
{
  jcntrl_raise_argument_error(__FILE__, __LINE__, "The array is not writable");
  return 0;
}

JCNTRL_VIRTUAL_WRAP(jcntrl_data_array, jcntrl_data_array, set_value)

int jcntrl_data_array_set_value(jcntrl_data_array *ary, jcntrl_size_type index,
                                double value)
{
  struct jcntrl_data_array_set_value_args args = {
    .index = index,
    .value = value,
    .ret = 0,
  };
  jcntrl_shared_object_call_virtual(jcntrl_data_array_object(ary),
                                    jcntrl_data_array, set_value, &args);
  return args.ret;
}

int jcntrl_data_array_set_value__super(
  const jcntrl_shared_object_data *ancestor, jcntrl_size_type index,
  double value)
{
  struct jcntrl_data_array_set_value_args args = {
    .index = index,
    .value = value,
    .ret = 0,
  };
  jcntrl_shared_object_call_super(ancestor, jcntrl_data_array, set_value,
                                  &args);
  return args.ret;
}

static intmax_t jcntrl_data_array_get_ivalue_impl(jcntrl_shared_object *obj,
                                                  jcntrl_size_type index,
                                                  int *err)
{
  jcntrl_raise_argument_error(__FILE__, __LINE__,
                              "Data cannot be representable as integral value");
  if (err)
    *err = 1;
  return 0;
}

JCNTRL_VIRTUAL_WRAP(jcntrl_data_array, jcntrl_data_array, get_ivalue)

intmax_t jcntrl_data_array_get_ivalue(jcntrl_data_array *ary,
                                      jcntrl_size_type index, int *err)
{
  struct jcntrl_data_array_get_ivalue_args args = {
    .index = index,
    .err = err,
    .ret = 0,
  };
  jcntrl_shared_object_call_virtual(jcntrl_data_array_object(ary),
                                    jcntrl_data_array, get_ivalue, &args);
  return args.ret;
}

intmax_t
jcntrl_data_array_get_ivalue__super(const jcntrl_shared_object_data *ancestor,
                                    jcntrl_size_type index, int *err)
{
  struct jcntrl_data_array_get_ivalue_args args = {
    .index = index,
    .err = err,
    .ret = 0,
  };
  jcntrl_shared_object_call_super(ancestor, jcntrl_data_array, get_ivalue,
                                  &args);
  return args.ret;
}

static int jcntrl_data_array_set_ivalue_impl(jcntrl_shared_object *obj,
                                             jcntrl_size_type index,
                                             intmax_t value)
{
  jcntrl_raise_argument_error(__FILE__, __LINE__, "The array is not writable");
  return 0;
}

JCNTRL_VIRTUAL_WRAP(jcntrl_data_array, jcntrl_data_array, set_ivalue)

int jcntrl_data_array_set_ivalue(jcntrl_data_array *ary, jcntrl_size_type index,
                                 intmax_t value)
{
  struct jcntrl_data_array_set_ivalue_args args = {
    .index = index,
    .value = value,
    .ret = 0,
  };
  jcntrl_shared_object_call_virtual(jcntrl_data_array_object(ary),
                                    jcntrl_data_array, set_ivalue, &args);
  return args.ret;
}

int jcntrl_data_array_set_ivalue__super(
  const jcntrl_shared_object_data *ancestor, jcntrl_size_type index,
  intmax_t value)
{
  struct jcntrl_data_array_set_ivalue_args args = {
    .index = index,
    .value = value,
    .ret = 0,
  };
  jcntrl_shared_object_call_super(ancestor, jcntrl_data_array, set_ivalue,
                                  &args);
  return args.ret;
}

static void jcntrl_data_array_func_init(jcntrl_shared_object_funcs *p)
{
  p->allocator = NULL;
  p->deleter = NULL;
  p->destructor = jcntrl_data_array_destructor;
  p->initializer = jcntrl_data_array_initializer;
  p->downcast = jcntrl_data_array_downcast_v;
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_data_array, jcntrl_data_array,
                          element_size);
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_data_array, jcntrl_data_array,
                          element_type);
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_data_array, jcntrl_data_array, get);
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_data_array, jcntrl_data_array,
                          get_writable);
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_data_array, jcntrl_data_array, get_value);
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_data_array, jcntrl_data_array, set_value);
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_data_array, jcntrl_data_array, get_ivalue);
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_data_array, jcntrl_data_array, set_ivalue);
}

JCNTRL_SHARED_METADATA_INIT_DEFINE(jcntrl_data_array,
                                   jcntrl_data_array_func_init)

static jcntrl_generic_data_array *
jcntrl_generic_data_array_downcast_impl(jcntrl_shared_object *obj)
{
  return JCNTRL_DOWNCAST_IMPL(jcntrl_generic_data_array, obj);
}

static void *jcntrl_generic_data_array_downcast_v(jcntrl_shared_object *obj)
{
  return jcntrl_generic_data_array_downcast_impl(obj);
}

static int jcntrl_generic_data_array_initializer(jcntrl_shared_object *obj)
{
  jcntrl_generic_data_array *ary;
  ary = jcntrl_generic_data_array_downcast_impl(obj);
  ary->allocated_data = NULL;
  ary->readonly_data = NULL;
  ary->number_of_tuples = 0;
  return 1;
}

static void jcntrl_generic_data_array_destructor(jcntrl_shared_object *obj)
{
  jcntrl_generic_data_array *ary;
  ary = jcntrl_generic_data_array_downcast_impl(obj);
  jcntrl_generic_data_array_discard_data(ary);
}

static jcntrl_size_type
jcntrl_generic_data_array_get_ntuple_impl(jcntrl_shared_object *obj)
{
  jcntrl_generic_data_array *ary;
  ary = jcntrl_generic_data_array_downcast_impl(obj);
  return ary->number_of_tuples;
}

JCNTRL_VIRTUAL_WRAP(jcntrl_generic_data_array, jcntrl_abstract_array,
                    get_ntuple)

static const void *jcntrl_generic_data_array_get_impl(jcntrl_shared_object *obj)
{
  jcntrl_generic_data_array *ary;
  ary = jcntrl_generic_data_array_downcast_impl(obj);
  return ary->readonly_data;
}

JCNTRL_VIRTUAL_WRAP(jcntrl_generic_data_array, jcntrl_data_array, get)

static int jcntrl_generic_data_array_detach(jcntrl_generic_data_array *ary)
{
  void *new_p;
  jcntrl_size_type elsize;
  jcntrl_size_type bytesize;
  JCNTRL_ASSERT(ary);

  /* note: later includes NULL */
  if (ary->allocated_data || ary->readonly_data == ary->allocated_data)
    return 1;

  JCNTRL_ASSERT(ary->number_of_tuples > 0);

  elsize = jcntrl_data_array_element_size(jcntrl_generic_data_array_data(ary));
  bytesize = elsize * ary->number_of_tuples;
  new_p = malloc(bytesize);
  if (!new_p) {
    jcntrl_raise_allocation_failed(__FILE__, __LINE__);
    return 0;
  }

  memcpy(new_p, ary->readonly_data, bytesize);
  ary->allocated_data = new_p;
  ary->readonly_data = new_p;
  return 1;
}

static void *
jcntrl_generic_data_array_get_writable_impl(jcntrl_shared_object *obj)
{
  jcntrl_generic_data_array *ary;
  ary = jcntrl_generic_data_array_downcast_impl(obj);
#ifdef _OPENMP
#pragma omp critical
#endif
  if (!ary->allocated_data)
    jcntrl_generic_data_array_detach(ary);
  return ary->allocated_data;
}

JCNTRL_VIRTUAL_WRAP(jcntrl_generic_data_array, jcntrl_data_array, get_writable)

static int jcntrl_generic_data_array_resize_impl(jcntrl_shared_object *obj,
                                                 jcntrl_size_type ntuple)
{
  jcntrl_generic_data_array *ary;
  void *new_p;
  jcntrl_size_type el_size, bytesize;

  ary = jcntrl_generic_data_array_downcast_impl(obj);

  el_size = jcntrl_data_array_element_size(jcntrl_generic_data_array_data(ary));
  JCNTRL_ASSERT(el_size > 0);
  JCNTRL_ASSERT(ntuple >= 0);

  if (jcntrl_s_mul_overflow(el_size, ntuple, &bytesize))
    return 0;

  if (bytesize == 0) {
    jcntrl_generic_data_array_discard_data(ary);
    ary->number_of_tuples = 0;
    return 1;
  }

  if (ary->allocated_data || ary->readonly_data) {
    jcntrl_size_type cloff, clsize;
    if (ntuple > ary->number_of_tuples) {
      cloff = el_size * ary->number_of_tuples;
      clsize = el_size * (ntuple - ary->number_of_tuples);
    } else {
      cloff = 0;
      clsize = 0;
    }

    if (ary->allocated_data) {
      new_p = realloc(ary->allocated_data, bytesize);
    } else {
      new_p = malloc(bytesize);
    }
    if (!new_p) {
      jcntrl_raise_allocation_failed(__FILE__, __LINE__);
      return 0;
    }
    if (clsize > 0) {
      memset((char *)new_p + cloff, 0, clsize);
    }
    if (!ary->allocated_data) {
      memcpy(new_p, ary->readonly_data, cloff);
    }
  } else {
    new_p = calloc(bytesize, 1);
    if (!new_p) {
      jcntrl_raise_allocation_failed(__FILE__, __LINE__);
      return 0;
    }
  }

  ary->number_of_tuples = ntuple;
  ary->allocated_data = new_p;
  ary->readonly_data = new_p;
  return 1;
}

JCNTRL_VIRTUAL_WRAP(jcntrl_generic_data_array, jcntrl_abstract_array, resize)

static void jcntrl_generic_data_array_init_func(jcntrl_shared_object_funcs *p)
{
  p->downcast = jcntrl_generic_data_array_downcast_v;
  p->allocator = NULL;
  p->deleter = NULL;
  p->initializer = jcntrl_generic_data_array_initializer;
  p->destructor = jcntrl_generic_data_array_destructor;
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_generic_data_array, jcntrl_abstract_array,
                          get_ntuple);
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_generic_data_array, jcntrl_data_array, get);
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_generic_data_array, jcntrl_data_array,
                          get_writable);
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_generic_data_array, jcntrl_abstract_array,
                          resize);
}

JCNTRL_SHARED_METADATA_INIT_DEFINE(jcntrl_generic_data_array,
                                   jcntrl_generic_data_array_init_func)

//---

static const void *jcntrl_typed_array_get_rdaddr(jcntrl_shared_object *obj,
                                                 jcntrl_size_type index,
                                                 jcntrl_size_type elsize)
{
  jcntrl_generic_data_array *ary;
  JCNTRL_ASSERT(jcntrl_shared_object_is_a(jcntrl_generic_data_array, obj));
  ary = jcntrl_generic_data_array_downcast_impl(obj);
  if (index < 0 || index >= ary->number_of_tuples) {
    jcntrl_raise_index_error(__FILE__, __LINE__, index);
    return NULL;
  }

  return (const char *)ary->readonly_data + index * elsize;
}

static void *jcntrl_typed_array_get_wraddr(jcntrl_shared_object *obj,
                                           jcntrl_size_type index,
                                           jcntrl_size_type elsize)
{
  int r;
  jcntrl_generic_data_array *ary;
  JCNTRL_ASSERT(jcntrl_shared_object_is_a(jcntrl_generic_data_array, obj));
  ary = jcntrl_generic_data_array_downcast_impl(obj);
  if (index < 0 || index >= ary->number_of_tuples) {
    jcntrl_raise_index_error(__FILE__, __LINE__, index);
    return NULL;
  }
#ifdef _OPENMP
#pragma omp critical
#endif
  r = jcntrl_generic_data_array_detach(ary);
  if (!r)
    return NULL;

  return (char *)ary->allocated_data + index * elsize;
}

static double jcntrl_typed_array_get_value_impl(
  jcntrl_shared_object *obj, jcntrl_size_type index, jcntrl_size_type elsize,
  double (*conv)(const void *data))
{
  const void *p;
  if (!conv) {
    jcntrl_raise_argument_error(__FILE__, __LINE__,
                                "Cannot represent value as double");
    return 0.0;
  }

  p = jcntrl_typed_array_get_rdaddr(obj, index, elsize);
  if (!p)
    return 0.0;
  return conv(p);
}

#define jcntrl_typed_array_get_value_impl(cls, obj, index)              \
  jcntrl_typed_array_get_value_impl(obj, index, cls##_element_size_c(), \
                                    cls##_get_value_conv)

#define jcntrl_chk_type(cls, type) \
  ((void)sizeof(*(&(const type *){0}) = cls##_get(NULL)))

static double jcntrl_char_array_get_value_conv(const void *p)
{
  jcntrl_chk_type(jcntrl_char_array, char);
  return *(const char *)p;
}

static double jcntrl_bool_array_get_value_conv(const void *p)
{
  jcntrl_chk_type(jcntrl_bool_array, char);
  return *(const char *)p;
}

static double jcntrl_int_array_get_value_conv(const void *p)
{
  jcntrl_chk_type(jcntrl_int_array, int);
  return *(const int *)p;
}

static double jcntrl_aint_array_get_value_conv(const void *p)
{
  jcntrl_chk_type(jcntrl_aint_array, jcntrl_aint_type);
  return *(const jcntrl_aint_type *)p;
}

static double jcntrl_float_array_get_value_conv(const void *p)
{
  jcntrl_chk_type(jcntrl_float_array, float);
  return *(const float *)p;
}

static double jcntrl_double_array_get_value_conv(const void *p)
{
  jcntrl_chk_type(jcntrl_double_array, double);
  return *(const double *)p;
}

static double jcntrl_size_array_get_value_conv(const void *p)
{
  jcntrl_chk_type(jcntrl_size_array, jcntrl_size_type);
  return *(const jcntrl_size_type *)p;
}

//---

static int jcntrl_typed_array_set_value_impl(jcntrl_shared_object *obj,
                                             jcntrl_size_type index,
                                             jcntrl_size_type elsize,
                                             double value,
                                             int (*conv)(void *p, double value))
{
  void *p;
  if (!conv) {
    jcntrl_raise_argument_error(__FILE__, __LINE__,
                                "Cannot write with double value");
    return 0;
  }

  p = jcntrl_typed_array_get_wraddr(obj, index, elsize);
  if (!p)
    return 0;

  return conv(p, value);
}

#define jcntrl_typed_array_set_value_impl(cls, obj, index, value)              \
  jcntrl_typed_array_set_value_impl(obj, index, cls##_element_size_c(), value, \
                                    cls##_set_value_conv)

static int jcntrl_char_array_set_value_conv(void *p, double value)
{
  jcntrl_chk_type(jcntrl_char_array, char);
  *(char *)p = value;
  return 1;
}

static int jcntrl_bool_array_set_value_conv(void *p, double value)
{
  jcntrl_chk_type(jcntrl_bool_array, char);
  *(char *)p = value;
  return 1;
}

static int jcntrl_int_array_set_value_conv(void *p, double value)
{
  jcntrl_chk_type(jcntrl_int_array, int);
  *(int *)p = value;
  return 1;
}

static int jcntrl_aint_array_set_value_conv(void *p, double value)
{
  jcntrl_chk_type(jcntrl_aint_array, jcntrl_aint_type);
  *(jcntrl_aint_type *)p = value;
  return 1;
}

static int jcntrl_float_array_set_value_conv(void *p, double value)
{
  jcntrl_chk_type(jcntrl_float_array, float);
  *(float *)p = value;
  return 1;
}

static int jcntrl_double_array_set_value_conv(void *p, double value)
{
  jcntrl_chk_type(jcntrl_double_array, double);
  *(double *)p = value;
  return 1;
}

static int jcntrl_size_array_set_value_conv(void *p, double value)
{
  jcntrl_chk_type(jcntrl_size_array, jcntrl_size_type);
  *(jcntrl_size_type *)p = value;
  return 1;
}

//---

static intmax_t jcntrl_typed_array_get_ivalue_impl(
  jcntrl_shared_object *obj, jcntrl_size_type index, int *err,
  jcntrl_size_type elsize, intmax_t (*conv)(const void *data, int *err))
{
  const void *p;
  if (!conv) {
    jcntrl_raise_argument_error(__FILE__, __LINE__,
                                "Cannot get value of array as intmax_t");
    if (err)
      *err = 1;
    return 0;
  }

  p = jcntrl_typed_array_get_rdaddr(obj, index, elsize);
  if (!p) {
    if (err)
      *err = 1;
    return 0;
  }

  return conv(p, err);
}

static intmax_t jcntrl_char_array_get_ivalue_conv(const void *p, int *err)
{
  jcntrl_chk_type(jcntrl_char_array, char);
  if (err)
    *err = 0;
  return *(const char *)p;
}

static intmax_t jcntrl_bool_array_get_ivalue_conv(const void *p, int *err)
{
  jcntrl_chk_type(jcntrl_bool_array, char);
  if (err)
    *err = 0;
  return *(const char *)p;
}

static intmax_t jcntrl_int_array_get_ivalue_conv(const void *p, int *err)
{
  jcntrl_chk_type(jcntrl_int_array, int);
  if (err)
    *err = 0;
  return *(const int *)p;
}

static intmax_t jcntrl_aint_array_get_ivalue_conv(const void *p, int *err)
{
  jcntrl_chk_type(jcntrl_aint_array, jcntrl_aint_type);
  /*
   * Although the MPI standard does not state MPI_Aint, which is the definition
   * of jcntrl_aint_type when MPI is enabled, as a scalar type, we can
   * indirectly assume it is scalar assignable. So, the MPI_Aint will always be
   * smaller than intmax_t.
   *
   * Just one concern is that MPI_Aint can be an unsigned type.
   */
  JCNTRL_ASSERT(sizeof(jcntrl_aint_type) <= sizeof(intmax_t));
  if (err)
    *err = 0;
  return *(const jcntrl_aint_type *)p;
}

#define jcntrl_float_array_get_ivalue_conv NULL
#define jcntrl_double_array_get_ivalue_conv NULL

static intmax_t jcntrl_size_array_get_ivalue_conv(const void *p, int *err)
{
  jcntrl_chk_type(jcntrl_size_array, jcntrl_size_type);
  if (err)
    *err = 0;
  return *(const jcntrl_size_type *)p;
}

#define jcntrl_typed_array_get_ivalue_impl(cls, obj, index, err)              \
  jcntrl_typed_array_get_ivalue_impl(obj, index, err, cls##_element_size_c(), \
                                     cls##_get_ivalue_conv)

//---

static int jcntrl_typed_array_set_ivalue_impl(
  jcntrl_shared_object *obj, jcntrl_size_type index, intmax_t value,
  jcntrl_size_type elsize, int (*conv)(void *data, intmax_t value))
{
  void *p;
  if (!conv) {
    jcntrl_raise_argument_error(__FILE__, __LINE__,
                                "Cannot set value of array as intmax_t");
    return 0;
  }

  p = jcntrl_typed_array_get_wraddr(obj, index, elsize);
  if (!p)
    return 0;

  return conv(p, value);
}

static int jcntrl_typed_array_set_ivalue_ovf_chk(intmax_t value, intmax_t min,
                                                 intmax_t max)
{
  if (value < min || value > max) {
    jcntrl_raise_overflow_error(__FILE__, __LINE__);
    return 0;
  }
  return 1;
}

static int jcntrl_char_array_set_ivalue_conv(void *p, intmax_t value)
{
  jcntrl_chk_type(jcntrl_char_array, char);
  if (!jcntrl_typed_array_set_ivalue_ovf_chk(value, CHAR_MIN, CHAR_MAX))
    return 0;
  *(char *)p = value;
  return 1;
}

static int jcntrl_bool_array_set_ivalue_conv(void *p, intmax_t value)
{
  jcntrl_chk_type(jcntrl_bool_array, char);
  if (!jcntrl_typed_array_set_ivalue_ovf_chk(value, CHAR_MIN, CHAR_MAX))
    return 0;
  *(char *)p = value;
  return 1;
}

static int jcntrl_int_array_set_ivalue_conv(void *p, intmax_t value)
{
  jcntrl_chk_type(jcntrl_int_array, int);
  if (!jcntrl_typed_array_set_ivalue_ovf_chk(value, INT_MIN, INT_MAX))
    return 0;
  *(int *)p = value;
  return 1;
}

static int jcntrl_aint_array_set_ivalue_conv(void *p, intmax_t value)
{
  jcntrl_chk_type(jcntrl_aint_array, jcntrl_aint_type);
  /* No information to check overflow */
  *(jcntrl_aint_type *)p = value;
  return 1;
}

#define jcntrl_float_array_set_ivalue_conv NULL
#define jcntrl_double_array_set_ivalue_conv NULL

static int jcntrl_size_array_set_ivalue_conv(void *p, intmax_t value)
{
  jcntrl_chk_type(jcntrl_size_array, jcntrl_size_type);
  if (!jcntrl_typed_array_set_ivalue_ovf_chk(value, JCNTRL_SIZE_MIN,
                                             JCNTRL_SIZE_MAX))
    return 0;
  *(jcntrl_size_type *)p = value;
  return 1;
}

#define jcntrl_typed_array_set_ivalue_impl(cls, obj, index, value) \
  jcntrl_typed_array_set_ivalue_impl(obj, index, value,            \
                                     cls##_element_size_c(),       \
                                     cls##_set_ivalue_conv)

#define JCNTRL_SHARED_METADATA_DATA_ARRAY_INIT_DEFINE(type, element_type)     \
  static jcntrl_##type##_array *jcntrl_##type##_array_downcast_impl(          \
    jcntrl_shared_object *obj)                                                \
  {                                                                           \
    return JCNTRL_DOWNCAST_IMPL(jcntrl_##type##_array, obj);                  \
  }                                                                           \
                                                                              \
  static void *jcntrl_##type##_array_downcast_v(jcntrl_shared_object *obj)    \
  {                                                                           \
    return jcntrl_##type##_array_downcast_impl(obj);                          \
  }                                                                           \
                                                                              \
  static jcntrl_shared_object *jcntrl_##type##_array_allocator(void)          \
  {                                                                           \
    jcntrl_##type##_array *p;                                                 \
    p = jcntrl_shared_object_default_allocator(jcntrl_##type##_array);        \
    return p ? jcntrl_##type##_array_object(p) : NULL;                        \
  }                                                                           \
                                                                              \
  static void jcntrl_##type##_array_deleter(jcntrl_shared_object *obj)        \
  {                                                                           \
    jcntrl_shared_object_default_deleter(obj);                                \
  }                                                                           \
                                                                              \
  static jcntrl_size_type jcntrl_##type##_array_element_size_c(void)          \
  {                                                                           \
    return sizeof(element_type);                                              \
  }                                                                           \
                                                                              \
  static jcntrl_size_type jcntrl_##type##_array_element_size_impl(            \
    jcntrl_shared_object *obj)                                                \
  {                                                                           \
    return jcntrl_##type##_array_element_size_c();                            \
  }                                                                           \
                                                                              \
  JCNTRL_VIRTUAL_WRAP(jcntrl_##type##_array, jcntrl_data_array, element_size) \
                                                                              \
  static double jcntrl_##type##_array_get_value_impl(                         \
    jcntrl_shared_object *obj, jcntrl_size_type index)                        \
  {                                                                           \
    return jcntrl_typed_array_get_value_impl(jcntrl_##type##_array, obj,      \
                                             index);                          \
  }                                                                           \
                                                                              \
  JCNTRL_VIRTUAL_WRAP(jcntrl_##type##_array, jcntrl_data_array, get_value)    \
                                                                              \
  static int jcntrl_##type##_array_set_value_impl(jcntrl_shared_object *obj,  \
                                                  jcntrl_size_type index,     \
                                                  double value)               \
  {                                                                           \
    return jcntrl_typed_array_set_value_impl(jcntrl_##type##_array, obj,      \
                                             index, value);                   \
  }                                                                           \
                                                                              \
  JCNTRL_VIRTUAL_WRAP(jcntrl_##type##_array, jcntrl_data_array, set_value)    \
                                                                              \
  static intmax_t jcntrl_##type##_array_get_ivalue_impl(                      \
    jcntrl_shared_object *obj, jcntrl_size_type index, int *err)              \
  {                                                                           \
    return jcntrl_typed_array_get_ivalue_impl(jcntrl_##type##_array, obj,     \
                                              index, err);                    \
  }                                                                           \
                                                                              \
  JCNTRL_VIRTUAL_WRAP(jcntrl_##type##_array, jcntrl_data_array, get_ivalue)   \
                                                                              \
  static int jcntrl_##type##_array_set_ivalue_impl(jcntrl_shared_object *obj, \
                                                   jcntrl_size_type index,    \
                                                   intmax_t value)            \
  {                                                                           \
    return jcntrl_typed_array_set_ivalue_impl(jcntrl_##type##_array, obj,     \
                                              index, value);                  \
  }                                                                           \
                                                                              \
  JCNTRL_VIRTUAL_WRAP(jcntrl_##type##_array, jcntrl_data_array, set_ivalue)   \
                                                                              \
  static void jcntrl_##type##_array_func_init(jcntrl_shared_object_funcs *p)  \
  {                                                                           \
    p->allocator = jcntrl_##type##_array_allocator;                           \
    p->deleter = jcntrl_##type##_array_deleter;                               \
    p->destructor = NULL;                                                     \
    p->initializer = NULL;                                                    \
    p->downcast = jcntrl_##type##_array_downcast_v;                           \
    JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_##type##_array, jcntrl_data_array,      \
                            element_size);                                    \
    JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_##type##_array, jcntrl_data_array,      \
                            get_value);                                       \
    JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_##type##_array, jcntrl_data_array,      \
                            set_value);                                       \
    JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_##type##_array, jcntrl_data_array,      \
                            get_ivalue);                                      \
    JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_##type##_array, jcntrl_data_array,      \
                            set_ivalue);                                      \
  }                                                                           \
                                                                              \
  JCNTRL_SHARED_METADATA_INIT_DEFINE(jcntrl_##type##_array,                   \
                                     jcntrl_##type##_array_func_init)

JCNTRL_SHARED_METADATA_DATA_ARRAY_INIT_DEFINE(char, char)
JCNTRL_SHARED_METADATA_DATA_ARRAY_INIT_DEFINE(bool, char)
JCNTRL_SHARED_METADATA_DATA_ARRAY_INIT_DEFINE(int, int)
JCNTRL_SHARED_METADATA_DATA_ARRAY_INIT_DEFINE(aint, jcntrl_aint_type)
JCNTRL_SHARED_METADATA_DATA_ARRAY_INIT_DEFINE(float, float)
JCNTRL_SHARED_METADATA_DATA_ARRAY_INIT_DEFINE(double, double)
JCNTRL_SHARED_METADATA_DATA_ARRAY_INIT_DEFINE(size, jcntrl_size_type)

jcntrl_shared_object *
jcntrl_generic_data_array_object(jcntrl_generic_data_array *ary)
{
  return jcntrl_data_array_object(jcntrl_generic_data_array_data(ary));
}

jcntrl_data_array *jcntrl_generic_data_array_data(jcntrl_generic_data_array *a)
{
  return &a->data;
}

static jcntrl_size_type
jcntrl_generic_data_array_element_size(jcntrl_generic_data_array *ary)
{
  return jcntrl_data_array_element_size(jcntrl_generic_data_array_data(ary));
}

static void jcntrl_generic_data_array_assign_readonly_data_nt(
  jcntrl_generic_data_array *ary, const void *data, jcntrl_size_type ntuple)
{
  ary->readonly_data = data;
  ary->allocated_data = NULL;
  ary->number_of_tuples = ntuple;
}

static void jcntrl_generic_data_array_assign_readonly_data_bs(
  jcntrl_generic_data_array *ary, const void *data, jcntrl_size_type bytesize)
{
  jcntrl_data_array *d;
  jcntrl_size_type el_size;

  d = jcntrl_generic_data_array_data(ary);
  el_size = jcntrl_data_array_element_size(d);
  JCNTRL_ASSERT(el_size > 0);

  jcntrl_generic_data_array_assign_readonly_data_nt(ary, data,
                                                    bytesize / el_size);
}

static jcntrl_generic_data_array *
jcntrl_generic_data_array_for_p(const jcntrl_shared_object_data *p,
                                const void *data, jcntrl_size_type size)
{
  jcntrl_shared_object *obj;
  jcntrl_generic_data_array *ary;

  JCNTRL_ASSERT(p);
  JCNTRL_ASSERT(
    jcntrl_shared_object_data_is_a(jcntrl_data_array_metadata_init(), p));

  obj = jcntrl_shared_object_new_by_meta(p);
  if (!obj)
    return NULL;

  ary = jcntrl_shared_object_downcast(jcntrl_generic_data_array, obj);
  if (!ary) {
    jcntrl_shared_object_delete(obj);
    return NULL;
  }

  jcntrl_generic_data_array_assign_readonly_data_bs(ary, data, size);
  return ary;
}

jcntrl_data_array *jcntrl_data_array_take_ownership(jcntrl_data_array *ary)
{
  if (jcntrl_shared_object_take_ownership(jcntrl_data_array_object(ary)))
    return ary;
  return NULL;
}

jcntrl_abstract_array *jcntrl_data_array_abstract(jcntrl_data_array *ary)
{
  JCNTRL_ASSERT(ary);
  return &ary->abstract;
}

jcntrl_shared_object *jcntrl_data_array_object(jcntrl_data_array *ary)
{
  JCNTRL_ASSERT(ary);
  return jcntrl_abstract_array_object(jcntrl_data_array_abstract(ary));
}

jcntrl_data_array *jcntrl_data_array_downcast(jcntrl_shared_object *obj)
{
  return jcntrl_shared_object_downcast(jcntrl_data_array, obj);
}

void jcntrl_data_array_delete(jcntrl_data_array *ary)
{
  JCNTRL_ASSERT(ary);

  jcntrl_shared_object_delete(jcntrl_data_array_object(ary));
}

jcntrl_data_array *jcntrl_data_array_resize(jcntrl_data_array *ary,
                                            jcntrl_size_type ntuple)
{
  if (jcntrl_abstract_array_resize(jcntrl_data_array_abstract(ary), ntuple))
    return ary;
  return NULL;
}

jcntrl_generic_data_array *
jcntrl_generic_data_array_resize(jcntrl_generic_data_array *ary,
                                 jcntrl_size_type ntuple)
{
  if (jcntrl_data_array_resize(jcntrl_generic_data_array_data(ary), ntuple))
    return ary;
  return NULL;
}

const char *jcntrl_data_array_name(jcntrl_data_array *ary,
                                   jcntrl_size_type *len)
{
  return jcntrl_abstract_array_name(jcntrl_data_array_abstract(ary), len);
}

jcntrl_char_array *jcntrl_data_array_name_d(jcntrl_data_array *ary)
{
  return jcntrl_abstract_array_name_d(jcntrl_data_array_abstract(ary));
}

int jcntrl_data_array_set_name(jcntrl_data_array *ary, const char *name,
                               jcntrl_size_type len)
{
  return jcntrl_abstract_array_set_name(jcntrl_data_array_abstract(ary), name,
                                        len);
}

int jcntrl_data_array_set_name_d(jcntrl_data_array *ary,
                                 jcntrl_data_array *name)
{
  return jcntrl_abstract_array_set_name_d(jcntrl_data_array_abstract(ary),
                                          name);
}

//--- functions for typed object
//--- new

jcntrl_char_array *jcntrl_char_array_new(void)
{
  return jcntrl_shared_object_new(jcntrl_char_array);
}

jcntrl_bool_array *jcntrl_bool_array_new(void)
{
  return jcntrl_shared_object_new(jcntrl_bool_array);
}

jcntrl_int_array *jcntrl_int_array_new(void)
{
  return jcntrl_shared_object_new(jcntrl_int_array);
}

jcntrl_aint_array *jcntrl_aint_array_new(void)
{
  return jcntrl_shared_object_new(jcntrl_aint_array);
}

jcntrl_float_array *jcntrl_float_array_new(void)
{
  return jcntrl_shared_object_new(jcntrl_float_array);
}

jcntrl_double_array *jcntrl_double_array_new(void)
{
  return jcntrl_shared_object_new(jcntrl_double_array);
}

jcntrl_size_array *jcntrl_size_array_new(void)
{
  return jcntrl_shared_object_new(jcntrl_size_array);
}

jcntrl_data_array *jcntrl_data_array_dup(jcntrl_data_array *inp)
{
  jcntrl_data_array *ary;
  jcntrl_shared_object *obj;
  const jcntrl_shared_object_data *p;
  jcntrl_size_type ntuple;
  jcntrl_size_type nlen;
  const char *name;

  JCNTRL_ASSERT(inp);
  p = jcntrl_data_array_element_type(inp);

  JCNTRL_ASSERT(
    jcntrl_shared_object_data_is_a(jcntrl_generic_data_array_metadata_init(),
                                   p));

  obj = jcntrl_shared_object_new_by_meta(p);
  if (!obj)
    return NULL;

  ary = jcntrl_data_array_downcast(obj);
  JCNTRL_ASSERT(ary);

  ntuple = jcntrl_data_array_get_ntuple(inp);
  if (ntuple > 0) {
    if (!jcntrl_data_array_resize(ary, ntuple)) {
      jcntrl_data_array_delete(ary);
      return NULL;
    }

    if (!jcntrl_data_array_copy(ary, inp, ntuple, 0, 0)) {
      jcntrl_data_array_delete(ary);
      return NULL;
    }
  }

  nlen = 0;
  name = jcntrl_data_array_name(inp, &nlen);
  if (name && nlen > 0) {
    if (!jcntrl_data_array_set_name(ary, name, nlen)) {
      jcntrl_data_array_delete(ary);
      return NULL;
    }
  }

  return ary;
}

//--- for

static jcntrl_shared_object *
jcntrl_typed_array_for_p(const jcntrl_shared_object_data *p, const void *data,
                         jcntrl_size_type ntuple)
{
  jcntrl_size_type element_size;
  jcntrl_shared_object *obj;
  jcntrl_generic_data_array *ary;

  JCNTRL_ASSERT(p);
  JCNTRL_ASSERT(ntuple > 0);

  obj = jcntrl_shared_object_new_by_meta(p);
  if (!obj)
    return NULL;

  ary = jcntrl_shared_object_downcast(jcntrl_generic_data_array, obj);
  JCNTRL_ASSERT(ary);
  if (!ary) {
    jcntrl_shared_object_delete(obj);
    return NULL;
  }

  jcntrl_generic_data_array_assign_readonly_data_nt(ary, data, ntuple);
  JCNTRL_ASSERT(ary->number_of_tuples == ntuple);
  return obj;
}

#define jcntrl_typed_array_for_c(cls, data, ntuple) \
  jcntrl_typed_array_for_p(JCNTRL_METADATA_INIT(cls)(), data, ntuple)

#define jcntrl_typed_array_for(cls, data, ntuple) \
  cls##_downcast(jcntrl_typed_array_for_c(cls, data, ntuple))

jcntrl_char_array *jcntrl_char_array_for(const char *data,
                                         jcntrl_size_type ntuple)
{
  return jcntrl_typed_array_for(jcntrl_char_array, data, ntuple);
}

jcntrl_bool_array *jcntrl_bool_array_for(const char *data,
                                         jcntrl_size_type ntuple)
{
  return jcntrl_typed_array_for(jcntrl_bool_array, data, ntuple);
}

jcntrl_int_array *jcntrl_int_array_for(const int *data, jcntrl_size_type ntuple)
{
  return jcntrl_typed_array_for(jcntrl_int_array, data, ntuple);
}

jcntrl_aint_array *jcntrl_aint_array_for(const jcntrl_aint_type *data,
                                         jcntrl_size_type ntuple)
{
  return jcntrl_typed_array_for(jcntrl_aint_array, data, ntuple);
}

jcntrl_float_array *jcntrl_float_array_for(const float *data,
                                           jcntrl_size_type ntuple)
{
  return jcntrl_typed_array_for(jcntrl_float_array, data, ntuple);
}

jcntrl_double_array *jcntrl_double_array_for(const double *data,
                                             jcntrl_size_type ntuple)
{
  return jcntrl_typed_array_for(jcntrl_double_array, data, ntuple);
}

jcntrl_size_array *jcntrl_size_array_for(const jcntrl_size_type *data,
                                         jcntrl_size_type ntuple)
{
  return jcntrl_typed_array_for(jcntrl_size_array, data, ntuple);
}

//---- bind

jcntrl_generic_data_array *
jcntrl_generic_data_array_bind(jcntrl_generic_data_array *ary, const void *data,
                               const jcntrl_shared_object_data *type,
                               jcntrl_size_type ntuple)
{
  JCNTRL_ASSERT(ntuple > 0);

  if (jcntrl_generic_data_array_object(ary)->metadata != type) {
    jcntrl_raise_argument_error(__FILE__, __LINE__,
                                "Invalid type for data array");
    return NULL;
  }

  jcntrl_generic_data_array_discard_data(ary);
  jcntrl_generic_data_array_assign_readonly_data_nt(ary, data, ntuple);
  return ary;
}

static jcntrl_shared_object *jcntrl_typed_array_bind_p(jcntrl_data_array *ary,
                                                       const void *data,
                                                       jcntrl_size_type ntuple)
{
  jcntrl_generic_data_array *gd;

  JCNTRL_ASSERT(ntuple > 0);

  gd = jcntrl_generic_data_array_downcast(jcntrl_data_array_object(ary));

  JCNTRL_ASSERT(gd);

  jcntrl_generic_data_array_discard_data(gd);
  jcntrl_generic_data_array_assign_readonly_data_nt(gd, data, ntuple);
  return jcntrl_generic_data_array_object(gd);
}

#define jcntrl_typed_array_bind(cls, ary, data, ntuple) \
  cls##_downcast(jcntrl_typed_array_bind_p(cls##_data(ary), data, ntuple))

jcntrl_char_array *jcntrl_char_array_bind(jcntrl_char_array *ary,
                                          const char *data,
                                          jcntrl_size_type ntuple)
{
  return jcntrl_typed_array_bind(jcntrl_char_array, ary, data, ntuple);
}

jcntrl_bool_array *jcntrl_bool_array_bind(jcntrl_bool_array *ary,
                                          const char *data,
                                          jcntrl_size_type ntuple)
{
  return jcntrl_typed_array_bind(jcntrl_bool_array, ary, data, ntuple);
}

jcntrl_int_array *jcntrl_int_array_bind(jcntrl_int_array *ary, const int *data,
                                        jcntrl_size_type ntuple)
{
  return jcntrl_typed_array_bind(jcntrl_int_array, ary, data, ntuple);
}

jcntrl_aint_array *jcntrl_aint_array_bind(jcntrl_aint_array *ary,
                                          const jcntrl_aint_type *data,
                                          jcntrl_size_type ntuple)
{
  return jcntrl_typed_array_bind(jcntrl_aint_array, ary, data, ntuple);
}

jcntrl_float_array *jcntrl_float_array_bind(jcntrl_float_array *ary,
                                            const float *data,
                                            jcntrl_size_type ntuple)
{
  return jcntrl_typed_array_bind(jcntrl_float_array, ary, data, ntuple);
}

jcntrl_double_array *jcntrl_double_array_bind(jcntrl_double_array *ary,
                                              const double *data,
                                              jcntrl_size_type ntuple)
{
  return jcntrl_typed_array_bind(jcntrl_double_array, ary, data, ntuple);
}

jcntrl_size_array *jcntrl_size_array_bind(jcntrl_size_array *ary,
                                          const jcntrl_size_type *data,
                                          jcntrl_size_type ntuple)
{
  return jcntrl_typed_array_bind(jcntrl_size_array, ary, data, ntuple);
}

//--- take_ownership

#define jcntrl_typed_array_take_ownership(cls, ary) \
  cls##_downcast(jcntrl_shared_object_take_ownership(cls##_object(ary)))

jcntrl_char_array *jcntrl_char_array_take_ownership(jcntrl_char_array *ary)
{
  return jcntrl_typed_array_take_ownership(jcntrl_char_array, ary);
}

jcntrl_bool_array *jcntrl_bool_array_take_ownership(jcntrl_bool_array *ary)
{
  return jcntrl_typed_array_take_ownership(jcntrl_bool_array, ary);
}

jcntrl_int_array *jcntrl_int_array_take_ownership(jcntrl_int_array *ary)
{
  return jcntrl_typed_array_take_ownership(jcntrl_int_array, ary);
}

jcntrl_aint_array *jcntrl_aint_array_take_ownership(jcntrl_aint_array *ary)
{
  return jcntrl_typed_array_take_ownership(jcntrl_aint_array, ary);
}

jcntrl_float_array *jcntrl_float_array_take_ownership(jcntrl_float_array *ary)
{
  return jcntrl_typed_array_take_ownership(jcntrl_float_array, ary);
}

jcntrl_double_array *
jcntrl_double_array_take_ownership(jcntrl_double_array *ary)
{
  return jcntrl_typed_array_take_ownership(jcntrl_double_array, ary);
}

jcntrl_size_array *jcntrl_size_array_take_ownership(jcntrl_size_array *ary)
{
  return jcntrl_typed_array_take_ownership(jcntrl_size_array, ary);
}

//--- delete

void jcntrl_char_array_delete(jcntrl_char_array *ary)
{
  jcntrl_shared_object_delete(jcntrl_char_array_object(ary));
}

void jcntrl_bool_array_delete(jcntrl_bool_array *ary)
{
  jcntrl_shared_object_delete(jcntrl_bool_array_object(ary));
}

void jcntrl_int_array_delete(jcntrl_int_array *ary)
{
  jcntrl_shared_object_delete(jcntrl_int_array_object(ary));
}

void jcntrl_aint_array_delete(jcntrl_aint_array *ary)
{
  jcntrl_shared_object_delete(jcntrl_aint_array_object(ary));
}

void jcntrl_float_array_delete(jcntrl_float_array *ary)
{
  jcntrl_shared_object_delete(jcntrl_float_array_object(ary));
}

void jcntrl_double_array_delete(jcntrl_double_array *ary)
{
  jcntrl_shared_object_delete(jcntrl_double_array_object(ary));
}

void jcntrl_size_array_delete(jcntrl_size_array *ary)
{
  jcntrl_shared_object_delete(jcntrl_size_array_object(ary));
}

//--- data

jcntrl_data_array *jcntrl_char_array_data(jcntrl_char_array *ary)
{
  return jcntrl_generic_data_array_data(&ary->data);
}

jcntrl_data_array *jcntrl_bool_array_data(jcntrl_bool_array *ary)
{
  return jcntrl_generic_data_array_data(&ary->data);
}

jcntrl_data_array *jcntrl_int_array_data(jcntrl_int_array *ary)
{
  return jcntrl_generic_data_array_data(&ary->data);
}

jcntrl_data_array *jcntrl_aint_array_data(jcntrl_aint_array *ary)
{
  return jcntrl_generic_data_array_data(&ary->data);
}

jcntrl_data_array *jcntrl_float_array_data(jcntrl_float_array *ary)
{
  return jcntrl_generic_data_array_data(&ary->data);
}

jcntrl_data_array *jcntrl_double_array_data(jcntrl_double_array *ary)
{
  return jcntrl_generic_data_array_data(&ary->data);
}

jcntrl_data_array *jcntrl_size_array_data(jcntrl_size_array *ary)
{
  return jcntrl_generic_data_array_data(&ary->data);
}

//--- object

jcntrl_shared_object *jcntrl_char_array_object(jcntrl_char_array *ary)
{
  return jcntrl_data_array_object(jcntrl_char_array_data(ary));
}

jcntrl_shared_object *jcntrl_bool_array_object(jcntrl_bool_array *ary)
{
  return jcntrl_data_array_object(jcntrl_bool_array_data(ary));
}

jcntrl_shared_object *jcntrl_int_array_object(jcntrl_int_array *ary)
{
  return jcntrl_data_array_object(jcntrl_int_array_data(ary));
}

jcntrl_shared_object *jcntrl_aint_array_object(jcntrl_aint_array *ary)
{
  return jcntrl_data_array_object(jcntrl_aint_array_data(ary));
}

jcntrl_shared_object *jcntrl_float_array_object(jcntrl_float_array *ary)
{
  return jcntrl_data_array_object(jcntrl_float_array_data(ary));
}

jcntrl_shared_object *jcntrl_double_array_object(jcntrl_double_array *ary)
{
  return jcntrl_data_array_object(jcntrl_double_array_data(ary));
}

jcntrl_shared_object *jcntrl_size_array_object(jcntrl_size_array *ary)
{
  return jcntrl_data_array_object(jcntrl_size_array_data(ary));
}

//--- downcast

jcntrl_generic_data_array *
jcntrl_generic_data_array_downcast(jcntrl_shared_object *obj)
{
  return jcntrl_shared_object_downcast(jcntrl_generic_data_array, obj);
}

jcntrl_char_array *jcntrl_char_array_downcast(jcntrl_shared_object *obj)
{
  return jcntrl_shared_object_downcast(jcntrl_char_array, obj);
}

jcntrl_bool_array *jcntrl_bool_array_downcast(jcntrl_shared_object *obj)
{
  return jcntrl_shared_object_downcast(jcntrl_bool_array, obj);
}

jcntrl_int_array *jcntrl_int_array_downcast(jcntrl_shared_object *obj)
{
  return jcntrl_shared_object_downcast(jcntrl_int_array, obj);
}

jcntrl_aint_array *jcntrl_aint_array_downcast(jcntrl_shared_object *obj)
{
  return jcntrl_shared_object_downcast(jcntrl_aint_array, obj);
}

jcntrl_float_array *jcntrl_float_array_downcast(jcntrl_shared_object *obj)
{
  return jcntrl_shared_object_downcast(jcntrl_float_array, obj);
}

jcntrl_double_array *jcntrl_double_array_downcast(jcntrl_shared_object *obj)
{
  return jcntrl_shared_object_downcast(jcntrl_double_array, obj);
}

jcntrl_size_array *jcntrl_size_array_downcast(jcntrl_shared_object *obj)
{
  return jcntrl_shared_object_downcast(jcntrl_size_array, obj);
}

//--- resize

static jcntrl_shared_object *
jcntrl_typed_array_resize(jcntrl_shared_object *obj, jcntrl_size_type ntuple)
{
  jcntrl_generic_data_array *gd;
  gd = jcntrl_shared_object_downcast(jcntrl_generic_data_array, obj);
  JCNTRL_ASSERT(gd);

  if (!jcntrl_generic_data_array_resize(gd, ntuple))
    return NULL;

  return jcntrl_generic_data_array_object(gd);
}

#define jcntrl_typed_array_resize(cls, ary, ntuple) \
  cls##_downcast(jcntrl_typed_array_resize(cls##_object(ary), ntuple))

jcntrl_char_array *jcntrl_char_array_resize(jcntrl_char_array *ary,
                                            jcntrl_size_type ntuple)
{
  return jcntrl_typed_array_resize(jcntrl_char_array, ary, ntuple);
}

jcntrl_bool_array *jcntrl_bool_array_resize(jcntrl_bool_array *ary,
                                            jcntrl_size_type ntuple)
{
  return jcntrl_typed_array_resize(jcntrl_bool_array, ary, ntuple);
}

jcntrl_int_array *jcntrl_int_array_resize(jcntrl_int_array *ary,
                                          jcntrl_size_type ntuple)
{
  return jcntrl_typed_array_resize(jcntrl_int_array, ary, ntuple);
}

jcntrl_aint_array *jcntrl_aint_array_resize(jcntrl_aint_array *ary,
                                            jcntrl_size_type ntuple)
{
  return jcntrl_typed_array_resize(jcntrl_aint_array, ary, ntuple);
}

jcntrl_float_array *jcntrl_float_array_resize(jcntrl_float_array *ary,
                                              jcntrl_size_type ntuple)
{
  return jcntrl_typed_array_resize(jcntrl_float_array, ary, ntuple);
}

jcntrl_double_array *jcntrl_double_array_resize(jcntrl_double_array *ary,
                                                jcntrl_size_type ntuple)
{
  return jcntrl_typed_array_resize(jcntrl_double_array, ary, ntuple);
}

jcntrl_size_array *jcntrl_size_array_resize(jcntrl_size_array *ary,
                                            jcntrl_size_type ntuple)
{
  return jcntrl_typed_array_resize(jcntrl_size_array, ary, ntuple);
}

//--- ntuple

jcntrl_size_type jcntrl_data_array_get_ntuple(jcntrl_data_array *ary)
{
  return jcntrl_abstract_array_get_ntuple(&ary->abstract);
}

jcntrl_size_type jcntrl_char_array_get_ntuple(jcntrl_char_array *ary)
{
  return jcntrl_data_array_get_ntuple(jcntrl_char_array_data(ary));
}

jcntrl_size_type jcntrl_bool_array_get_ntuple(jcntrl_bool_array *ary)
{
  return jcntrl_data_array_get_ntuple(jcntrl_bool_array_data(ary));
}

jcntrl_size_type jcntrl_int_array_get_ntuple(jcntrl_int_array *ary)
{
  return jcntrl_data_array_get_ntuple(jcntrl_int_array_data(ary));
}

jcntrl_size_type jcntrl_aint_array_get_ntuple(jcntrl_aint_array *ary)
{
  return jcntrl_data_array_get_ntuple(jcntrl_aint_array_data(ary));
}

jcntrl_size_type jcntrl_float_array_get_ntuple(jcntrl_float_array *ary)
{
  return jcntrl_data_array_get_ntuple(jcntrl_float_array_data(ary));
}

jcntrl_size_type jcntrl_double_array_get_ntuple(jcntrl_double_array *ary)
{
  return jcntrl_data_array_get_ntuple(jcntrl_double_array_data(ary));
}

jcntrl_size_type jcntrl_size_array_get_ntuple(jcntrl_size_array *ary)
{
  return jcntrl_data_array_get_ntuple(jcntrl_size_array_data(ary));
}

//--- get

const void *jcntrl_data_array_get_by_meta(jcntrl_data_array *ary,
                                          const jcntrl_shared_object_data *meta)
{
  if (jcntrl_shared_object_data_is_a(jcntrl_data_array_element_type(ary), meta))
    return jcntrl_data_array_get(ary);
  return NULL;
}

#define jcntrl_typed_data_array_get(name, eltype, ary)                      \
  ((void)sizeof(name##_get(NULL) == (const eltype *)NULL),                  \
   (const eltype *)jcntrl_data_array_get_by_meta(ary, JCNTRL_METADATA_INIT( \
                                                        name)()))

const char *jcntrl_data_array_get_char(jcntrl_data_array *ary)
{
  return jcntrl_typed_data_array_get(jcntrl_char_array, char, ary);
}

const char *jcntrl_data_array_get_bool(jcntrl_data_array *ary)
{
  return jcntrl_typed_data_array_get(jcntrl_bool_array, char, ary);
}

const int *jcntrl_data_array_get_int(jcntrl_data_array *ary)
{
  return jcntrl_typed_data_array_get(jcntrl_int_array, int, ary);
}

const jcntrl_aint_type *jcntrl_data_array_get_aint(jcntrl_data_array *ary)
{
  return jcntrl_typed_data_array_get(jcntrl_aint_array, jcntrl_aint_type, ary);
}

const float *jcntrl_data_array_get_float(jcntrl_data_array *ary)
{
  return jcntrl_typed_data_array_get(jcntrl_float_array, float, ary);
}

const double *jcntrl_data_array_get_double(jcntrl_data_array *ary)
{
  return jcntrl_typed_data_array_get(jcntrl_double_array, double, ary);
}

const jcntrl_size_type *jcntrl_data_array_get_sizes(jcntrl_data_array *ary)
{
  return jcntrl_typed_data_array_get(jcntrl_size_array, jcntrl_size_type, ary);
}

const char *jcntrl_char_array_get(jcntrl_char_array *ary)
{
  return (const char *)jcntrl_data_array_get(jcntrl_char_array_data(ary));
}

const int *jcntrl_int_array_get(jcntrl_int_array *ary)
{
  return (const int *)jcntrl_data_array_get(jcntrl_int_array_data(ary));
}

const jcntrl_aint_type *jcntrl_aint_array_get(jcntrl_aint_array *ary)
{
  return (const jcntrl_aint_type *)jcntrl_data_array_get(
    jcntrl_aint_array_data(ary));
}

const char *jcntrl_bool_array_get(jcntrl_bool_array *ary)
{
  return (const char *)jcntrl_data_array_get(jcntrl_bool_array_data(ary));
}

const float *jcntrl_float_array_get(jcntrl_float_array *ary)
{
  return (const float *)jcntrl_data_array_get(jcntrl_float_array_data(ary));
}

const double *jcntrl_double_array_get(jcntrl_double_array *ary)
{
  return (const double *)jcntrl_data_array_get(jcntrl_double_array_data(ary));
}

const jcntrl_size_type *jcntrl_size_array_get(jcntrl_size_array *ary)
{
  return (const jcntrl_size_type *)jcntrl_data_array_get(
    jcntrl_size_array_data(ary));
}

//--- get_writable

void *
jcntrl_data_array_get_writable_by_meta(jcntrl_data_array *ary,
                                       const jcntrl_shared_object_data *meta)
{
  if (jcntrl_shared_object_data_is_a(meta, jcntrl_data_array_element_type(ary)))
    return jcntrl_data_array_get_writable(ary);
  return NULL;
}

#define jcntrl_typed_data_array_get_writable(name, eltype, ary)                \
  ((void)sizeof(name##_get_writable(NULL) == (eltype *)NULL),                  \
   (eltype *)jcntrl_data_array_get_writable_by_meta(ary, JCNTRL_METADATA_INIT( \
                                                           name)()))

char *jcntrl_data_array_get_writable_char(jcntrl_data_array *ary)
{
  return jcntrl_typed_data_array_get_writable(jcntrl_char_array, char, ary);
}

char *jcntrl_data_array_get_writable_bool(jcntrl_data_array *ary)
{
  return jcntrl_typed_data_array_get_writable(jcntrl_bool_array, char, ary);
}

int *jcntrl_data_array_get_writable_int(jcntrl_data_array *ary)
{
  return jcntrl_typed_data_array_get_writable(jcntrl_int_array, int, ary);
}

jcntrl_aint_type *jcntrl_data_array_get_writable_aint(jcntrl_data_array *ary)
{
  return jcntrl_typed_data_array_get_writable(jcntrl_aint_array,
                                              jcntrl_aint_type, ary);
}

float *jcntrl_data_array_get_writable_float(jcntrl_data_array *ary)
{
  return jcntrl_typed_data_array_get_writable(jcntrl_float_array, float, ary);
}

double *jcntrl_data_array_get_writable_double(jcntrl_data_array *ary)
{
  return jcntrl_typed_data_array_get_writable(jcntrl_double_array, double, ary);
}

jcntrl_size_type *jcntrl_data_array_get_writable_sizes(jcntrl_data_array *ary)
{
  return jcntrl_typed_data_array_get_writable(jcntrl_size_array,
                                              jcntrl_size_type, ary);
}

char *jcntrl_char_array_get_writable(jcntrl_char_array *ary)
{
  return (char *)jcntrl_data_array_get_writable(jcntrl_char_array_data(ary));
}

char *jcntrl_bool_array_get_writable(jcntrl_bool_array *ary)
{
  return (char *)jcntrl_data_array_get_writable(jcntrl_bool_array_data(ary));
}

int *jcntrl_int_array_get_writable(jcntrl_int_array *ary)
{
  return (int *)jcntrl_data_array_get_writable(jcntrl_int_array_data(ary));
}

jcntrl_aint_type *jcntrl_aint_array_get_writable(jcntrl_aint_array *ary)
{
  return (jcntrl_aint_type *)jcntrl_data_array_get_writable(
    jcntrl_aint_array_data(ary));
}

float *jcntrl_float_array_get_writable(jcntrl_float_array *ary)
{
  return (float *)jcntrl_data_array_get_writable(jcntrl_float_array_data(ary));
}

double *jcntrl_double_array_get_writable(jcntrl_double_array *ary)
{
  return (double *)jcntrl_data_array_get_writable(
    jcntrl_double_array_data(ary));
}

jcntrl_size_type *jcntrl_size_array_get_writable(jcntrl_size_array *ary)
{
  return (jcntrl_size_type *)jcntrl_data_array_get_writable(
    jcntrl_size_array_data(ary));
}

//--- copyable

int jcntrl_data_array_copyable_tt(const jcntrl_shared_object_data *desttype,
                                  const jcntrl_shared_object_data *srctype)
{
  const jcntrl_shared_object_data *g;
  g = jcntrl_generic_data_array_metadata_init();
  if (desttype == g || srctype == g)
    return 0;

  if (!jcntrl_shared_object_data_is_a(g, desttype))
    return 0;

  if (!jcntrl_shared_object_data_is_a(g, srctype))
    return 0;

  return jcntrl_shared_object_data_is_a(desttype, srctype);
}

int jcntrl_data_array_copyable_ta(const jcntrl_shared_object_data *desttype,
                                  jcntrl_data_array *src)
{
  return jcntrl_data_array_copyable_tt(desttype,
                                       jcntrl_data_array_element_type(src));
}

int jcntrl_data_array_copyable_at(jcntrl_data_array *dest,
                                  const jcntrl_shared_object_data *srctype)
{
  return jcntrl_data_array_copyable_tt(jcntrl_data_array_element_type(dest),
                                       srctype);
}

int jcntrl_data_array_copyable(jcntrl_data_array *dest, jcntrl_data_array *src)
{
  return jcntrl_data_array_copyable_tt(jcntrl_data_array_element_type(dest),
                                       jcntrl_data_array_element_type(src));
}

int jcntrl_char_array_copyable(jcntrl_data_array *src)
{
  return jcntrl_data_array_copyable_tt(jcntrl_char_array_metadata_init(),
                                       jcntrl_data_array_element_type(src));
}

int jcntrl_bool_array_copyable(jcntrl_data_array *src)
{
  return jcntrl_data_array_copyable_tt(jcntrl_bool_array_metadata_init(),
                                       jcntrl_data_array_element_type(src));
}

int jcntrl_int_array_copyable(jcntrl_data_array *src)
{
  return jcntrl_data_array_copyable_tt(jcntrl_int_array_metadata_init(),
                                       jcntrl_data_array_element_type(src));
}

int jcntrl_double_array_copyable(jcntrl_data_array *src)
{
  return jcntrl_data_array_copyable_tt(jcntrl_double_array_metadata_init(),
                                       jcntrl_data_array_element_type(src));
}

int jcntrl_size_array_copyable(jcntrl_data_array *src)
{
  return jcntrl_data_array_copyable_tt(jcntrl_size_array_metadata_init(),
                                       jcntrl_data_array_element_type(src));
}

int jcntrl_aint_array_copyable(jcntrl_data_array *src)
{
  return jcntrl_data_array_copyable_tt(jcntrl_aint_array_metadata_init(),
                                       jcntrl_data_array_element_type(src));
}

//--- copy

static int jcntrl_data_array_copy_range_chk(jcntrl_size_type number_of_tuples,
                                            jcntrl_size_type ist,
                                            jcntrl_size_type *ntuple)
{
  if (*ntuple <= 0)
    return 1;

  if (ist < 0 || ist >= number_of_tuples) {
    jcntrl_raise_index_error(__FILE__, __LINE__, ist);
    return 0;
  }

  if (JCNTRL_SIZE_MAX - ist < *ntuple) {
    jcntrl_raise_overflow_error(__FILE__, __LINE__);
    return 0;
  }

  if (ist + *ntuple > number_of_tuples)
    *ntuple = number_of_tuples - ist;
  return 1;
}

int jcntrl_data_array_copy(jcntrl_data_array *dest, jcntrl_data_array *src,
                           jcntrl_size_type ntuple, jcntrl_size_type idest,
                           jcntrl_size_type skip)
{
  void *raw_dst;
  const void *raw_src;
  jcntrl_size_type elsize, ibskip, obskip, nbcopy;
  jcntrl_size_type intuple, sntuple;
  jcntrl_size_type dst_elsize, src_elsize;

  src_elsize = jcntrl_data_array_element_size(src);
  dst_elsize = jcntrl_data_array_element_size(dest);

  JCNTRL_ASSERT(dest);
  JCNTRL_ASSERT(src);
  JCNTRL_ASSERT(ntuple >= 0);
  JCNTRL_ASSERT(src_elsize > 0);
  JCNTRL_ASSERT(dst_elsize == src_elsize);
  JCNTRL_ASSERT(jcntrl_data_array_copyable(dest, src));

  sntuple = jcntrl_data_array_get_ntuple(dest);
  intuple = jcntrl_data_array_get_ntuple(src);

  if (!jcntrl_data_array_copy_range_chk(sntuple, idest, &ntuple))
    return 0;
  if (!jcntrl_data_array_copy_range_chk(intuple, skip, &ntuple))
    return 0;

  if (ntuple <= 0)
    return 1;

  elsize = src_elsize;
  obskip = elsize * idest;
  ibskip = elsize * skip;
  nbcopy = elsize * ntuple;

  raw_src = jcntrl_data_array_get(src);
  raw_dst = jcntrl_data_array_get_writable(dest);
  if (!raw_dst)
    return 0;

  memmove((char *)raw_dst + obskip, (char *)raw_src + ibskip, nbcopy);
  return 1;
}

int jcntrl_char_array_copy(jcntrl_char_array *dest, jcntrl_data_array *src,
                           jcntrl_size_type ntuple, jcntrl_size_type idest,
                           jcntrl_size_type iskip)
{
  return jcntrl_data_array_copy(jcntrl_char_array_data(dest), src, ntuple,
                                idest, iskip);
}

int jcntrl_bool_array_copy(jcntrl_bool_array *dest, jcntrl_data_array *src,
                           jcntrl_size_type ntuple, jcntrl_size_type idest,
                           jcntrl_size_type iskip)
{
  return jcntrl_data_array_copy(jcntrl_bool_array_data(dest), src, ntuple,
                                idest, iskip);
}

int jcntrl_int_array_copy(jcntrl_int_array *dest, jcntrl_data_array *src,
                          jcntrl_size_type ntuple, jcntrl_size_type idest,
                          jcntrl_size_type iskip)
{
  return jcntrl_data_array_copy(jcntrl_int_array_data(dest), src, ntuple, idest,
                                iskip);
}

int jcntrl_aint_array_copy(jcntrl_aint_array *dest, jcntrl_data_array *src,
                           jcntrl_size_type ntuple, jcntrl_size_type idest,
                           jcntrl_size_type iskip)
{
  return jcntrl_data_array_copy(jcntrl_aint_array_data(dest), src, ntuple,
                                idest, iskip);
}

int jcntrl_float_array_copy(jcntrl_float_array *dest, jcntrl_data_array *src,
                            jcntrl_size_type ntuple, jcntrl_size_type idest,
                            jcntrl_size_type iskip)
{
  return jcntrl_data_array_copy(jcntrl_float_array_data(dest), src, ntuple,
                                idest, iskip);
}

int jcntrl_double_array_copy(jcntrl_double_array *dest, jcntrl_data_array *src,
                             jcntrl_size_type ntuple, jcntrl_size_type idest,
                             jcntrl_size_type iskip)
{
  return jcntrl_data_array_copy(jcntrl_double_array_data(dest), src, ntuple,
                                idest, iskip);
}

int jcntrl_size_array_copy(jcntrl_size_array *dest, jcntrl_data_array *src,
                           jcntrl_size_type ntuple, jcntrl_size_type idest,
                           jcntrl_size_type iskip)
{
  return jcntrl_data_array_copy(jcntrl_size_array_data(dest), src, ntuple,
                                idest, iskip);
}

int jcntrl_data_array_copyidx(jcntrl_data_array *dest, jcntrl_data_array *src,
                              jcntrl_data_array *dstidx,
                              jcntrl_data_array *srcidx)
{
  void *raw_dst;
  const void *raw_src;
  jcntrl_size_type elsize;
  jcntrl_size_type ituple, otuple;
  jcntrl_size_type ivtuple, ovtuple;

  JCNTRL_ASSERT(dest);
  JCNTRL_ASSERT(src);
  JCNTRL_ASSERT(
    jcntrl_shared_object_data_is_a(jcntrl_data_array_element_type(dest),
                                   jcntrl_data_array_element_type(src)));

  elsize = jcntrl_data_array_element_size(src);
  JCNTRL_ASSERT(jcntrl_data_array_element_size(dest) == elsize);

  ovtuple = jcntrl_data_array_get_ntuple(dest);
  ivtuple = jcntrl_data_array_get_ntuple(src);
  otuple = dstidx ? jcntrl_data_array_get_ntuple(dstidx) : ovtuple;
  ituple = srcidx ? jcntrl_data_array_get_ntuple(srcidx) : ivtuple;
  if (dstidx) {
    if (otuple != ituple) {
      jcntrl_raise_argument_error(__FILE__, __LINE__,
                                  "Copy size does not match");
      return 0;
    }
  } else {
    if (otuple < ituple) {
      if (!jcntrl_data_array_resize(dest, ituple))
        return 0;
    }
  }

  for (jcntrl_size_type i = 0; i < ituple; ++i) {
    intmax_t iidx, oidx;
    int ierr, jerr;

    ierr = jerr = 0;
    iidx = srcidx ? jcntrl_data_array_get_ivalue(srcidx, i, &ierr) : i;
    oidx = dstidx ? jcntrl_data_array_get_ivalue(dstidx, i, &jerr) : i;

    if (ierr || jerr)
      return 0;

    if (iidx < 0 || iidx >= ivtuple) {
      jcntrl_raise_index_error(__FILE__, __LINE__, iidx);
      return 0;
    }
    if (oidx < 0 || oidx >= ovtuple) {
      jcntrl_raise_index_error(__FILE__, __LINE__, oidx);
      return 0;
    }
  }

  raw_dst = jcntrl_data_array_get_writable(dest);
  raw_src = jcntrl_data_array_get(src);

  for (jcntrl_size_type i = 0; i < ituple; ++i) {
    intmax_t iidx, oidx;
    int ierr, jerr;

    ierr = jerr = 0;
    iidx = srcidx ? jcntrl_data_array_get_ivalue(srcidx, i, &ierr) : i;
    oidx = dstidx ? jcntrl_data_array_get_ivalue(dstidx, i, &jerr) : i;
    JCNTRL_ASSERT(ierr == 0);
    JCNTRL_ASSERT(jerr == 0);

    memcpy((char *)raw_dst + oidx * elsize,
           (const char *)raw_src + iidx * elsize, elsize);
  }
  return 1;
}

//--- utility

char *jcntrl_char_array_make_cstr(jcntrl_data_array *array)
{
  char *p;
  const char *str;
  jcntrl_size_type ntuple, asz;

  if (!jcntrl_char_array_copyable(array)) {
    jcntrl_raise_argument_error(__FILE__, __LINE__,
                                "Given array is not char typed array");
    return NULL;
  }

  str = jcntrl_data_array_get_char(array);
  ntuple = jcntrl_data_array_get_ntuple(array);
  if (ntuple < 0)
    return NULL;

  if (!str)
    ntuple = 0;

  asz = ntuple;
  if (ntuple == 0 || str[ntuple - 1] != '\0') {
    if (jcntrl_s_add_overflow(ntuple, 1, &asz))
      return NULL;
  }

  p = malloc(asz * sizeof(char));
  if (!p) {
    jcntrl_raise_allocation_failed(__FILE__, __LINE__);
    return NULL;
  }

  if (ntuple > 0)
    memcpy(p, str, ntuple);
  if (asz > ntuple)
    p[asz - 1] = '\0';
  return p;
}

static void jcntrl_char_array_work_cstrp(const char *src, jcntrl_size_type insz,
                                         jcntrl_size_type asz,
                                         jcntrl_char_array_work_cstr_func *func,
                                         void *arg)
{
  char str[asz];
  if (insz > 0)
    memcpy(str, src, insz);
  if (insz < asz)
    str[asz - 1] = '\0';

  func(str, asz, arg);
}

int jcntrl_char_array_work_cstr(jcntrl_data_array *array,
                                jcntrl_char_array_work_cstr_func *func,
                                void *arg)
{
  const char *str;
  jcntrl_size_type ntuple, asz;

  if (!jcntrl_char_array_copyable(array)) {
    jcntrl_raise_argument_error(__FILE__, __LINE__,
                                "Given array is not char typed array");
    return 0;
  }

  str = jcntrl_data_array_get_char(array);
  ntuple = jcntrl_data_array_get_ntuple(array);
  if (ntuple < 0)
    return 0;

  if (!str)
    ntuple = 0;

  asz = ntuple;
  if (ntuple == 0 || str[ntuple - 1] != '\0') {
    if (jcntrl_s_add_overflow(ntuple, 1, &asz))
      return 0;
  }

  jcntrl_char_array_work_cstrp(str, ntuple, asz, func, arg);
  return 1;
}

struct jcntrl_char_array_strtod_data
{
  double d;
  jcntrl_size_type *len;
};

static void jcntrl_char_array_strtod_wrk(char *str, jcntrl_size_type len,
                                         void *arg)
{
  char *ep;
  struct jcntrl_char_array_strtod_data *p;
  p = arg;
  p->d = strtod(str, &ep);
  if (p->len)
    *p->len = ep - str;
}

double jcntrl_char_array_strtod(jcntrl_data_array *array, jcntrl_size_type *len)
{
  struct jcntrl_char_array_strtod_data d = {
    .d = 0.0,
    .len = len,
  };
  if (!jcntrl_char_array_work_cstr(array, jcntrl_char_array_strtod_wrk, &d)) {
    if (len)
      *len = -1;
  }
  return d.d;
}

struct jcntrl_char_array_strtof_data
{
  float d;
  jcntrl_size_type *len;
};

static void jcntrl_char_array_strtof_wrk(char *str, jcntrl_size_type len,
                                         void *arg)
{
  char *ep;
  struct jcntrl_char_array_strtof_data *p;
  p = arg;
  p->d = strtof(str, &ep);
  if (p->len)
    *p->len = ep - str;
}

float jcntrl_char_array_strtof(jcntrl_data_array *array, jcntrl_size_type *len)
{
  struct jcntrl_char_array_strtof_data d = {
    .d = 0.0f,
    .len = len,
  };
  if (!jcntrl_char_array_work_cstr(array, jcntrl_char_array_strtof_wrk, &d)) {
    if (len)
      *len = -1;
  }
  return d.d;
}

struct jcntrl_char_array_strtol_data
{
  long d;
  int base;
  jcntrl_size_type *len;
};

static void jcntrl_char_array_strtol_wrk(char *str, jcntrl_size_type len,
                                         void *arg)
{
  char *ep;
  struct jcntrl_char_array_strtol_data *p;
  p = arg;
  p->d = strtol(str, &ep, p->base);
  if (p->len)
    *p->len = ep - str;
}

long jcntrl_char_array_strtol(jcntrl_data_array *array, int base,
                              jcntrl_size_type *len)
{
  struct jcntrl_char_array_strtol_data d = {
    .d = 0,
    .base = base,
    .len = len,
  };
  if (!jcntrl_char_array_work_cstr(array, jcntrl_char_array_strtol_wrk, &d)) {
    if (len)
      *len = -1;
  }
  return d.d;
}

struct jcntrl_char_array_strtoll_data
{
  long long d;
  int base;
  jcntrl_size_type *len;
};

static void jcntrl_char_array_strtoll_wrk(char *str, jcntrl_size_type len,
                                          void *arg)
{
  char *ep;
  struct jcntrl_char_array_strtoll_data *p;
  p = arg;
  p->d = strtoll(str, &ep, p->base);
  if (p->len)
    *p->len = ep - str;
}

long long jcntrl_char_array_strtoll(jcntrl_data_array *array, int base,
                                    jcntrl_size_type *len)
{
  struct jcntrl_char_array_strtoll_data d = {
    .d = 0,
    .base = base,
    .len = len,
  };
  if (!jcntrl_char_array_work_cstr(array, jcntrl_char_array_strtoll_wrk, &d)) {
    if (len)
      *len = -1;
  }
  return d.d;
}

const char *jcntrl_get_string_c(jcntrl_char_array *src, jcntrl_size_type *len)
{
  if (!src) {
    if (len)
      *len = 0;
    return NULL;
  }

  if (len)
    *len = jcntrl_char_array_get_ntuple(src);
  return jcntrl_char_array_get(src);
}

int jcntrl_set_string(jcntrl_char_array **dest, jcntrl_data_array *src)
{
  jcntrl_size_type n;

  JCNTRL_ASSERT(dest);
  JCNTRL_ASSERT(!src || jcntrl_char_array_copyable(src));

  n = 0;
  if (src)
    n = jcntrl_data_array_get_ntuple(src);
  if (n > 0) {
    if (!*dest) {
      *dest = jcntrl_char_array_new();
      if (!*dest)
        return 0;
    }
    if (!jcntrl_char_array_resize(*dest, n))
      return 0;
    if (!jcntrl_char_array_copy(*dest, src, n, 0, 0)) {
      jcntrl_char_array_resize(*dest, 0);
      return 0;
    }
    return 1;
  }

  if (*dest)
    jcntrl_char_array_resize(*dest, 0);
  return 1;
}

int jcntrl_set_string_c(jcntrl_char_array **dest, const char *str,
                        jcntrl_size_type len)
{
  jcntrl_static_cstr_array cstr;
  jcntrl_data_array *d;

  if (!str)
    return jcntrl_set_string(dest, NULL);

  if (len < 0)
    len = strlen(str);

  jcntrl_static_cstr_array_init_base(&cstr, str, len);
  d = jcntrl_static_cstr_array_data(&cstr);
  return jcntrl_set_string(dest, d);
}
