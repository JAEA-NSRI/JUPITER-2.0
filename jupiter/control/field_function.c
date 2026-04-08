#include "field_function.h"
#include "defs.h"
#include "shared_object_priv.h"
#include "field_object.h"
#include "struct_data.h"

static jcntrl_field_function *
jcntrl_field_function_downcast_impl(jcntrl_shared_object *obj)
{
  return JCNTRL_DOWNCAST_IMPL(jcntrl_field_function, obj);
}

static void *jcntrl_field_function_downcast_v(jcntrl_shared_object *obj)
{
  return jcntrl_field_function_downcast_impl(obj);
}

static void jcntrl_field_function_init_func(jcntrl_shared_object_funcs *p)
{
  p->downcast = jcntrl_field_function_downcast_v;
}

JCNTRL_SHARED_METADATA_INIT_DEFINE(jcntrl_field_function,
                                   jcntrl_field_function_init_func)
