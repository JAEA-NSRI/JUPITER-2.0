#include "logical_variable.h"
#include "defs.h"
#include "shared_object_priv.h"
#include "struct_data.h"
#include "data_object.h"

static jcntrl_logical_variable *
jcntrl_logical_variable_downcast_impl(jcntrl_shared_object *obj)
{
  return JCNTRL_DOWNCAST_IMPL(jcntrl_logical_variable, obj);
}

static void *
jcntrl_logical_variable_downcast_v(jcntrl_shared_object *obj)
{
  return jcntrl_logical_variable_downcast_impl(obj);
}

static void jcntrl_logical_variable_init_func(jcntrl_shared_object_funcs *p)
{
  p->downcast = jcntrl_logical_variable_downcast_v;
}

JCNTRL_SHARED_METADATA_INIT_DEFINE(jcntrl_logical_variable,
                                   jcntrl_logical_variable_init_func)
