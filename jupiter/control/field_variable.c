
#include "field_variable.h"
#include "defs.h"
#include "shared_object_priv.h"
#include "struct_data.h"
#include "data_object.h"
#include "error.h"
#include "shared_object.h"
#include "field_object.h"
#include "data_array.h"

#include <math.h>
#include <stdlib.h>

static jcntrl_field_variable *
jcntrl_field_variable_downcast_impl(jcntrl_shared_object *obj)
{
  return JCNTRL_DOWNCAST_IMPL(jcntrl_field_variable, obj);
}

static void *
jcntrl_field_variable_downcast_v(jcntrl_shared_object *obj)
{
  return jcntrl_field_variable_downcast_impl(obj);
}

static jcntrl_shared_object *
jcntrl_field_variable_allocator(void)
{
  jcntrl_field_variable *p;
  p = jcntrl_shared_object_default_allocator(jcntrl_field_variable);
  return p ? jcntrl_field_variable_object(p) : NULL;
}

static void jcntrl_field_variable_deleter(jcntrl_shared_object *obj)
{
  jcntrl_shared_object_default_deleter(obj);
}

static int jcntrl_field_variable_initializer(jcntrl_shared_object *obj)
{
  jcntrl_double_array *a;
  jcntrl_field_variable *fv;
  fv = jcntrl_field_variable_downcast_impl(obj);

  a = jcntrl_double_array_new();
  if (!a)
    return 0;

  fv->data = jcntrl_double_array_data(a);
  return 1;
}

static void jcntrl_field_variable_desctructor(jcntrl_shared_object *obj)
{
  jcntrl_field_variable *fv;
  fv = jcntrl_field_variable_downcast_impl(obj);
  if (fv->data)
    jcntrl_data_array_delete(fv->data);
  fv->data = NULL;
}

static void jcntrl_field_variable_init_func(jcntrl_shared_object_funcs *p)
{
  p->allocator = jcntrl_field_variable_allocator;
  p->deleter = jcntrl_field_variable_deleter;
  p->initializer = jcntrl_field_variable_initializer;
  p->destructor = jcntrl_field_variable_desctructor;
  p->downcast = jcntrl_field_variable_downcast_v;
}

JCNTRL_SHARED_METADATA_INIT_DEFINE(jcntrl_field_variable,
                                   jcntrl_field_variable_init_func)

jcntrl_field_variable *jcntrl_field_variable_new(void)
{
  return jcntrl_shared_object_new(jcntrl_field_variable);
}

void jcntrl_field_variable_delete(jcntrl_field_variable *fvar)
{
  JCNTRL_ASSERT(fvar);

  jcntrl_shared_object_delete(jcntrl_field_variable_object(fvar));
}

jcntrl_shared_object *
jcntrl_field_variable_object(jcntrl_field_variable *fvar)
{
  JCNTRL_ASSERT(fvar);

  return jcntrl_field_object_object(&fvar->field_object);
}

jcntrl_field_variable *
jcntrl_field_variable_downcast(jcntrl_shared_object *object)
{
  return jcntrl_shared_object_downcast(jcntrl_field_variable, object);
}

double jcntrl_field_variable_value(jcntrl_field_variable *fvar, int *error)
{
  jcntrl_size_type ntuple;

  JCNTRL_ASSERT(fvar);
  JCNTRL_ASSERT(fvar->data);

  ntuple = jcntrl_data_array_get_ntuple(fvar->data);
  if (ntuple != 1) {
    if (error) {
      *error = 1;
    }
    jcntrl_raise_argument_error(__FILE__, __LINE__,
                                "Number of elements in field variable is not "
                                "1. This error is caused by trying to obtain "
                                "a field variable that has multiple elements.");
    return HUGE_VAL;
  }

  return jcntrl_data_array_get_value(fvar->data, 0);
}

int jcntrl_field_variable_set_value(jcntrl_field_variable *fvar, double value)
{
  jcntrl_size_type ntuple;

  JCNTRL_ASSERT(fvar);
  JCNTRL_ASSERT(fvar->data);

  ntuple = jcntrl_data_array_get_ntuple(fvar->data);
  if (ntuple > 1) {
    jcntrl_raise_argument_error(__FILE__, __LINE__,
                                "Number of elements in field variable is not "
                                "1. This error is caused by trying to set a "
                                "field variable that has multiple elements.");
    return 0;
  }

  if (ntuple != 1) {
    if (!jcntrl_data_array_resize(fvar->data, 1))
      return 0;
  }
  return jcntrl_data_array_set_value(fvar->data, 0, value);
}

jcntrl_data_array *jcntrl_field_variable_array(jcntrl_field_variable *fvar)
{
  JCNTRL_ASSERT(fvar);
  JCNTRL_ASSERT(fvar->data);

  return fvar->data;
}

int jcntrl_field_variable_set_array(jcntrl_field_variable *fvar,
                                    jcntrl_data_array *array)
{
  jcntrl_data_array *nary;

  JCNTRL_ASSERT(fvar);
  JCNTRL_ASSERT(array);

  nary = jcntrl_data_array_take_ownership(array);
  if (nary) {
    if (fvar->data) {
      jcntrl_data_array_delete(fvar->data);
    }

    fvar->data = nary;
    return 1;
  }
  return 0;
}
