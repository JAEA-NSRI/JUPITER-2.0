#include "mask_data.h"
#include "data_array.h"
#include "error.h"
#include "shared_object.h"
#include "shared_object_priv.h"
#include "struct_data.h"
#include "mask_object.h"
#include "defs.h"

static jcntrl_mask_data *
jcntrl_mask_data_downcast_impl(jcntrl_shared_object *obj)
{
  return JCNTRL_DOWNCAST_IMPL(jcntrl_mask_data, obj);
}

static void *
jcntrl_mask_data_dowoncast_v(jcntrl_shared_object *obj)
{
  return jcntrl_mask_data_downcast_impl(obj);
}

static jcntrl_shared_object *jcntrl_mask_data_allocator(void)
{
  jcntrl_mask_data *p;
  p = jcntrl_shared_object_default_allocator(jcntrl_mask_data);
  return p ? jcntrl_mask_data_object(p) : NULL;
}

static void jcntrl_mask_data_deleter(jcntrl_shared_object *obj)
{
  jcntrl_shared_object_default_deleter(obj);
}

static int jcntrl_mask_data_initializer(jcntrl_shared_object *obj)
{
  jcntrl_mask_data *m;
  m = jcntrl_mask_data_downcast_impl(obj);
  m->array = NULL;
  return 1;
}

static void jcntrl_mask_data_destructor(jcntrl_shared_object *obj)
{
  jcntrl_mask_data *m;
  m = jcntrl_mask_data_downcast_impl(obj);
  if (m->array)
    jcntrl_bool_array_delete(m->array);
  m->array = NULL;
}

static void jcntrl_mask_data_init_func(jcntrl_shared_object_funcs *p)
{
  p->initializer = jcntrl_mask_data_initializer;
  p->destructor = jcntrl_mask_data_destructor;
  p->allocator = jcntrl_mask_data_allocator;
  p->deleter = jcntrl_mask_data_deleter;
  p->downcast = jcntrl_mask_data_dowoncast_v;
}

JCNTRL_SHARED_METADATA_INIT_DEFINE(jcntrl_mask_data, jcntrl_mask_data_init_func)

jcntrl_mask_data *jcntrl_mask_data_new(void)
{
  return jcntrl_shared_object_new(jcntrl_mask_data);
}

void jcntrl_mask_data_delete(jcntrl_mask_data *mask)
{
  jcntrl_shared_object_delete(jcntrl_mask_data_object(mask));
}

jcntrl_shared_object *jcntrl_mask_data_object(jcntrl_mask_data *mask)
{
  JCNTRL_ASSERT(mask);
  return &mask->mask_object.data_object.object;
}

jcntrl_mask_data *jcntrl_mask_data_downcast(jcntrl_shared_object *object)
{
  return jcntrl_shared_object_downcast(jcntrl_mask_data, object);
}

jcntrl_bool_array *jcntrl_mask_data_array(jcntrl_mask_data *mask)
{
  JCNTRL_ASSERT(mask);
  return mask->array;
}

void jcntrl_mask_data_set_array(jcntrl_mask_data *mask,
                                jcntrl_bool_array *array)
{
  JCNTRL_ASSERT(mask);
  if (mask->array)
    jcntrl_bool_array_delete(mask->array);
  mask->array = NULL;
  if (array)
    mask->array = jcntrl_bool_array_take_ownership(array);
}
