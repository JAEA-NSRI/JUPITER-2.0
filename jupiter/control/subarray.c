#include "subarray.h"
#include "abstract_array.h"
#include "data_array.h"
#include "data_array_data.h"
#include "defs.h"
#include "error.h"
#include "overflow.h"
#include "shared_object.h"
#include "shared_object_priv.h"

static jcntrl_data_subarray *
jcntrl_data_subarray_downcast_impl(jcntrl_shared_object *obj)
{
  return JCNTRL_DOWNCAST_IMPL(jcntrl_data_subarray, obj);
}

static void *jcntrl_data_subarray_downcast_v(jcntrl_shared_object *obj)
{
  return jcntrl_data_subarray_downcast_impl(obj);
}

static int jcntrl_data_subarray_initializer(jcntrl_shared_object *obj)
{
  jcntrl_data_subarray *ary;
  ary = jcntrl_data_subarray_downcast_impl(obj);
  *(jcntrl_data_array **)&ary->source = NULL;
  ary->offset = 0;
  ary->viewsize = 0;
  return 1;
}

static void jcntrl_data_subarray_destructor(jcntrl_shared_object *obj)
{
  jcntrl_data_subarray *ary;
  ary = jcntrl_data_subarray_downcast_impl(obj);
  if (ary->source)
    jcntrl_data_array_delete(ary->source);
  *(jcntrl_data_array **)&ary->source = NULL;
  ary->offset = 0;
  ary->viewsize = 0;
}

static jcntrl_shared_object *jcntrl_data_subarray_allocator(void)
{
  jcntrl_data_subarray *p;
  p = jcntrl_shared_object_default_allocator(jcntrl_data_subarray);
  return p ? jcntrl_data_subarray_object(p) : NULL;
}

static void jcntrl_data_subarray_deleter(jcntrl_shared_object *obj)
{
  jcntrl_shared_object_default_deleter(obj);
}

static jcntrl_size_type
jcntrl_data_subarray_element_size_impl(jcntrl_shared_object *obj)
{
  jcntrl_data_subarray *ary;
  ary = jcntrl_data_subarray_downcast_impl(obj);
  if (ary->source)
    return jcntrl_data_array_element_size(ary->source);
  return 0;
}

JCNTRL_VIRTUAL_WRAP(jcntrl_data_subarray, jcntrl_data_array, element_size)

static const jcntrl_shared_object_data *
jcntrl_data_subarray_element_type_impl(jcntrl_shared_object *obj)
{
  jcntrl_data_subarray *ary;
  ary = jcntrl_data_subarray_downcast_impl(obj);
  if (ary->source)
    return jcntrl_data_array_element_type(ary->source);
  return jcntrl_data_array_element_type__super(
    jcntrl_data_array_metadata_init());
}

JCNTRL_VIRTUAL_WRAP(jcntrl_data_subarray, jcntrl_data_array, element_type)

static jcntrl_size_type
jcntrl_data_subarray_get_ntuple_impl(jcntrl_shared_object *obj)
{
  jcntrl_data_subarray *ary;

  ary = jcntrl_data_subarray_downcast_impl(obj);
  if (ary->source)
    return ary->viewsize;

  return jcntrl_abstract_array_get_ntuple__super(
    jcntrl_abstract_array_metadata_init());
}

JCNTRL_VIRTUAL_WRAP(jcntrl_data_subarray, jcntrl_abstract_array, get_ntuple)

static const void *jcntrl_data_subarray_get_impl(jcntrl_shared_object *obj)
{
  const void *p;
  jcntrl_data_subarray *ary;
  jcntrl_size_type elsize;

  ary = jcntrl_data_subarray_downcast_impl(obj);
  if (!ary->source)
    return NULL;

  p = jcntrl_data_array_get(ary->source);
  if (!p)
    return NULL;

  elsize = jcntrl_data_array_element_size(ary->source);
  return (const char *)p + elsize * ary->offset;
}

JCNTRL_VIRTUAL_WRAP(jcntrl_data_subarray, jcntrl_data_array, get)

static void *jcntrl_data_subarray_get_writable_impl(jcntrl_shared_object *obj)
{
  void *p;
  jcntrl_data_subarray *ary;
  jcntrl_size_type elsize;

  ary = jcntrl_data_subarray_downcast_impl(obj);
  if (!ary->source)
    return NULL;

  p = jcntrl_data_array_get_writable(ary->source);
  if (!p)
    return NULL;

  elsize = jcntrl_data_array_element_size(ary->source);
  return (char *)p + elsize * ary->offset;
}

JCNTRL_VIRTUAL_WRAP(jcntrl_data_subarray, jcntrl_data_array, get_writable)

static double jcntrl_data_subarray_get_value_impl(jcntrl_shared_object *obj,
                                                  jcntrl_size_type index)
{
  jcntrl_data_subarray *ary;

  ary = jcntrl_data_subarray_downcast_impl(obj);
  if (!ary->source)
    return 0.0;

  if (index < 0 || index >= ary->viewsize) {
    jcntrl_raise_index_error(__FILE__, __LINE__, index);
    return 0.0;
  }

  return jcntrl_data_array_get_value(ary->source, index + ary->offset);
}

JCNTRL_VIRTUAL_WRAP(jcntrl_data_subarray, jcntrl_data_array, get_value)

static int jcntrl_data_subarray_set_value_impl(jcntrl_shared_object *obj,
                                               jcntrl_size_type index,
                                               double value)
{
  jcntrl_data_subarray *ary;

  ary = jcntrl_data_subarray_downcast_impl(obj);
  if (!ary->source)
    return 0;

  if (index < 0 || index >= ary->viewsize) {
    jcntrl_raise_index_error(__FILE__, __LINE__, index);
    return 0;
  }

  return jcntrl_data_array_set_value(ary->source, index + ary->offset, value);
}

JCNTRL_VIRTUAL_WRAP(jcntrl_data_subarray, jcntrl_data_array, set_value)

static intmax_t jcntrl_data_subarray_get_ivalue_impl(jcntrl_shared_object *obj,
                                                     jcntrl_size_type index,
                                                     int *err)
{
  jcntrl_data_subarray *ary;

  ary = jcntrl_data_subarray_downcast_impl(obj);
  if (!ary->source) {
    if (err)
      *err = 1;
    return 0;
  }

  if (index < 0 || index >= ary->viewsize) {
    jcntrl_raise_index_error(__FILE__, __LINE__, index);
    if (err)
      *err = 1;
    return 0;
  }

  return jcntrl_data_array_get_ivalue(ary->source, index + ary->offset, err);
}

JCNTRL_VIRTUAL_WRAP(jcntrl_data_subarray, jcntrl_data_array, get_ivalue)

static int jcntrl_data_subarray_set_ivalue_impl(jcntrl_shared_object *obj,
                                                jcntrl_size_type index,
                                                intmax_t value)
{
  jcntrl_data_subarray *ary;

  ary = jcntrl_data_subarray_downcast_impl(obj);
  if (!ary->source)
    return 0;

  if (index < 0 || index >= ary->viewsize) {
    jcntrl_raise_index_error(__FILE__, __LINE__, index);
    return 0;
  }

  return jcntrl_data_array_set_ivalue(ary->source, index + ary->offset, value);
}

JCNTRL_VIRTUAL_WRAP(jcntrl_data_subarray, jcntrl_data_array, set_ivalue)

static void jcntrl_data_subarray_init_func(jcntrl_shared_object_funcs *p)
{
  p->downcast = jcntrl_data_subarray_downcast_v;
  p->initializer = jcntrl_data_subarray_initializer;
  p->destructor = jcntrl_data_subarray_destructor;
  p->allocator = jcntrl_data_subarray_allocator;
  p->deleter = jcntrl_data_subarray_deleter;
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_data_subarray, jcntrl_abstract_array,
                          get_ntuple);
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_data_subarray, jcntrl_data_array,
                          element_size);
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_data_subarray, jcntrl_data_array,
                          element_type);
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_data_subarray, jcntrl_data_array, get);
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_data_subarray, jcntrl_data_array,
                          get_writable);
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_data_subarray, jcntrl_data_array,
                          get_value);
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_data_subarray, jcntrl_data_array,
                          set_value);
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_data_subarray, jcntrl_data_array,
                          get_ivalue);
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_data_subarray, jcntrl_data_array,
                          set_ivalue);
}

JCNTRL_SHARED_METADATA_INIT_DEFINE(jcntrl_data_subarray,
                                   jcntrl_data_subarray_init_func)

static int jcntrl_data_subarray_bind(jcntrl_data_subarray *ary,
                                     jcntrl_data_array *source,
                                     jcntrl_size_type offset,
                                     jcntrl_size_type viewsize)
{
  if (ary->source)
    jcntrl_data_array_delete(ary->source);
  *(jcntrl_data_array **)&ary->source =
    jcntrl_data_array_take_ownership(source);
  return jcntrl_data_subarray_set_view(ary, offset, viewsize);
}

int jcntrl_data_subarray_static_init(jcntrl_data_subarray *ary,
                                     jcntrl_data_array *source,
                                     jcntrl_size_type offset,
                                     jcntrl_size_type viewsize)
{
  int r;
  jcntrl_shared_object_static_init(jcntrl_data_subarray_object(ary),
                                   jcntrl_data_subarray_metadata_init());

  r = jcntrl_data_subarray_bind(ary, source, offset, viewsize);
  if (!r)
    jcntrl_data_subarray_delete(ary);
  return r;
}

jcntrl_data_subarray *jcntrl_data_subarray_new(jcntrl_data_array *source,
                                               jcntrl_size_type offset,
                                               jcntrl_size_type viewsize)
{
  jcntrl_data_subarray *p;
  p = jcntrl_shared_object_new(jcntrl_data_subarray);
  if (!p)
    return NULL;

  if (!jcntrl_data_subarray_bind(p, source, offset, viewsize)) {
    jcntrl_data_subarray_delete(p);
    return NULL;
  }

  return p;
}

void jcntrl_data_subarray_delete(jcntrl_data_subarray *ary)
{
  jcntrl_shared_object_delete(jcntrl_data_subarray_object(ary));
}

jcntrl_data_array *jcntrl_data_subarray_data(jcntrl_data_subarray *ary)
{
  return &ary->data;
}

jcntrl_shared_object *jcntrl_data_subarray_object(jcntrl_data_subarray *ary)
{
  return jcntrl_data_array_object(jcntrl_data_subarray_data(ary));
}

jcntrl_data_subarray *jcntrl_data_subarray_downcast(jcntrl_shared_object *obj)
{
  return jcntrl_shared_object_downcast(jcntrl_data_subarray, obj);
}

jcntrl_data_array *jcntrl_data_subarray_source(jcntrl_data_subarray *ary)
{
  return ary->source;
}

const void *jcntrl_data_subarray_get(jcntrl_data_subarray *ary)
{
  return jcntrl_data_array_get(jcntrl_data_subarray_data(ary));
}

void *jcntrl_data_subarray_get_writable(jcntrl_data_subarray *ary)
{
  return jcntrl_data_array_get_writable(jcntrl_data_subarray_data(ary));
}

jcntrl_size_type jcntrl_data_subarray_get_ntuple(jcntrl_data_subarray *ary)
{
  return jcntrl_data_array_get_ntuple(jcntrl_data_subarray_data(ary));
}

jcntrl_size_type jcntrl_data_subarray_element_size(jcntrl_data_subarray *ary)
{
  return jcntrl_data_array_element_size(jcntrl_data_subarray_data(ary));
}

const jcntrl_shared_object_data *
jcntrl_data_subarray_element_type(jcntrl_data_subarray *ary)
{
  return jcntrl_data_array_element_type(jcntrl_data_subarray_data(ary));
}

double jcntrl_data_subarray_get_value(jcntrl_data_subarray *ary,
                                      jcntrl_size_type index)
{
  return jcntrl_data_array_get_value(jcntrl_data_subarray_data(ary), index);
}

int jcntrl_data_subarray_set_view(jcntrl_data_subarray *ary,
                                  jcntrl_size_type offset,
                                  jcntrl_size_type viewsize)
{
  jcntrl_size_type ntuple;
  jcntrl_size_type d;

  JCNTRL_ASSERT(ary);
  JCNTRL_ASSERT(viewsize > 0);
  JCNTRL_ASSERT(offset >= 0);

  if (!ary->source)
    return 0;

  ntuple = jcntrl_data_array_get_ntuple(ary->source);
  if (offset < 0 || offset >= ntuple) {
    jcntrl_raise_index_error(__FILE__, __LINE__, offset);
    return 0;
  }

  if (jcntrl_s_add_overflow(offset, viewsize, &d))
    return 0;

  if (offset + viewsize > ntuple) {
    jcntrl_raise_index_error(__FILE__, __LINE__, offset + viewsize);
    return 0;
  }

  ary->offset = offset;
  ary->viewsize = viewsize;
  return 1;
}

static const void *
jcntrl_typed_subarray_get(const jcntrl_shared_object_data *exptype,
                          jcntrl_data_subarray *ary)
{
  const jcntrl_shared_object_data *eltype;

  eltype = jcntrl_data_array_element_type(jcntrl_data_subarray_data(ary));
  if (jcntrl_shared_object_data_is_a(exptype, eltype))
    return jcntrl_data_subarray_get(ary);
  return NULL;
}

#define jcntrl_typed_subarray_get_typechk(type, eltype) \
  ((void)sizeof(type##_get((type *)NULL) == (const eltype *)NULL))

#define jcntrl_typed_subarray_get(type, eltype, ary)                       \
  (jcntrl_typed_subarray_get_typechk(type, eltype),                        \
   (const eltype *)jcntrl_typed_subarray_get(JCNTRL_METADATA_INIT(type)(), \
                                             ary))

const char *jcntrl_data_subarray_get_char(jcntrl_data_subarray *ary)
{
  return jcntrl_typed_subarray_get(jcntrl_char_array, char, ary);
}

const char *jcntrl_data_subarray_get_bool(jcntrl_data_subarray *ary)
{
  return jcntrl_typed_subarray_get(jcntrl_bool_array, char, ary);
}

const int *jcntrl_data_subarray_get_int(jcntrl_data_subarray *ary)
{
  return jcntrl_typed_subarray_get(jcntrl_int_array, int, ary);
}

const jcntrl_aint_type *jcntrl_data_subarray_get_aint(jcntrl_data_subarray *ary)
{
  return jcntrl_typed_subarray_get(jcntrl_aint_array, jcntrl_aint_type, ary);
}

const float *jcntrl_data_subarray_get_float(jcntrl_data_subarray *ary)
{
  return jcntrl_typed_subarray_get(jcntrl_float_array, float, ary);
}

const double *jcntrl_data_subarray_get_double(jcntrl_data_subarray *ary)
{
  return jcntrl_typed_subarray_get(jcntrl_double_array, double, ary);
}

const jcntrl_size_type *jcntrl_data_subarray_get_size(jcntrl_data_subarray *ary)
{
  return jcntrl_typed_subarray_get(jcntrl_size_array, jcntrl_size_type, ary);
}

static void *
jcntrl_typed_subarray_get_writable(const jcntrl_shared_object_data *exptype,
                                   jcntrl_data_subarray *ary)
{
  const jcntrl_shared_object_data *eltype;

  eltype = jcntrl_data_array_element_type(jcntrl_data_subarray_data(ary));
  if (jcntrl_shared_object_data_is_a(exptype, eltype))
    return jcntrl_data_subarray_get_writable(ary);
  return NULL;
}

#define jcntrl_typed_subarray_get_writable(type, eltype, ary)                 \
  (jcntrl_typed_subarray_get_typechk(type, eltype),                           \
   (eltype *)jcntrl_typed_subarray_get_writable(JCNTRL_METADATA_INIT(type)(), \
                                                ary))

char *jcntrl_data_subarray_get_writable_char(jcntrl_data_subarray *ary)
{
  return jcntrl_typed_subarray_get_writable(jcntrl_char_array, char, ary);
}

char *jcntrl_data_subarray_get_writable_bool(jcntrl_data_subarray *ary)
{
  return jcntrl_typed_subarray_get_writable(jcntrl_bool_array, char, ary);
}

int *jcntrl_data_subarray_get_writable_int(jcntrl_data_subarray *ary)
{
  return jcntrl_typed_subarray_get_writable(jcntrl_int_array, int, ary);
}

jcntrl_aint_type *
jcntrl_data_subarray_get_writable_aint(jcntrl_data_subarray *ary)
{
  return jcntrl_typed_subarray_get_writable(jcntrl_aint_array, jcntrl_aint_type,
                                            ary);
}

float *jcntrl_data_subarray_get_writable_float(jcntrl_data_subarray *ary)
{
  return jcntrl_typed_subarray_get_writable(jcntrl_float_array, float, ary);
}

double *jcntrl_data_subarray_get_writable_double(jcntrl_data_subarray *ary)
{
  return jcntrl_typed_subarray_get_writable(jcntrl_double_array, double, ary);
}

jcntrl_size_type *
jcntrl_data_subarray_get_writable_size(jcntrl_data_subarray *ary)
{
  return jcntrl_typed_subarray_get_writable(jcntrl_size_array, jcntrl_size_type,
                                            ary);
}
