#include "field_object.h"
#include "data_object.h"
#include "error.h"
#include "shared_object.h"
#include "shared_object_priv.h"
#include "struct_data.h"
#include "defs.h"

static jcntrl_field_object *
jcntrl_field_object_downcast_impl(jcntrl_shared_object *object)
{
  return JCNTRL_DOWNCAST_IMPL(jcntrl_field_object, object);
}

static void *jcntrl_field_object_downcast_v(jcntrl_shared_object *object)
{
  return jcntrl_field_object_downcast_impl(object);
}

static void jcntrl_field_object_init_func(jcntrl_shared_object_funcs *p)
{
  p->downcast = jcntrl_field_object_downcast_v;
}

JCNTRL_SHARED_METADATA_INIT_DEFINE(jcntrl_field_object,
                                   jcntrl_field_object_init_func)

jcntrl_shared_object *jcntrl_field_object_object(jcntrl_field_object *object)
{
  return jcntrl_data_object_object(jcntrl_field_object_data(object));
}

jcntrl_data_object *jcntrl_field_object_data(jcntrl_field_object *object)
{
  JCNTRL_ASSERT(object);
  return &object->data_object;
}

jcntrl_field_object *jcntrl_field_object_downcast(jcntrl_shared_object *object)
{
  return jcntrl_shared_object_downcast(jcntrl_field_object, object);
}
