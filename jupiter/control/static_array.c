#include "static_array.h"
#include "defs.h"
#include "error.h"
#include "data_array.h"
#include "shared_object_priv.h"
#include <limits.h>
#include <stdint.h>

static jcntrl_static_array *
jcntrl_static_array_downcast_impl(jcntrl_shared_object *obj)
{
  return JCNTRL_DOWNCAST_IMPL(jcntrl_static_array, obj);
}

static void *jcntrl_static_array_downcast_v(jcntrl_shared_object *obj)
{
  return jcntrl_static_array_downcast_impl(obj);
}

static int jcntrl_static_array_initializer(jcntrl_shared_object *obj)
{
  jcntrl_static_array *p;
  p = jcntrl_static_array_downcast_impl(obj);
  p->ntuple = 0;
  return 1;
}

static void jcntrl_static_array_destructor(jcntrl_shared_object *obj)
{
  /* nop */
}

static jcntrl_size_type
jcntrl_static_array_get_ntuple_impl(jcntrl_shared_object *obj)
{
  jcntrl_static_array *p;
  p = jcntrl_static_array_downcast_impl(obj);
  return p->ntuple;
}

JCNTRL_VIRTUAL_WRAP(jcntrl_static_array, jcntrl_abstract_array, get_ntuple)

static void jcntrl_static_array_init_func(jcntrl_shared_object_funcs *p)
{
  p->downcast = jcntrl_static_array_downcast_v;
  p->initializer = jcntrl_static_array_initializer;
  p->destructor = jcntrl_static_array_destructor;
  p->allocator = NULL;
  p->deleter = NULL;
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_static_array, jcntrl_abstract_array,
                          get_ntuple);
}

JCNTRL_SHARED_METADATA_INIT_DEFINE(jcntrl_static_array,
                                   jcntrl_static_array_init_func)

static int jcntrl_static_typed_array_index_chk(jcntrl_static_array *array,
                                               jcntrl_size_type index,
                                               const char *file, int line)
{
  if (index < 0 || index >= array->ntuple) {
    jcntrl_raise_index_error(file, line, index);
    return 0;
  }
  return 1;
}

#define jcntrl_static_typed_array_index_chk(ary, index) \
  jcntrl_static_typed_array_index_chk(ary, index, __FILE__, __LINE__)

#define JCNTRL_STATIC_TYPED_ARRAY_DOWNCAST_IMPL(name)                  \
  static name *name##_downcast_impl(jcntrl_shared_object *object)      \
  {                                                                    \
    return JCNTRL_DOWNCAST_IMPL(name, object);                         \
  }                                                                    \
                                                                       \
  static void *name##_downcast_v(jcntrl_shared_object *object)         \
  {                                                                    \
    return name##_downcast_impl(object);                               \
  }

#define JCNTRL_STATIC_TYPED_ARRAY_INITIALIZER_IMPL(name)      \
  static int name##_initializer(jcntrl_shared_object *object) \
  {                                                           \
    name *ary = name##_downcast_impl(object);                 \
    *(name##_element_type **)&ary->ptr = NULL;                \
    return 1;                                                 \
  }

#define JCNTRL_STATIC_TYPED_ARRAY_DESTRUCTOR_IMPL(name) \
  static void name##_destructor(jcntrl_shared_object *object) { /* nop */ }

#define JCNTRL_STATIC_TYPED_ARRAY_ELEMENT_SIZE_IMPL(name) \
  static jcntrl_size_type name##_element_size_impl(       \
    jcntrl_shared_object *object)                         \
  {                                                       \
    return name##_element_size();                         \
  }                                                       \
                                                          \
  JCNTRL_VIRTUAL_WRAP(name, jcntrl_data_array, element_size)

#define JCNTRL_STATIC_TYPED_ARRAY_ELEMENT_TYPE_IMPL(name, base)     \
  static const jcntrl_shared_object_data *name##_element_type_impl( \
    jcntrl_shared_object *object)                                   \
  {                                                                 \
    return JCNTRL_METADATA_INIT(base)();                            \
  }                                                                 \
                                                                    \
  JCNTRL_VIRTUAL_WRAP(name, jcntrl_data_array, element_type)

#define JCNTRL_STATIC_TYPED_ARRAY_GET_IMPL(name)                   \
  static const void *name##_get_impl(jcntrl_shared_object *object) \
  {                                                                \
    name *ary = name##_downcast_impl(object);                      \
    return ary->ptr;                                               \
  }                                                                \
                                                                   \
  JCNTRL_VIRTUAL_WRAP(name, jcntrl_data_array, get)

#define JCNTRL_STATIC_TYPED_ARRAY_GET_WRITABLE_IMPL(name)             \
  static void *name##_get_writable_impl(jcntrl_shared_object *object) \
  {                                                                   \
    name *ary = name##_downcast_impl(object);                         \
    return ary->ptr;                                                  \
  }                                                                   \
                                                                      \
  JCNTRL_VIRTUAL_WRAP(name, jcntrl_data_array, get_writable)

#define JCNTRL_STATIC_TYPED_ARRAY_GET_VALUE_IMPL(name)              \
  static double name##_get_value_impl(jcntrl_shared_object *object, \
                                      jcntrl_size_type index)       \
  {                                                                 \
    name *ary = name##_downcast_impl(object);                       \
    if (jcntrl_static_typed_array_index_chk(&ary->data, index))     \
      return ary->ptr[index];                                       \
    return 0.0;                                                     \
  }                                                                 \
                                                                    \
  JCNTRL_VIRTUAL_WRAP(name, jcntrl_data_array, get_value)

#define JCNTRL_STATIC_TYPED_ARRAY_GET_IVALUE_IMPL(name)                    \
  static intmax_t name##_get_ivalue_impl(jcntrl_shared_object *object,     \
                                         jcntrl_size_type index, int *err) \
  {                                                                        \
    name *ary = name##_downcast_impl(object);                              \
    if (jcntrl_static_typed_array_index_chk(&ary->data, index))            \
      return name##_get_ivalue_conv(&ary->ptr[index], err);                \
    return 0;                                                              \
  }                                                                        \
                                                                           \
  JCNTRL_VIRTUAL_WRAP(name, jcntrl_data_array, get_ivalue)

#define JCNTRL_STATIC_TYPED_ARRAY_SET_VALUE_IMPL(name)                   \
  static int name##_set_value_impl(jcntrl_shared_object *object,         \
                                   jcntrl_size_type index, double value) \
  {                                                                      \
    name *ary = name##_downcast_impl(object);                            \
    if (jcntrl_static_typed_array_index_chk(&ary->data, index)) {        \
      ary->ptr[index] = value;                                           \
      return 1;                                                          \
    }                                                                    \
    return 0;                                                            \
  }                                                                      \
                                                                         \
  JCNTRL_VIRTUAL_WRAP(name, jcntrl_data_array, set_value)

#define JCNTRL_STATIC_TYPED_ARRAY_SET_IVALUE_IMPL(name)                     \
  static int name##_set_ivalue_impl(jcntrl_shared_object *object,           \
                                    jcntrl_size_type index, intmax_t value) \
  {                                                                         \
    name *ary = name##_downcast_impl(object);                               \
    if (jcntrl_static_typed_array_index_chk(&ary->data, index))             \
      return name##_set_ivalue_conv(&ary->ptr[index], value);               \
    return 0;                                                               \
  }                                                                         \
                                                                            \
  JCNTRL_VIRTUAL_WRAP(name, jcntrl_data_array, set_ivalue)

#define JCNTRL_STATIC_TYPED_ARRAY_INIT_FUNC(name, vtbase)     \
  static void name##_init_func(jcntrl_shared_object_funcs *p) \
  {                                                           \
    p->downcast = name##_downcast_v;                          \
    p->initializer = name##_initializer;                      \
    p->destructor = name##_destructor;                        \
    JCNTRL_STATIC_TYPED_##vtbase##_VIRTUAL_SET(p, name);      \
  }

#define JCNTRL_STATIC_TYPED_RD_VIRTUAL_SET(p, name)                  \
  JCNTRL_VIRTUAL_WRAP_SET(p, name, jcntrl_data_array, element_size); \
  JCNTRL_VIRTUAL_WRAP_SET(p, name, jcntrl_data_array, element_type); \
  JCNTRL_VIRTUAL_WRAP_SET(p, name, jcntrl_data_array, get);          \
  JCNTRL_VIRTUAL_WRAP_SET(p, name, jcntrl_data_array, get_value);    \
  JCNTRL_VIRTUAL_WRAP_SET(p, name, jcntrl_data_array, get_ivalue)

#define JCNTRL_STATIC_TYPED_RD_ARRAY_IMPL(name, base)     \
  JCNTRL_STATIC_TYPED_ARRAY_DOWNCAST_IMPL(name)           \
  JCNTRL_STATIC_TYPED_ARRAY_INITIALIZER_IMPL(name)        \
  JCNTRL_STATIC_TYPED_ARRAY_DESTRUCTOR_IMPL(name)         \
  JCNTRL_STATIC_TYPED_ARRAY_ELEMENT_SIZE_IMPL(name)       \
  JCNTRL_STATIC_TYPED_ARRAY_ELEMENT_TYPE_IMPL(name, base) \
  JCNTRL_STATIC_TYPED_ARRAY_GET_IMPL(name)                \
  JCNTRL_STATIC_TYPED_ARRAY_GET_VALUE_IMPL(name)          \
  JCNTRL_STATIC_TYPED_ARRAY_GET_IVALUE_IMPL(name)

#define JCNTRL_STATIC_TYPED_RDWR_VIRTUAL_SET(p, name)                \
  JCNTRL_STATIC_TYPED_RD_VIRTUAL_SET(p, name);                       \
  JCNTRL_VIRTUAL_WRAP_SET(p, name, jcntrl_data_array, get_writable); \
  JCNTRL_VIRTUAL_WRAP_SET(p, name, jcntrl_data_array, set_value);    \
  JCNTRL_VIRTUAL_WRAP_SET(p, name, jcntrl_data_array, set_ivalue)

#define JCNTRL_STATIC_TYPED_RDWR_ARRAY_IMPL(name, base) \
  JCNTRL_STATIC_TYPED_RD_ARRAY_IMPL(name, base)         \
  JCNTRL_STATIC_TYPED_ARRAY_GET_WRITABLE_IMPL(name)     \
  JCNTRL_STATIC_TYPED_ARRAY_SET_VALUE_IMPL(name)        \
  JCNTRL_STATIC_TYPED_ARRAY_SET_IVALUE_IMPL(name)

#define JCNTRL_STATIC_TYPED_RD_ARRAY(name, base) \
  JCNTRL_STATIC_TYPED_RD_ARRAY_IMPL(name, base)  \
  JCNTRL_STATIC_TYPED_ARRAY_INIT_FUNC(name, RD)  \
  JCNTRL_SHARED_METADATA_INIT_DEFINE(name, name##_init_func)

#define JCNTRL_STATIC_TYPED_RDWR_ARRAY(name, base) \
  JCNTRL_STATIC_TYPED_RDWR_ARRAY_IMPL(name, base)  \
  JCNTRL_STATIC_TYPED_ARRAY_INIT_FUNC(name, RDWR)  \
  JCNTRL_SHARED_METADATA_INIT_DEFINE(name, name##_init_func)

static intmax_t jcntrl_static_char_array_get_ivalue_conv(const char *p,
                                                         int *err)
{
  if (err)
    *err = 0;
  return *p;
}

static intmax_t jcntrl_static_cstr_array_get_ivalue_conv(const char *p,
                                                         int *err)
{
  return jcntrl_static_char_array_get_ivalue_conv(p, err);
}

static intmax_t jcntrl_static_bool_array_get_ivalue_conv(const char *p,
                                                         int *err)
{
  if (err)
    *err = 0;
  return *p;
}

static intmax_t jcntrl_static_int_array_get_ivalue_conv(const int *p, int *err)
{
  if (err)
    *err = 0;
  return *p;
}

static intmax_t
jcntrl_static_aint_array_get_ivalue_conv(const jcntrl_aint_type *p, int *err)
{
  if (err)
    *err = 0;
  return *p;
}

static intmax_t jcntrl_static_float_array_get_ivalue_conv(const float *p,
                                                          int *err)
{
  jcntrl_raise_argument_error(__FILE__, __LINE__,
                              "Cannot get value as integral type");
  if (err)
    *err = 1;
  return 0;
}

static intmax_t jcntrl_static_double_array_get_ivalue_conv(const double *p,
                                                           int *err)
{
  jcntrl_raise_argument_error(__FILE__, __LINE__,
                              "Cannot get value as integral type");
  if (err)
    *err = 1;
  return 0;
}

static int jcntrl_static_array_ovf_chk(intmax_t value, intmax_t min,
                                       intmax_t max)
{
  if (value < min || value > max) {
    jcntrl_raise_overflow_error(__FILE__, __LINE__);
    return 0;
  }
  return 1;
}

static intmax_t
jcntrl_static_size_array_get_ivalue_conv(const jcntrl_size_type *p, int *err)
{
  if (err)
    *err = 0;
  return *p;
}

static int jcntrl_static_char_array_set_ivalue_conv(char *p, intmax_t value)
{
  if (!jcntrl_static_array_ovf_chk(value, CHAR_MIN, CHAR_MAX))
    return 0;

  *p = value;
  return 1;
}

static int jcntrl_static_bool_array_set_ivalue_conv(char *p, intmax_t value)
{
  if (!jcntrl_static_array_ovf_chk(value, CHAR_MIN, CHAR_MAX))
    return 0;

  *p = value;
  return 1;
}

static int jcntrl_static_int_array_set_ivalue_conv(int *p, intmax_t value)
{
  if (!jcntrl_static_array_ovf_chk(value, INT_MIN, INT_MAX))
    return 0;

  *p = value;
  return 1;
}

static int jcntrl_static_aint_array_set_ivalue_conv(jcntrl_aint_type *p,
                                                    intmax_t value)
{
  /* cannot check overflow: see jcntrl_aint_array_set_ivalue_conv() */
  *p = value;
  return 1;
}

static int jcntrl_static_float_array_set_ivalue_conv(float *p, intmax_t value)
{
  jcntrl_raise_argument_error(__FILE__, __LINE__,
                              "Cannot be assigned by integral value");
  return 0;
}

static int jcntrl_static_double_array_set_ivalue_conv(double *p, intmax_t value)
{
  jcntrl_raise_argument_error(__FILE__, __LINE__,
                              "Cannot be assigned by integral value");
  return 0;
}

static int jcntrl_static_size_array_set_ivalue_conv(jcntrl_size_type *p,
                                                    intmax_t value)
{
  if (!jcntrl_static_array_ovf_chk(value, JCNTRL_SIZE_MIN, JCNTRL_SIZE_MAX))
    return 0;

  *p = value;
  return 1;
}

JCNTRL_STATIC_TYPED_RDWR_ARRAY(jcntrl_static_char_array, jcntrl_char_array)
JCNTRL_STATIC_TYPED_RD_ARRAY(jcntrl_static_cstr_array, jcntrl_char_array)
JCNTRL_STATIC_TYPED_RDWR_ARRAY(jcntrl_static_bool_array, jcntrl_bool_array)
JCNTRL_STATIC_TYPED_RDWR_ARRAY(jcntrl_static_int_array, jcntrl_int_array)
JCNTRL_STATIC_TYPED_RDWR_ARRAY(jcntrl_static_aint_array, jcntrl_aint_array)
JCNTRL_STATIC_TYPED_RDWR_ARRAY(jcntrl_static_float_array, jcntrl_float_array)
JCNTRL_STATIC_TYPED_RDWR_ARRAY(jcntrl_static_double_array, jcntrl_double_array)
JCNTRL_STATIC_TYPED_RDWR_ARRAY(jcntrl_static_size_array, jcntrl_size_array)
