#ifndef JUPITER_CONTROL_MASK_FUNCTION_PRIV_H
#define JUPITER_CONTROL_MASK_FUNCTION_PRIV_H

#include "defs.h"
#include "struct_data.h"

JUPITER_CONTROL_DECL_BEGIN

/* See shared_object_priv.h for overriding virtual functions */

typedef int jcntrl_mask_function_eval_func(jcntrl_shared_object *obj,
                                           jcntrl_cell *cell, int i, int j,
                                           int k);

struct jcntrl_mask_function_eval_args
{
  jcntrl_cell *cell;
  int i, j, k;
  int ret;
};

static inline void
jcntrl_mask_function_eval__wrapper(jcntrl_shared_object *obj, void *arg,
                                   jcntrl_mask_function_eval_func *func)
{
  struct jcntrl_mask_function_eval_args *p;
  p = (struct jcntrl_mask_function_eval_args *)arg;
  p->ret = func(obj, p->cell, p->i, p->j, p->k);
}

JUPITER_CONTROL_DECL_END

#endif
