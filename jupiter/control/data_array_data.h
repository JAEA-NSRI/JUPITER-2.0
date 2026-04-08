#ifndef JUPITER_CONTROL_DATA_ARRAY_DATA_H
#define JUPITER_CONTROL_DATA_ARRAY_DATA_H

#include "defs.h"
#include "shared_object_priv.h"
#include "abstract_array_data.h"

#include <stdint.h>

JUPITER_CONTROL_DECL_BEGIN

struct jcntrl_data_array
{
  jcntrl_abstract_array abstract;
};
#define jcntrl_data_array__ancestor jcntrl_abstract_array
#define jcntrl_data_array__dnmem abstract.jcntrl_abstract_array__dnmem
enum jcntrl_data_array_vtable_names
{
  jcntrl_data_array_element_size_id = JCNTRL_VTABLE_START(jcntrl_data_array),
  jcntrl_data_array_element_type_id,
  jcntrl_data_array_get_id,
  jcntrl_data_array_get_writable_id,
  jcntrl_data_array_get_value_id,
  jcntrl_data_array_set_value_id,
  jcntrl_data_array_get_ivalue_id,
  jcntrl_data_array_set_ivalue_id,
  JCNTRL_VTABLE_SIZE(jcntrl_data_array),
};

// element_size
typedef jcntrl_size_type
jcntrl_data_array_element_size_func(jcntrl_shared_object *obj);

struct jcntrl_data_array_element_size_args
{
  jcntrl_size_type sz; /* return value */
};

/**
 * Usage:
 *
 * ```c
 * static jcntrl_size_type
 * your_data_array_element_size_impl(jcntrl_shared_object *obj)
 * {
 *   return ...;
 * }
 *
 * JCNTRL_VIRTUAL_WRAP(your_data_array, jcntrl_data_array, element_size)
 *
 * static void jcntrl_your_data_array_init_func(jcntrl_shared_object_funcs *p)
 * {
 *   // ...
 *   JCNTRL_VIRTUAL_WRAP_SET(p, your_data_array, jcntrl_data_array,
 *                           element_size);
 * }
 * ```
 *
 * @note It is not recommended to override this function to return a different
 *       value to the superclass unless the implementation of superclass returns
 *       0 (i.e., superclass does not require specific element size).
 *
 * @note If you override element_type, this function should return the
 *       element size of returning type by element_type.
 */
static inline void jcntrl_data_array_element_size__wrapper(
  jcntrl_shared_object *obj, void *args,
  jcntrl_data_array_element_size_func *func)
{
  struct jcntrl_data_array_element_size_args *a;
  a = (struct jcntrl_data_array_element_size_args *)args;
  a->sz = func(obj);
}

/**
 * Calls jcntrl_data_array_element_size virtual function for ancestor class.
 */
JUPITER_CONTROL_DECL
jcntrl_size_type jcntrl_data_array_element_size__super(
  const jcntrl_shared_object_data *ancestor);

// element_type
/**
 * Return type of the class that subclasses jcntrl_data_array for the element
 * type is compatible.
 */
typedef const jcntrl_shared_object_data *
jcntrl_data_array_element_type_func(jcntrl_shared_object *obj);

struct jcntrl_data_array_element_type_args
{
  const jcntrl_shared_object_data *ret;
};

static inline void jcntrl_data_array_element_type__wrapper(
  jcntrl_shared_object *obj, void *args,
  jcntrl_data_array_element_type_func *func)
{
  struct jcntrl_data_array_element_type_args *a;
  a = (struct jcntrl_data_array_element_type_args *)args;
  a->ret = func(obj);
}

JUPITER_CONTROL_DECL
const jcntrl_shared_object_data *jcntrl_data_array_element_type__super(
  const jcntrl_shared_object_data *ancestor);

// get
typedef const void *jcntrl_data_array_get_func(jcntrl_shared_object *obj);

struct jcntrl_data_array_get_args
{
  const void *p;
};

static inline void
jcntrl_data_array_get__wrapper(jcntrl_shared_object *obj, void *args,
                               jcntrl_data_array_get_func *func)
{
  struct jcntrl_data_array_get_args *a;
  a = (struct jcntrl_data_array_get_args *)args;
  a->p = func(obj);
}

JUPITER_CONTROL_DECL
const void *
jcntrl_data_array_get__super(const jcntrl_shared_object_data *ancestor);

// get_writable
typedef void *jcntrl_data_array_get_writable_func(jcntrl_shared_object *obj);

struct jcntrl_data_array_get_writable_args
{
  void *p;
};

static inline void jcntrl_data_array_get_writable__wrapper(
  jcntrl_shared_object *obj, void *args,
  jcntrl_data_array_get_writable_func *func)
{
  struct jcntrl_data_array_get_writable_args *a;
  a = (struct jcntrl_data_array_get_writable_args *)args;
  a->p = func(obj);
}

JUPITER_CONTROL_DECL
void *jcntrl_data_array_get_writable__super(
  const jcntrl_shared_object_data *ancestor);

// get_value
typedef double jcntrl_data_array_get_value_func(jcntrl_shared_object *obj,
                                                jcntrl_size_type index);

struct jcntrl_data_array_get_value_args
{
  jcntrl_size_type index;
  double ret;
};

static inline void
jcntrl_data_array_get_value__wrapper(jcntrl_shared_object *obj, void *args,
                                     jcntrl_data_array_get_value_func *func)
{
  struct jcntrl_data_array_get_value_args *a;
  a = (struct jcntrl_data_array_get_value_args *)args;
  a->ret = func(obj, a->index);
}

JUPITER_CONTROL_DECL
double
jcntrl_data_array_get_value__super(const jcntrl_shared_object_data *ancestor,
                                   jcntrl_size_type index);

// set_value
typedef int jcntrl_data_array_set_value_func(jcntrl_shared_object *obj,
                                             jcntrl_size_type index,
                                             double value);

struct jcntrl_data_array_set_value_args
{
  jcntrl_size_type index;
  double value;
  int ret;
};

static inline void
jcntrl_data_array_set_value__wrapper(jcntrl_shared_object *obj, void *args,
                                     jcntrl_data_array_set_value_func *func)
{
  struct jcntrl_data_array_set_value_args *a;
  a = (struct jcntrl_data_array_set_value_args *)args;
  a->ret = func(obj, a->index, a->value);
}

JUPITER_CONTROL_DECL
int jcntrl_data_array_set_value__super(
  const jcntrl_shared_object_data *ancestor, jcntrl_size_type index,
  double value);

// get_ivalue
typedef intmax_t jcntrl_data_array_get_ivalue_func(jcntrl_shared_object *obj,
                                                   jcntrl_size_type index,
                                                   int *err);

struct jcntrl_data_array_get_ivalue_args
{
  jcntrl_size_type index;
  int *err;
  intmax_t ret;
};

static inline void
jcntrl_data_array_get_ivalue__wrapper(jcntrl_shared_object *obj, void *args,
                                      jcntrl_data_array_get_ivalue_func *func)
{
  struct jcntrl_data_array_get_ivalue_args *a;
  a = (struct jcntrl_data_array_get_ivalue_args *)args;
  a->ret = func(obj, a->index, a->err);
}

JUPITER_CONTROL_DECL
intmax_t
jcntrl_data_array_get_ivalue__super(const jcntrl_shared_object_data *ancestor,
                                    jcntrl_size_type index, int *err);

// set_ivalue
typedef int jcntrl_data_array_set_ivalue_func(jcntrl_shared_object *obj,
                                              jcntrl_size_type index,
                                              intmax_t value);

struct jcntrl_data_array_set_ivalue_args
{
  jcntrl_size_type index;
  intmax_t value;
  int ret;
};

static inline void
jcntrl_data_array_set_ivalue__wrapper(jcntrl_shared_object *obj, void *args,
                                      jcntrl_data_array_set_ivalue_func *func)
{
  struct jcntrl_data_array_set_ivalue_args *a;
  a = (struct jcntrl_data_array_set_ivalue_args *)args;
  a->ret = func(obj, a->index, a->value);
}

JUPITER_CONTROL_DECL
int jcntrl_data_array_set_ivalue__super(
  const jcntrl_shared_object_data *ancestor, jcntrl_size_type index,
  intmax_t value);

JUPITER_CONTROL_DECL_END

#endif
