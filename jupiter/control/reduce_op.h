#ifndef JUPITER_CONTROL_REDUCE_OP_H
#define JUPITER_CONTROL_REDUCE_OP_H

#include "defs.h"
#include "shared_object.h"
#include "shared_object_priv.h"
#include "mpi_controller.h"

JUPITER_CONTROL_DECL_BEGIN

struct jcntrl_reduce_op
{
  jcntrl_shared_object object;
  int commute;
};
#define jcntrl_reduce_op__ancestor jcntrl_shared_object
#define jcntrl_reduce_op__dnmem object
enum jcntrl_reduce_op_vtable_names
{
  jcntrl_reduce_op_calc_id = JCNTRL_VTABLE_START(jcntrl_reduce_op),
  jcntrl_reduce_op_mpi_op_create_id, // internally used. cannot override
  jcntrl_reduce_op_mpi_op_free_id,   // internally used. cannot override
  JCNTRL_VTABLE_SIZE(jcntrl_reduce_op),
};

static inline jcntrl_shared_object *
jcntrl_reduce_op_object(jcntrl_reduce_op *op)
{
  return &op->object;
}

static inline jcntrl_reduce_op *
jcntrl_reduce_op_downcast(jcntrl_shared_object *obj)
{
  return jcntrl_shared_object_downcast(jcntrl_reduce_op, obj);
}

//-----

typedef void jcntrl_reduce_op_calc_func(jcntrl_data_array *invec,
                                        jcntrl_data_array *outvec,
                                        jcntrl_shared_object *object);

struct jcntrl_reduce_op_calc_args
{
  jcntrl_data_array *invec;
  jcntrl_data_array *outvec;
};

static inline void
jcntrl_reduce_op_calc__wrapper(jcntrl_shared_object *obj, void *arg,
                               jcntrl_reduce_op_calc_func *func)
{
  struct jcntrl_reduce_op_calc_args *a = arg;
  func(a->invec, a->outvec, obj);
}

static inline void jcntrl_reduce_op_calc(jcntrl_reduce_op *op,
                                         jcntrl_data_array *invec,
                                         jcntrl_data_array *outvec)
{
  struct jcntrl_reduce_op_calc_args a = {
    .invec = invec,
    .outvec = outvec,
  };
  jcntrl_shared_object_call_virtual(jcntrl_reduce_op_object(op),
                                    jcntrl_reduce_op, calc, &a);
}

static inline void
jcntrl_reduce_op_calc__super(const jcntrl_shared_object_data *ancestor,
                             jcntrl_data_array *invec,
                             jcntrl_data_array *outvec)
{
  struct jcntrl_reduce_op_calc_args a = {
    .invec = invec,
    .outvec = outvec,
  };
  jcntrl_shared_object_call_super(ancestor, jcntrl_reduce_op, calc, &a);
}

JUPITER_CONTROL_DECL_END

#endif
