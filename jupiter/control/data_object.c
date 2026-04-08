#include "data_object.h"
#include "shared_object.h"
#include "shared_object_priv.h"
#include "struct_data.h"
#include "defs.h"

static jcntrl_data_object *
jcntrl_data_object_downcast_impl(jcntrl_shared_object *obj)
{
  return JCNTRL_DOWNCAST_IMPL(jcntrl_data_object, obj);
}

static void *
jcntrl_data_object_downcast_v(jcntrl_shared_object *obj)
{
  return jcntrl_data_object_downcast_impl(obj);
}

static void
jcntrl_data_object_init_func(jcntrl_shared_object_funcs *p)
{
  p->downcast = jcntrl_data_object_downcast_v;
}

JCNTRL_SHARED_METADATA_INIT_DEFINE(jcntrl_data_object,
                                   jcntrl_data_object_init_func)

jcntrl_shared_object *jcntrl_data_object_object(jcntrl_data_object *obj)
{
  return &obj->object;
}

jcntrl_data_object *jcntrl_data_object_downcast(jcntrl_shared_object *obj)
{
  return jcntrl_shared_object_downcast(jcntrl_data_object, obj);
}
