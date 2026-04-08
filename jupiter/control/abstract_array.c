#include "abstract_array.h"
#include "abstract_array_data.h"
#include "data_array.h"
#include "defs.h"
#include "error.h"
#include "shared_object.h"
#include "shared_object_priv.h"
#include "static_array.h"

static jcntrl_abstract_array *
jcntrl_abstract_array_downcast_impl(jcntrl_shared_object *obj)
{
  return JCNTRL_DOWNCAST_IMPL(jcntrl_abstract_array, obj);
}

static void *jcntrl_abstract_array_downcast_v(jcntrl_shared_object *obj)
{
  return jcntrl_abstract_array_downcast_impl(obj);
}

static int jcntrl_abstract_array_initializer(jcntrl_shared_object *obj)
{
  jcntrl_abstract_array *ary;
  ary = jcntrl_abstract_array_downcast_impl(obj);
  ary->name = NULL;
  return 1;
}

static void jcntrl_abstract_array_destructor(jcntrl_shared_object *obj)
{
  jcntrl_abstract_array *ary;
  ary = jcntrl_abstract_array_downcast_impl(obj);

  if (ary->name)
    jcntrl_char_array_delete(ary->name);
}

static jcntrl_size_type
jcntrl_abstract_array_get_ntuple_impl(jcntrl_shared_object *obj)
{
  return 0;
}

JCNTRL_VIRTUAL_WRAP(jcntrl_abstract_array, jcntrl_abstract_array, get_ntuple)

jcntrl_size_type jcntrl_abstract_array_get_ntuple(jcntrl_abstract_array *ary)
{
  struct jcntrl_abstract_array_get_ntuple_args args = {0};
  jcntrl_shared_object_call_virtual(jcntrl_abstract_array_object(ary),
                                    jcntrl_abstract_array, get_ntuple, &args);
  return args.ntuple;
}

jcntrl_size_type jcntrl_abstract_array_get_ntuple__super(
  const jcntrl_shared_object_data *ancestor)
{
  struct jcntrl_abstract_array_get_ntuple_args args = {0};
  jcntrl_shared_object_call_super(ancestor, jcntrl_abstract_array, get_ntuple,
                                  &args);
  return args.ntuple;
}

static int jcntrl_abstract_array_resize_impl(jcntrl_shared_object *obj,
                                             jcntrl_size_type ntuple)
{
  jcntrl_raise_argument_error(__FILE__, __LINE__,
                              "Array resize for this array is not supported");
  return 0;
}

JCNTRL_VIRTUAL_WRAP(jcntrl_abstract_array, jcntrl_abstract_array, resize)

int jcntrl_abstract_array_resize(jcntrl_abstract_array *ary,
                                 jcntrl_size_type ntuple)
{
  struct jcntrl_abstract_array_resize_args args = {
    .ntuple = ntuple,
    .ret = 0,
  };
  jcntrl_shared_object_call_virtual(jcntrl_abstract_array_object(ary),
                                    jcntrl_abstract_array, resize, &args);
  return args.ret;
}

int jcntrl_abstract_array_resize__super(
  const jcntrl_shared_object_data *ancestor, jcntrl_size_type ntuple)
{
  struct jcntrl_abstract_array_resize_args args = {
    .ntuple = ntuple,
    .ret = 0,
  };
  jcntrl_shared_object_call_super(ancestor, jcntrl_abstract_array, resize,
                                  &args);
  return args.ret;
}

static void jcntrl_abstract_array_init_func(jcntrl_shared_object_funcs *p)
{
  p->downcast = jcntrl_abstract_array_downcast_v;
  p->initializer = jcntrl_abstract_array_initializer;
  p->destructor = jcntrl_abstract_array_destructor;
  p->allocator = NULL;
  p->deleter = NULL;
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_abstract_array, jcntrl_abstract_array,
                          get_ntuple);
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_abstract_array, jcntrl_abstract_array,
                          resize);
}

JCNTRL_SHARED_METADATA_INIT_DEFINE(jcntrl_abstract_array,
                                   jcntrl_abstract_array_init_func)

jcntrl_shared_object *jcntrl_abstract_array_object(jcntrl_abstract_array *p)
{
  return &p->object;
}

jcntrl_abstract_array *jcntrl_abstract_array_downcast(jcntrl_shared_object *o)
{
  return jcntrl_shared_object_downcast(jcntrl_abstract_array, o);
}

const char *jcntrl_abstract_array_name(jcntrl_abstract_array *ary,
                                   jcntrl_size_type *len)
{
  JCNTRL_ASSERT(ary);
  return jcntrl_get_string_c(ary->name, len);
}

jcntrl_char_array *jcntrl_abstract_array_name_d(jcntrl_abstract_array *ary)
{
  return ary->name;
}

int jcntrl_abstract_array_set_name(jcntrl_abstract_array *ary, const char *name,
                                   jcntrl_size_type len)
{
  JCNTRL_ASSERT(ary);
  return jcntrl_set_string_c(&ary->name, name, len);
}

int jcntrl_abstract_array_set_name_d(jcntrl_abstract_array *ary,
                                     jcntrl_data_array *name)
{
  JCNTRL_ASSERT(ary);
  return jcntrl_set_string(&ary->name, name);
}
