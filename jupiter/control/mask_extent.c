#include "mask_extent.h"
#include "defs.h"
#include "error.h"
#include "executive.h"
#include "global.h"
#include "information.h"
#include "input.h"
#include "mask_function.h"
#include "output.h"
#include "shared_object.h"
#include "shared_object_priv.h"
#include "executive_data.h"
#include "struct_data.h"
#include "mask_function_priv.h"

#include <string.h>

static void init_extent_empty(int extent[6])
{
  extent[0] = 0;
  extent[1] = -1;
  extent[2] = 0;
  extent[3] = -1;
  extent[4] = 0;
  extent[5] = -1;
}

struct jcntrl_mask_extent_function
{
  jcntrl_mask_function func;
  int extent[6];
};
#define jcntrl_mask_extent_function__ancestor jcntrl_mask_function
#define jcntrl_mask_extent_function__dnmem func.jcntrl_mask_function__dnmem
JCNTRL_VTABLE_NONE(jcntrl_mask_extent_function);

static jcntrl_mask_extent_function *
jcntrl_mask_extent_function_downcast_impl(jcntrl_shared_object *obj)
{
  return JCNTRL_DOWNCAST_IMPL(jcntrl_mask_extent_function, obj);
}

static void *jcntrl_mask_extent_function_downcast_v(jcntrl_shared_object *obj)
{
  return jcntrl_mask_extent_function_downcast_impl(obj);
}

static jcntrl_shared_object *
jcntrl_mask_extent_function_object(jcntrl_mask_extent_function *f)
{
  return jcntrl_mask_function_object(&f->func);
}

static int jcntrl_mask_extent_function_initializer(jcntrl_shared_object *obj)
{
  jcntrl_mask_extent_function *f;
  f = jcntrl_mask_extent_function_downcast_impl(obj);
  init_extent_empty(f->extent);
  return 1;
}

static void jcntrl_mask_extent_function_destructor(jcntrl_shared_object *obj)
{
  /* nop */
}

static jcntrl_shared_object *jcntrl_mask_extent_function_allocator(void)
{
  jcntrl_mask_extent_function *f;
  f = jcntrl_shared_object_default_allocator(jcntrl_mask_extent_function);
  return f ? jcntrl_mask_extent_function_object(f) : NULL;
}

static void jcntrl_mask_extent_function_deleter(jcntrl_shared_object *obj)
{
  jcntrl_shared_object_default_deleter(obj);
}

static int
jcntrl_mask_extent_function_eval_impl(jcntrl_shared_object *obj,
                                      jcntrl_cell *cell, int i,
                                      int j, int k)
{
  jcntrl_mask_extent_function *f;
  f = jcntrl_mask_extent_function_downcast_impl(obj);
  if (i < f->extent[0])
    return 1;
  if (j < f->extent[2])
    return 1;
  if (k < f->extent[4])
    return 1;
  if (i < f->extent[1] && j < f->extent[3] && k < f->extent[5])
    return 0;
  return 1;
}

JCNTRL_VIRTUAL_WRAP(jcntrl_mask_extent_function, jcntrl_mask_function, eval)

static void jcntrl_mask_extent_function_init_func(jcntrl_shared_object_funcs *p)
{
  p->downcast = jcntrl_mask_extent_function_downcast_v;
  p->initializer = jcntrl_mask_extent_function_initializer;
  p->destructor = jcntrl_mask_extent_function_destructor;
  p->allocator = jcntrl_mask_extent_function_allocator;
  p->deleter = jcntrl_mask_extent_function_deleter;
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_mask_extent_function, jcntrl_mask_function,
                          eval);
}

JCNTRL_SHARED_METADATA_INIT_DEFINE(jcntrl_mask_extent_function,
                                   jcntrl_mask_extent_function_init_func)

const int *
jcntrl_mask_extent_function_get_extent(jcntrl_mask_extent_function *extent)
{
  JCNTRL_ASSERT(extent);
  return extent->extent;
}

void jcntrl_mask_extent_function_set_extent(jcntrl_mask_extent_function *extent,
                                            const int value[6])
{
  JCNTRL_ASSERT(extent);
  JCNTRL_ASSERT(value);
  memcpy(extent->extent, value, sizeof(extent->extent));
}

//---

struct jcntrl_mask_extent
{
  jcntrl_executive executive;
  int extent[6];
};
#define jcntrl_mask_extent__ancestor jcntrl_executive
#define jcntrl_mask_extent__dnmem executive.jcntrl_executive__dnmem
JCNTRL_VTABLE_NONE(jcntrl_mask_extent);

static jcntrl_mask_extent *
jcntrl_mask_extent_downcast_impl(jcntrl_shared_object *obj)
{
  return JCNTRL_DOWNCAST_IMPL(jcntrl_mask_extent, obj);
}

static void *jcntrl_mask_extent_downcast_v(jcntrl_shared_object *obj)
{
  return jcntrl_mask_extent_downcast_impl(obj);
}

static int jcntrl_mask_extent_initializer(jcntrl_shared_object *obj)
{
  jcntrl_input *input;
  jcntrl_output *output;
  jcntrl_mask_extent *e = jcntrl_mask_extent_downcast_impl(obj);
  init_extent_empty(e->extent);

  input = jcntrl_executive_get_input(jcntrl_mask_extent_executive(e));
  if (!input)
    return 0;

  output = jcntrl_executive_get_output(jcntrl_mask_extent_executive(e));
  if (!output)
    return 0;

  if (!jcntrl_input_set_number_of_ports(input, 0))
    return 0;

  if (!jcntrl_output_set_number_of_ports(output, 1))
    return 0;

  return 1;
}

static void jcntrl_mask_extent_desctructor(jcntrl_shared_object *obj)
{
  /* nop */
}

static jcntrl_shared_object *jcntrl_mask_extent_allocator(void)
{
  jcntrl_mask_extent *e;
  e = jcntrl_shared_object_default_allocator(jcntrl_mask_extent);
  return e ? jcntrl_mask_extent_object(e) : NULL;
}

static void jcntrl_mask_extent_deleter(jcntrl_shared_object *obj)
{
  jcntrl_shared_object_default_deleter(obj);
}

int jcntrl_mask_extent_fill_output_port_information_impl(
  jcntrl_shared_object *data, int index, jcntrl_output *output)
{
  jcntrl_information *info;
  info = jcntrl_output_information(output);
  if (!info)
    return 0;

  if (!jcntrl_information_set_objecttype(info, JCNTRL_INFO_DATATYPE,
                                         jcntrl_mask_extent_function))
    return 0;
  return 1;
}

JCNTRL_VIRTUAL_WRAP(jcntrl_mask_extent, jcntrl_executive,
                    fill_output_port_information)

int jcntrl_mask_extent_process_update_data_impl(jcntrl_information *request,
                                                jcntrl_input *input_head,
                                                jcntrl_output *output_head,
                                                jcntrl_shared_object *data)
{
  jcntrl_mask_extent *e;
  jcntrl_output *output;


  e = jcntrl_mask_extent_downcast_impl(data);

  for (output = jcntrl_output_next_port(output_head); output;
       output = jcntrl_output_next_port(output)) {
    jcntrl_information *info;
    jcntrl_shared_object *obj;
    jcntrl_mask_extent_function *f;

    info = jcntrl_output_information(output);
    if (!info)
      return 0;

    obj = jcntrl_information_get_object(info, JCNTRL_INFO_DATA_OBJECT);
    if (!obj)
      return 0;

    f = jcntrl_shared_object_downcast(jcntrl_mask_extent_function, obj);
    if (!f)
      return 0;

    jcntrl_mask_extent_function_set_extent(f, e->extent);
  }
  return 1;
}

JCNTRL_VIRTUAL_WRAP(jcntrl_mask_extent, jcntrl_executive,
                    process_update_data)

static void jcntrl_mask_extent_init_func(jcntrl_shared_object_funcs *p)
{
  p->downcast = jcntrl_mask_extent_downcast_v;
  p->initializer = jcntrl_mask_extent_initializer;
  p->destructor = jcntrl_mask_extent_desctructor;
  p->allocator = jcntrl_mask_extent_allocator;
  p->deleter = jcntrl_mask_extent_deleter;
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_mask_extent, jcntrl_executive,
                          fill_output_port_information);
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_mask_extent, jcntrl_executive,
                          process_update_data);
}

JCNTRL_SHARED_METADATA_INIT_DEFINE(jcntrl_mask_extent,
                                   jcntrl_mask_extent_init_func)

int jcntrl_install_mask_extent(void)
{
  return jcntrl_executive_install(JCNTRL_MASK_EXTENT, jcntrl_mask_extent);
}

jcntrl_mask_extent *jcntrl_mask_extent_new(void)
{
  return jcntrl_shared_object_new(jcntrl_mask_extent);
}

void jcntrl_mask_extent_delete(jcntrl_mask_extent *e)
{
  jcntrl_shared_object_delete(jcntrl_mask_extent_object(e));
}

jcntrl_executive *jcntrl_mask_extent_executive(jcntrl_mask_extent *e)
{
  return &e->executive;
}

jcntrl_shared_object *jcntrl_mask_extent_object(jcntrl_mask_extent *e)
{
  return jcntrl_executive_object(jcntrl_mask_extent_executive(e));
}

jcntrl_mask_extent *jcntrl_mask_extent_downcast(jcntrl_shared_object *obj)
{
  return jcntrl_shared_object_downcast(jcntrl_mask_extent, obj);
}

const int *jcntrl_mask_extent_get_extent(jcntrl_mask_extent *extent)
{
  JCNTRL_ASSERT(extent);
  return extent->extent;
}

void jcntrl_mask_extent_set_extent(jcntrl_mask_extent *extent,
                                   const int value[6])
{
  JCNTRL_ASSERT(extent);
  JCNTRL_ASSERT(value);
  memcpy(extent->extent, value, sizeof(extent->extent));
}
