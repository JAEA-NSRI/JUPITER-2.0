#ifndef JUPITER_CONTROL_ABSTRACT_ARRAY_DATA_H
#define JUPITER_CONTROL_ABSTRACT_ARRAY_DATA_H

#include "defs.h"
#include "shared_object_priv.h"

JUPITER_CONTROL_DECL_BEGIN

struct jcntrl_abstract_array
{
  jcntrl_shared_object object;
  jcntrl_char_array *name;
};
#define jcntrl_abstract_array__ancestor jcntrl_shared_object
#define jcntrl_abstract_array__dnmem object
enum jcntrl_abstract_array_vtable_names
{
  jcntrl_abstract_array_get_ntuple_id =
    JCNTRL_VTABLE_START(jcntrl_abstract_array),
  jcntrl_abstract_array_resize_id,
  JCNTRL_VTABLE_SIZE(jcntrl_abstract_array),
};

// get_ntuple
typedef jcntrl_size_type
jcntrl_abstract_array_get_ntuple_func(jcntrl_shared_object *obj);

struct jcntrl_abstract_array_get_ntuple_args
{
  jcntrl_size_type ntuple;
};

static inline void jcntrl_abstract_array_get_ntuple__wrapper(
  jcntrl_shared_object *obj, void *args,
  jcntrl_abstract_array_get_ntuple_func *func)
{
  struct jcntrl_abstract_array_get_ntuple_args *a;
  a = (struct jcntrl_abstract_array_get_ntuple_args *)args;
  a->ntuple = func(obj);
}

jcntrl_size_type jcntrl_abstract_array_get_ntuple__super(
  const jcntrl_shared_object_data *ancestor);

// resize
typedef int jcntrl_abstract_array_resize_func(jcntrl_shared_object *obj,
                                              jcntrl_size_type ntuple);

struct jcntrl_abstract_array_resize_args
{
  jcntrl_size_type ntuple;
  int ret;
};

static inline void
jcntrl_abstract_array_resize__wrapper(jcntrl_shared_object *obj, void *args,
                                      jcntrl_abstract_array_resize_func *func)
{
  struct jcntrl_abstract_array_resize_args *a;
  a = (struct jcntrl_abstract_array_resize_args *)args;
  a->ret = func(obj, a->ntuple);
}

int jcntrl_abstract_array_resize__super(
  const jcntrl_shared_object_data *ancestor, jcntrl_size_type ntuple);

JUPITER_CONTROL_DECL_END

#endif
