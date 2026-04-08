#ifndef JUPITER_CONTROL_STATIC_ARRAY_H
#define JUPITER_CONTROL_STATIC_ARRAY_H

#include "data_array.h"
#include "defs.h"
#include "shared_object.h"
#include "shared_object_priv.h"
#include "struct_data.h"

JUPITER_CONTROL_DECL_BEGIN

struct jcntrl_static_array
{
  jcntrl_data_array data;
  jcntrl_size_type ntuple;
};
typedef struct jcntrl_static_array jcntrl_static_array;
#define jcntrl_static_array__ancestor jcntrl_data_array
#define jcntrl_static_array__dnmem data.jcntrl_data_array__dnmem
JCNTRL_VTABLE_NONE(jcntrl_static_array);

#define JCNTRL_STATIC_ARRAY_BASE jcntrl_static_array
#define JCNTRL_STATIC_ARRAY_BASE__dnmem data.jcntrl_static_array__dnmem
#define JCNTRL_STATIC_ARRAY_STRUCT_DEFINE(name, type) \
  struct name                                         \
  {                                                   \
    JCNTRL_STATIC_ARRAY_BASE data;                    \
    type *const ptr;                                  \
  };                                                  \
  typedef struct name name;                           \
  typedef type name##_element_type;                   \
  JCNTRL_VTABLE_NONE(name);

#define jcntrl_static_char_array__ancestor JCNTRL_STATIC_ARRAY_BASE
#define jcntrl_static_char_array__dnmem JCNTRL_STATIC_ARRAY_BASE__dnmem
JCNTRL_STATIC_ARRAY_STRUCT_DEFINE(jcntrl_static_char_array, char)

#define jcntrl_static_cstr_array__ancestor JCNTRL_STATIC_ARRAY_BASE
#define jcntrl_static_cstr_array__dnmem JCNTRL_STATIC_ARRAY_BASE__dnmem
JCNTRL_STATIC_ARRAY_STRUCT_DEFINE(jcntrl_static_cstr_array, const char)

#define jcntrl_static_bool_array__ancestor JCNTRL_STATIC_ARRAY_BASE
#define jcntrl_static_bool_array__dnmem JCNTRL_STATIC_ARRAY_BASE__dnmem
JCNTRL_STATIC_ARRAY_STRUCT_DEFINE(jcntrl_static_bool_array, char)

#define jcntrl_static_int_array__ancestor JCNTRL_STATIC_ARRAY_BASE
#define jcntrl_static_int_array__dnmem JCNTRL_STATIC_ARRAY_BASE__dnmem
JCNTRL_STATIC_ARRAY_STRUCT_DEFINE(jcntrl_static_int_array, int)

#define jcntrl_static_aint_array__ancestor JCNTRL_STATIC_ARRAY_BASE
#define jcntrl_static_aint_array__dnmem JCNTRL_STATIC_ARRAY_BASE__dnmem
JCNTRL_STATIC_ARRAY_STRUCT_DEFINE(jcntrl_static_aint_array, jcntrl_aint_type)

#define jcntrl_static_float_array__ancestor JCNTRL_STATIC_ARRAY_BASE
#define jcntrl_static_float_array__dnmem JCNTRL_STATIC_ARRAY_BASE__dnmem
JCNTRL_STATIC_ARRAY_STRUCT_DEFINE(jcntrl_static_float_array, float)

#define jcntrl_static_double_array__ancestor JCNTRL_STATIC_ARRAY_BASE
#define jcntrl_static_double_array__dnmem JCNTRL_STATIC_ARRAY_BASE__dnmem
JCNTRL_STATIC_ARRAY_STRUCT_DEFINE(jcntrl_static_double_array, double)

#define jcntrl_static_size_array__ancestor JCNTRL_STATIC_ARRAY_BASE
#define jcntrl_static_size_array__dnmem JCNTRL_STATIC_ARRAY_BASE__dnmem
JCNTRL_STATIC_ARRAY_STRUCT_DEFINE(jcntrl_static_size_array, jcntrl_size_type)

JUPITER_CONTROL_DECL
JCNTRL_SHARED_METADATA_INIT_DECL(jcntrl_static_array);
JUPITER_CONTROL_DECL
JCNTRL_SHARED_METADATA_INIT_DECL(jcntrl_static_char_array);
JUPITER_CONTROL_DECL
JCNTRL_SHARED_METADATA_INIT_DECL(jcntrl_static_cstr_array);
JUPITER_CONTROL_DECL
JCNTRL_SHARED_METADATA_INIT_DECL(jcntrl_static_bool_array);
JUPITER_CONTROL_DECL
JCNTRL_SHARED_METADATA_INIT_DECL(jcntrl_static_int_array);
JUPITER_CONTROL_DECL
JCNTRL_SHARED_METADATA_INIT_DECL(jcntrl_static_aint_array);
JUPITER_CONTROL_DECL
JCNTRL_SHARED_METADATA_INIT_DECL(jcntrl_static_float_array);
JUPITER_CONTROL_DECL
JCNTRL_SHARED_METADATA_INIT_DECL(jcntrl_static_double_array);
JUPITER_CONTROL_DECL
JCNTRL_SHARED_METADATA_INIT_DECL(jcntrl_static_size_array);

static inline jcntrl_data_array *
jcntrl_static_array_data(jcntrl_static_array *ary)
{
  return &ary->data;
}

static inline jcntrl_shared_object *
jcntrl_static_array_object(jcntrl_static_array *ary)
{
  return jcntrl_data_array_object(jcntrl_static_array_data(ary));
}

#define JCNTRL_STATIC_TYPED_ARRAY_DATA(name)                \
  static inline jcntrl_data_array *name##_data(name *array) \
  {                                                         \
    return jcntrl_static_array_data(&array->data);          \
  }

#define JCNTRL_STATIC_TYPED_ARRAY_OBJECT(name)                   \
  static inline jcntrl_shared_object *name##_object(name *array) \
  {                                                              \
    return jcntrl_data_array_object(name##_data(array));         \
  }

#define JCNTRL_STATIC_TYPED_ARRAY_DOWNCAST(name)                    \
  static inline name *name##_downcast(jcntrl_shared_object *object) \
  {                                                                 \
    return jcntrl_shared_object_downcast(name, object);             \
  }

#define JCNTRL_STATIC_TYPED_ARRAY_ELEMENT_SIZE(name)       \
  static inline jcntrl_size_type name##_element_size(void) \
  {                                                        \
    return sizeof(name##_element_type);                    \
  }

#define JCNTRL_STATIC_TYPED_ARRAY_GET(name)                  \
  static inline name##_element_type *name##_get(name *array) \
  {                                                          \
    return array->ptr;                                       \
  }

#define JCNTRL_STATIC_TYPED_ARRAY_GET_NTUPLE(name)              \
  static inline jcntrl_size_type name##_get_ntuple(name *array) \
  {                                                             \
    return array->data.ntuple;                                  \
  }

#define JCNTRL_STATIC_TYPED_ARRAY_INIT_BASE(name)                           \
  static inline void name##_init_base(name *ary, name##_element_type *data, \
                                      jcntrl_size_type n)                   \
  {                                                                         \
    jcntrl_shared_object_static_init(name##_object(ary),                    \
                                     JCNTRL_METADATA_INIT(name)());         \
    ary->data.ntuple = n;                                                   \
    *(name##_element_type **)&ary->ptr = data;                              \
  }

#define JCNTRL_STATIC_TYPED_ARRAY_INIT_N(name, ary, n, ...) \
  name##_init_base(ary, (name##_element_type[n]){__VA_ARGS__}, n)

#define JCNTRL_STATIC_TYPED_ARRAY_INIT_B(name, ary, ...)          \
  name##_init_base(ary, (name##_element_type[]){__VA_ARGS__},     \
                   sizeof((name##_element_type[]){__VA_ARGS__}) / \
                     sizeof(name##_element_type))

#define JCNTRL_STATIC_TYPED_ARRAY_INIT_CSTR(name, ary, str) \
  name##_init_base(ary, (name##_element_type[]){"" str}, strlen("" str))

JCNTRL_STATIC_TYPED_ARRAY_DATA(jcntrl_static_char_array)
JCNTRL_STATIC_TYPED_ARRAY_DATA(jcntrl_static_cstr_array)
JCNTRL_STATIC_TYPED_ARRAY_DATA(jcntrl_static_bool_array)
JCNTRL_STATIC_TYPED_ARRAY_DATA(jcntrl_static_int_array)
JCNTRL_STATIC_TYPED_ARRAY_DATA(jcntrl_static_aint_array)
JCNTRL_STATIC_TYPED_ARRAY_DATA(jcntrl_static_float_array)
JCNTRL_STATIC_TYPED_ARRAY_DATA(jcntrl_static_double_array)
JCNTRL_STATIC_TYPED_ARRAY_DATA(jcntrl_static_size_array)

JCNTRL_STATIC_TYPED_ARRAY_OBJECT(jcntrl_static_char_array)
JCNTRL_STATIC_TYPED_ARRAY_OBJECT(jcntrl_static_cstr_array)
JCNTRL_STATIC_TYPED_ARRAY_OBJECT(jcntrl_static_bool_array)
JCNTRL_STATIC_TYPED_ARRAY_OBJECT(jcntrl_static_int_array)
JCNTRL_STATIC_TYPED_ARRAY_OBJECT(jcntrl_static_aint_array)
JCNTRL_STATIC_TYPED_ARRAY_OBJECT(jcntrl_static_float_array)
JCNTRL_STATIC_TYPED_ARRAY_OBJECT(jcntrl_static_double_array)
JCNTRL_STATIC_TYPED_ARRAY_OBJECT(jcntrl_static_size_array)

JCNTRL_STATIC_TYPED_ARRAY_DOWNCAST(jcntrl_static_char_array)
JCNTRL_STATIC_TYPED_ARRAY_DOWNCAST(jcntrl_static_cstr_array)
JCNTRL_STATIC_TYPED_ARRAY_DOWNCAST(jcntrl_static_bool_array)
JCNTRL_STATIC_TYPED_ARRAY_DOWNCAST(jcntrl_static_int_array)
JCNTRL_STATIC_TYPED_ARRAY_DOWNCAST(jcntrl_static_aint_array)
JCNTRL_STATIC_TYPED_ARRAY_DOWNCAST(jcntrl_static_float_array)
JCNTRL_STATIC_TYPED_ARRAY_DOWNCAST(jcntrl_static_double_array)
JCNTRL_STATIC_TYPED_ARRAY_DOWNCAST(jcntrl_static_size_array)

JCNTRL_STATIC_TYPED_ARRAY_ELEMENT_SIZE(jcntrl_static_char_array)
JCNTRL_STATIC_TYPED_ARRAY_ELEMENT_SIZE(jcntrl_static_cstr_array)
JCNTRL_STATIC_TYPED_ARRAY_ELEMENT_SIZE(jcntrl_static_bool_array)
JCNTRL_STATIC_TYPED_ARRAY_ELEMENT_SIZE(jcntrl_static_int_array)
JCNTRL_STATIC_TYPED_ARRAY_ELEMENT_SIZE(jcntrl_static_aint_array)
JCNTRL_STATIC_TYPED_ARRAY_ELEMENT_SIZE(jcntrl_static_float_array)
JCNTRL_STATIC_TYPED_ARRAY_ELEMENT_SIZE(jcntrl_static_double_array)
JCNTRL_STATIC_TYPED_ARRAY_ELEMENT_SIZE(jcntrl_static_size_array)

JCNTRL_STATIC_TYPED_ARRAY_GET(jcntrl_static_char_array)
JCNTRL_STATIC_TYPED_ARRAY_GET(jcntrl_static_cstr_array)
JCNTRL_STATIC_TYPED_ARRAY_GET(jcntrl_static_bool_array)
JCNTRL_STATIC_TYPED_ARRAY_GET(jcntrl_static_int_array)
JCNTRL_STATIC_TYPED_ARRAY_GET(jcntrl_static_aint_array)
JCNTRL_STATIC_TYPED_ARRAY_GET(jcntrl_static_float_array)
JCNTRL_STATIC_TYPED_ARRAY_GET(jcntrl_static_double_array)
JCNTRL_STATIC_TYPED_ARRAY_GET(jcntrl_static_size_array)

JCNTRL_STATIC_TYPED_ARRAY_GET_NTUPLE(jcntrl_static_char_array)
JCNTRL_STATIC_TYPED_ARRAY_GET_NTUPLE(jcntrl_static_cstr_array)
JCNTRL_STATIC_TYPED_ARRAY_GET_NTUPLE(jcntrl_static_bool_array)
JCNTRL_STATIC_TYPED_ARRAY_GET_NTUPLE(jcntrl_static_int_array)
JCNTRL_STATIC_TYPED_ARRAY_GET_NTUPLE(jcntrl_static_aint_array)
JCNTRL_STATIC_TYPED_ARRAY_GET_NTUPLE(jcntrl_static_float_array)
JCNTRL_STATIC_TYPED_ARRAY_GET_NTUPLE(jcntrl_static_double_array)
JCNTRL_STATIC_TYPED_ARRAY_GET_NTUPLE(jcntrl_static_size_array)

JCNTRL_STATIC_TYPED_ARRAY_INIT_BASE(jcntrl_static_char_array)
JCNTRL_STATIC_TYPED_ARRAY_INIT_BASE(jcntrl_static_cstr_array)
JCNTRL_STATIC_TYPED_ARRAY_INIT_BASE(jcntrl_static_bool_array)
JCNTRL_STATIC_TYPED_ARRAY_INIT_BASE(jcntrl_static_int_array)
JCNTRL_STATIC_TYPED_ARRAY_INIT_BASE(jcntrl_static_aint_array)
JCNTRL_STATIC_TYPED_ARRAY_INIT_BASE(jcntrl_static_float_array)
JCNTRL_STATIC_TYPED_ARRAY_INIT_BASE(jcntrl_static_double_array)
JCNTRL_STATIC_TYPED_ARRAY_INIT_BASE(jcntrl_static_size_array)

#define jcntrl_static_char_array_init_n(ary, n, ...)                 \
  JCNTRL_STATIC_TYPED_ARRAY_INIT_N(jcntrl_static_char_array, ary, n, \
                                   __VA_ARGS__)

#define jcntrl_static_cstr_array_init_n(ary, n, ...)                 \
  JCNTRL_STATIC_TYPED_ARRAY_INIT_N(jcntrl_static_cstr_array, ary, n, \
                                   __VA_ARGS__)

#define jcntrl_static_bool_array_init_n(ary, n, ...)                 \
  JCNTRL_STATIC_TYPED_ARRAY_INIT_N(jcntrl_static_bool_array, ary, n, \
                                   __VA_ARGS__)

#define jcntrl_static_int_array_init_n(ary, n, ...) \
  JCNTRL_STATIC_TYPED_ARRAY_INIT_N(jcntrl_static_int_array, ary, n, __VA_ARGS__)

#define jcntrl_static_aint_array_init_n(ary, n, ...)                 \
  JCNTRL_STATIC_TYPED_ARRAY_INIT_N(jcntrl_static_aint_array, ary, n, \
                                   __VA_ARGS__)

#define jcntrl_static_float_array_init_n(ary, n, ...)                 \
  JCNTRL_STATIC_TYPED_ARRAY_INIT_N(jcntrl_static_float_array, ary, n, \
                                   __VA_ARGS__)

#define jcntrl_static_double_array_init_n(ary, n, ...)                 \
  JCNTRL_STATIC_TYPED_ARRAY_INIT_N(jcntrl_static_double_array, ary, n, \
                                   __VA_ARGS__)

#define jcntrl_static_size_array_init_n(ary, n, ...)                 \
  JCNTRL_STATIC_TYPED_ARRAY_INIT_N(jcntrl_static_size_array, ary, n, \
                                   __VA_ARGS__)

#define jcntrl_static_char_array_init_b(ary, ...) \
  JCNTRL_STATIC_TYPED_ARRAY_INIT_B(jcntrl_static_char_array, ary, __VA_ARGS__)

#define jcntrl_static_cstr_array_init_b(ary, ...) \
  JCNTRL_STATIC_TYPED_ARRAY_INIT_B(jcntrl_static_cstr_array, ary, __VA_ARGS__)

#define jcntrl_static_bool_array_init_b(ary, ...) \
  JCNTRL_STATIC_TYPED_ARRAY_INIT_B(jcntrl_static_bool_array, ary, __VA_ARGS__)

#define jcntrl_static_int_array_init_b(ary, ...) \
  JCNTRL_STATIC_TYPED_ARRAY_INIT_B(jcntrl_static_int_array, ary, __VA_ARGS__)

#define jcntrl_static_aint_array_init_b(ary, ...) \
  JCNTRL_STATIC_TYPED_ARRAY_INIT_B(jcntrl_static_aint_array, ary, __VA_ARGS__)

#define jcntrl_static_float_array_init_b(ary, ...) \
  JCNTRL_STATIC_TYPED_ARRAY_INIT_B(jcntrl_static_float_array, ary, __VA_ARGS__)

#define jcntrl_static_double_array_init_b(ary, ...) \
  JCNTRL_STATIC_TYPED_ARRAY_INIT_B(jcntrl_static_double_array, ary, __VA_ARGS__)

#define jcntrl_static_size_array_init_b(ary, ...) \
  JCNTRL_STATIC_TYPED_ARRAY_INIT_B(jcntrl_static_size_array, ary, __VA_ARGS__)

#define jcntrl_static_char_array_init_cstr(ary, str) \
  JCNTRL_STATIC_TYPED_ARRAY_INIT_CSTR(jcntrl_static_char_array, ary, str)

#define jcntrl_static_cstr_array_init_cstr(ary, str) \
  JCNTRL_STATIC_TYPED_ARRAY_INIT_CSTR(jcntrl_static_cstr_array, ary, str)

JUPITER_CONTROL_DECL_END

#endif
