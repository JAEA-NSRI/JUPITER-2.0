#include "real_array.h"
#include "control/defs.h"
#include "control/shared_object.h"
#include "control/shared_object_priv.h"
#include "control/struct_data.h"
#include "control/data_array.h"
#include "control/data_array_data.h"
#include "csvutil.h"
#include "geometry/util.h"

#ifdef JUPITER_DOUBLE
#define jupiter_real_array__ancestor jcntrl_double_array
#define jupiter_real_array__dnmem data.jcntrl_double_array__dnmem
#else
#define jupiter_real_array__ancestor jcntrl_float_array
#define jupiter_real_array__dnmem data.jcntrl_float_array__dnmem
#endif

struct jupiter_real_array
{
  jupiter_real_array__ancestor data;
};
JCNTRL_VTABLE_NONE(jupiter_real_array);

static jupiter_real_array *
jupiter_real_array_downcast_impl(jcntrl_shared_object *obj)
{
  return JCNTRL_DOWNCAST_IMPL(jupiter_real_array, obj);
}

static void *jupiter_real_array_downcast_v(jcntrl_shared_object *obj)
{
  return jupiter_real_array_downcast_impl(obj);
}

static jcntrl_shared_object *jupiter_real_array_allocator(void)
{
  jupiter_real_array *a;
  a = jcntrl_shared_object_default_allocator(jupiter_real_array);
  return a ? jupiter_real_array_object(a) : NULL;
}

static void jupiter_real_array_deleter(jcntrl_shared_object *obj)
{
  jcntrl_shared_object_default_deleter(obj);
}

static int jupiter_real_array_initializer(jcntrl_shared_object *obj)
{
  return 1;
}

static void jupiter_real_array_destructor(jcntrl_shared_object *obj)
{
  /* nop */
}

static jcntrl_size_type
jupiter_real_array_element_size_impl(jcntrl_shared_object *obj)
{
  jcntrl_size_type sz;
  sz = jcntrl_data_array_element_size__super(
    jupiter_real_array_metadata_init()->ancestor);
  CSVASSERT(sz == sizeof(type));
  return sz;
}

JCNTRL_VIRTUAL_WRAP(jupiter_real_array, jcntrl_data_array, element_size)

static void jupiter_real_array_init_func(jcntrl_shared_object_funcs *p)
{
  p->downcast = jupiter_real_array_downcast_v;
  p->initializer = jupiter_real_array_initializer;
  p->destructor = jupiter_real_array_destructor;
  p->allocator = jupiter_real_array_allocator;
  p->deleter = jupiter_real_array_deleter;
  JCNTRL_VIRTUAL_WRAP_SET(p, jupiter_real_array, jcntrl_data_array,
                          element_size);
}

JCNTRL_SHARED_METADATA_INIT_DEFINE(jupiter_real_array,
                                   jupiter_real_array_init_func)

jupiter_real_array *jupiter_real_array_new(void)
{
  return jcntrl_shared_object_new(jupiter_real_array);
}

void jupiter_real_array_delete(jupiter_real_array *ary)
{
  jcntrl_shared_object_delete(jupiter_real_array_object(ary));
}

jcntrl_data_array *jupiter_real_array_data_array(jupiter_real_array *ary)
{
  return &ary->data.data.data;
}

jcntrl_shared_object *jupiter_real_array_object(jupiter_real_array *ary)
{
  return jcntrl_data_array_object(jupiter_real_array_data_array(ary));
}

jupiter_real_array *jupiter_real_array_downcast(jcntrl_shared_object *obj)
{
  return jcntrl_shared_object_downcast(jupiter_real_array, obj);
}

jupiter_real_array *jupiter_real_array_bind(jupiter_real_array *ary,
                                            const type *data,
                                            jcntrl_size_type n)
{
#ifdef JUPITER_DOUBLE
  if (jcntrl_double_array_bind(&ary->data, data, n))
    return ary;
#else
  if (jcntrl_float_array_bind(&ary->data, data, n))
    return ary;
#endif
  return NULL;
}

int jupiter_real_array_is_float(void)
{
  return jcntrl_shared_object_data_is_a(jcntrl_float_array_metadata_init(),
                                        jupiter_real_array_metadata_init());
}

int jupiter_real_array_is_double(void)
{
  return jcntrl_shared_object_data_is_a(jcntrl_double_array_metadata_init(),
                                        jupiter_real_array_metadata_init());
}

jcntrl_float_array *jupiter_real_array_as_float(jupiter_real_array *ary)
{
  return jcntrl_shared_object_downcast(jcntrl_float_array,
                                       jupiter_real_array_object(ary));
}

jcntrl_double_array *jupiter_real_array_as_double(jupiter_real_array *ary)
{
  return jcntrl_shared_object_downcast(jcntrl_double_array,
                                       jupiter_real_array_object(ary));
}
