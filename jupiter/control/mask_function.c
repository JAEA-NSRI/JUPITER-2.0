#include "mask_function.h"
#include "mask_function_priv.h"
#include "defs.h"
#include "error.h"
#include "shared_object.h"
#include "shared_object_priv.h"
#include "struct_data.h"
#include "mask_object.h"

static jcntrl_mask_function *
jcntrl_mask_function_downcast_impl(jcntrl_shared_object *p)
{
  return JCNTRL_DOWNCAST_IMPL(jcntrl_mask_function, p);
}

static void *jcntrl_mask_function_downcast_v(jcntrl_shared_object *p)
{
  return jcntrl_mask_function_downcast_impl(p);
}

static int jcntrl_mask_function_initializer(jcntrl_shared_object *p)
{
  return 1;
}

static void jcntrl_mask_function_destructor(jcntrl_shared_object *p)
{
  /* nop */
}

int jcntrl_mask_function_eval_impl(jcntrl_shared_object *obj, //
                                   jcntrl_cell *cell, int i, int j, int k)
{
  return 0;
}

JCNTRL_VIRTUAL_WRAP(jcntrl_mask_function, jcntrl_mask_function, eval)

static void jcntrl_mask_function_init_func(jcntrl_shared_object_funcs *p)
{
  p->initializer = jcntrl_mask_function_initializer;
  p->destructor = jcntrl_mask_function_destructor;
  p->downcast = jcntrl_mask_function_downcast_v;
  p->allocator = NULL;
  p->deleter = NULL;
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_mask_function, jcntrl_mask_function, eval);
}

JCNTRL_SHARED_METADATA_INIT_DEFINE(jcntrl_mask_function,
                                   jcntrl_mask_function_init_func)

jcntrl_mask_function *jcntrl_mask_function_downcast(jcntrl_shared_object *p)
{
  return jcntrl_shared_object_downcast(jcntrl_mask_function, p);
}

jcntrl_shared_object *jcntrl_mask_function_object(jcntrl_mask_function *func)
{
  JCNTRL_ASSERT(func);
  return &func->mask_object.data_object.object;
}

void jcntrl_mask_function_delete(jcntrl_mask_function *func)
{
  JCNTRL_ASSERT(func);
  jcntrl_shared_object_delete(jcntrl_mask_function_object(func));
}

int jcntrl_mask_function_eval(jcntrl_mask_function *func, //
                              jcntrl_cell *cell, int i, int j, int k)
{
  struct jcntrl_mask_function_eval_args d = {
    .cell = cell,
    .i = i,
    .j = j,
    .k = k,
    .ret = 0,
  };
  jcntrl_shared_object_call_virtual(jcntrl_mask_function_object(func),
                                    jcntrl_mask_function, eval, &d);
  return d.ret;
}
