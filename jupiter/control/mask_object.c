#include "mask_object.h"
#include "data_object.h"
#include "defs.h"
#include "shared_object.h"
#include "shared_object_priv.h"
#include "struct_data.h"

static jcntrl_mask_object *
jcntrl_mask_object_downcast_impl(jcntrl_shared_object *object)
{
  return JCNTRL_DOWNCAST_IMPL(jcntrl_mask_object, object);
}

static void *jcntrl_mask_object_downcast_v(jcntrl_shared_object *object)
{
  return jcntrl_mask_object_downcast_impl(object);
}

static void jcntrl_mask_object_init_func(jcntrl_shared_object_funcs *f)
{
  f->downcast = jcntrl_mask_object_downcast_v;
}

JCNTRL_SHARED_METADATA_INIT_DEFINE(jcntrl_mask_object,
                                   jcntrl_mask_object_init_func)

jcntrl_shared_object *jcntrl_mask_object_object(jcntrl_mask_object *object)
{
  return jcntrl_data_object_object(jcntrl_mask_object_data(object));
}

jcntrl_data_object *jcntrl_mask_object_data(jcntrl_mask_object *object)
{
  return &object->data_object;
}

jcntrl_mask_object *jcntrl_mask_object_downcast(jcntrl_shared_object *object)
{
  return jcntrl_shared_object_downcast(jcntrl_mask_object, object);
}
