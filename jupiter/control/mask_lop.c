#include "mask_lop.h"
#include "error.h"
#include "executive.h"
#include "information.h"
#include "input.h"
#include "logical_operator.h"
#include "mask_function.h"
#include "mask_function_priv.h"
#include "executive_data.h"
#include "defs.h"
#include "output.h"
#include "overflow.h"
#include "shared_object.h"
#include "shared_object_priv.h"
#include "struct_data.h"

#include <stdlib.h>
#include <string.h>

/**
 * @todo We need to support jcntrl_mask_data (and extent) when JCNTRL_MASK_GET
 * (get from grid) or JCNTRL_MASK_GRID (condition for grid attribute array) gets
 * implemented.
 */
struct jcntrl_mask_lop_function
{
  jcntrl_mask_function func;
  jcntrl_logical_operator op;
  jcntrl_size_type ninputs;
  jcntrl_mask_function **inputs;
};
#define jcntrl_mask_lop_function__ancestor jcntrl_mask_function
#define jcntrl_mask_lop_function__dnmem func.jcntrl_mask_function__dnmem
JCNTRL_VTABLE_NONE(jcntrl_mask_lop_function);

static jcntrl_mask_lop_function *
jcntrl_mask_lop_function_downcast_impl(jcntrl_shared_object *obj)
{
  return JCNTRL_DOWNCAST_IMPL(jcntrl_mask_lop_function, obj);
}

static void *jcntrl_mask_lop_function_downcast_v(jcntrl_shared_object *obj)
{
  return jcntrl_mask_lop_function_downcast_impl(obj);
}

static int jcntrl_mask_lop_function_initializer(jcntrl_shared_object *obj)
{
  jcntrl_mask_lop_function *p;
  p = jcntrl_mask_lop_function_downcast_impl(obj);
  p->op = JCNTRL_LOP_INVALID;
  p->ninputs = 0;
  p->inputs = NULL;
  return 1;
}

static void jcntrl_mask_lop_function_destructor(jcntrl_shared_object *obj)
{
  jcntrl_mask_lop_function *p;
  p = jcntrl_mask_lop_function_downcast_impl(obj);
  jcntrl_mask_lop_function_set_number_of_inputs(p, 0);
}

static jcntrl_shared_object *jcntrl_mask_lop_function_allocator(void)
{
  jcntrl_mask_lop_function *p;
  p = jcntrl_shared_object_default_allocator(jcntrl_mask_lop_function);
  return p ? jcntrl_mask_lop_function_object(p) : NULL;
}

static void jcntrl_mask_lop_function_deleter(jcntrl_shared_object *obj)
{
  jcntrl_shared_object_default_deleter(obj);
}

struct jcntrl_mask_lop_function_data
{
  jcntrl_mask_function *f;
  struct jcntrl_mask_function_eval_args *d;
};

static int jcntrl_mask_lop_evals(const void *d)
{
  const struct jcntrl_mask_lop_function_data *args = d;
  if (!args->f)
    return 0;

  return jcntrl_mask_function_eval(args->f, args->d->cell, args->d->i,
                                   args->d->j, args->d->k);
}

static int jcntrl_mask_lop_function_eval_impl(jcntrl_shared_object *obj,
                                              jcntrl_cell *cell, int i, int j,
                                              int k)
{
  int n;
  jcntrl_mask_lop_function *p;
  struct jcntrl_mask_lop_function_data *d;
  jcntrl_logical_evaluator *evals;
  int r;
  struct jcntrl_mask_function_eval_args args = {
    .cell = cell,
    .i = i,
    .j = j,
    .k = k,
  };

  p = jcntrl_mask_lop_function_downcast_impl(obj);
  if (p->ninputs <= 0)
    return 0;

  n = p->ninputs;
  if (n != p->ninputs) {
    jcntrl_raise_overflow_error(__FILE__, __LINE__);
    return 0;
  }

  d = NULL;
  evals = NULL;
  r = 0;
  do {
    d = calloc(n, sizeof(struct jcntrl_mask_lop_function_data));
    if (!d)
      break;

    evals = calloc(n, sizeof(jcntrl_logical_evaluator));
    if (!evals)
      break;

    for (int i = 0; i < n; ++i) {
      d[i].d = &args;
      d[i].f = p->inputs[i];
      evals[i].arg = &d[i];
      evals[i].f = jcntrl_mask_lop_evals;
    }

    r = jcntrl_logical_calc_n(p->op, n, evals);
  } while (0);

  if (d)
    free(d);
  if (evals)
    free(evals);
  return r;
}

JCNTRL_VIRTUAL_WRAP(jcntrl_mask_lop_function, jcntrl_mask_function, eval)

static void jcntrl_mask_lop_function_init_func(jcntrl_shared_object_funcs *p)
{
  p->downcast = jcntrl_mask_lop_function_downcast_v;
  p->initializer = jcntrl_mask_lop_function_initializer;
  p->destructor = jcntrl_mask_lop_function_destructor;
  p->allocator = jcntrl_mask_lop_function_allocator;
  p->deleter = jcntrl_mask_lop_function_deleter;
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_mask_lop_function, jcntrl_mask_function,
                          eval);
}

JCNTRL_SHARED_METADATA_INIT_DEFINE(jcntrl_mask_lop_function,
                                   jcntrl_mask_lop_function_init_func)

//----

struct jcntrl_mask_lop
{
  jcntrl_executive exec;
  jcntrl_logical_operator op;
};
#define jcntrl_mask_lop__ancestor jcntrl_executive
#define jcntrl_mask_lop__dnmem exec.jcntrl_executive__dnmem
JCNTRL_VTABLE_NONE(jcntrl_mask_lop);

static jcntrl_mask_lop *jcntrl_mask_lop_downcast_impl(jcntrl_shared_object *obj)
{
  return JCNTRL_DOWNCAST_IMPL(jcntrl_mask_lop, obj);
}

static void *jcntrl_mask_lop_downcast_v(jcntrl_shared_object *obj)
{
  return jcntrl_mask_lop_downcast_impl(obj);
}

static int jcntrl_mask_lop_initializer(jcntrl_shared_object *obj)
{
  jcntrl_executive *exec;
  jcntrl_output *output;
  jcntrl_mask_lop *p = jcntrl_mask_lop_downcast_impl(obj);
  p->op = JCNTRL_LOP_INVALID;

  exec = jcntrl_mask_lop_executive(p);
  output = jcntrl_executive_get_output(exec);
  if (!jcntrl_output_set_number_of_ports(output, 1))
    return 0;
  return 1;
}

static void jcntrl_mask_lop_destructor(jcntrl_shared_object *obj) { /* nop */ }

static jcntrl_shared_object *jcntrl_mask_lop_allocator(void)
{
  jcntrl_mask_lop *p;
  p = jcntrl_shared_object_default_allocator(jcntrl_mask_lop);
  return p ? jcntrl_mask_lop_object(p) : NULL;
}

static void jcntrl_mask_lop_deleter(jcntrl_shared_object *obj)
{
  jcntrl_shared_object_default_deleter(obj);
}

static int
jcntrl_mask_lop_fill_input_port_information_impl(jcntrl_shared_object *object,
                                                 int index, jcntrl_input *input)
{
  jcntrl_information *info;
  info = jcntrl_input_information(input);

  /* Currently only accepts mask functions (See above) */
  if (!jcntrl_information_set_datatype(info, JCNTRL_INFO_REQUIRED_DATATYPE,
                                       JCNTRL_DATATYPE_MASK_FUN))
    return 0;
  return 1;
}

JCNTRL_VIRTUAL_WRAP(jcntrl_mask_lop, jcntrl_executive,
                    fill_input_port_information)

static int jcntrl_mask_lop_fill_output_port_information_impl(
  jcntrl_shared_object *object, int index, jcntrl_output *output)
{
  jcntrl_information *info;
  info = jcntrl_output_information(output);

  if (!jcntrl_information_set_objecttype(info, JCNTRL_INFO_DATATYPE,
                                         jcntrl_mask_lop_function))
    return 0;
  return 1;
}

JCNTRL_VIRTUAL_WRAP(jcntrl_mask_lop, jcntrl_executive,
                    fill_output_port_information)

static int jcntrl_mask_lop_process_update_data_impl(jcntrl_information *request,
                                                    jcntrl_input *input_head,
                                                    jcntrl_output *output_head,
                                                    jcntrl_shared_object *obj)
{
  jcntrl_input *input;
  jcntrl_size_type ninput;
  jcntrl_mask_lop_function *f;
  jcntrl_mask_lop *p;
  p = jcntrl_mask_lop_downcast_impl(obj);

  if (!jcntrl_logical_op_calcfunc(p->op)) {
    jcntrl_raise_argument_error(__FILE__, __LINE__, "Invalid logical operator");
    return 0;
  }

  ninput = 0;
  input = jcntrl_input_next_port(input_head);
  for (; input; input = jcntrl_input_next_port(input)) {
    if (!jcntrl_input_upstream_information(input))
      continue;

    if (jcntrl_s_add_overflow(ninput, 1, &ninput))
      return 0;
  }

  f = jcntrl_output_get_data_object_at_as(output_head, 0,
                                          jcntrl_mask_lop_function);
  JCNTRL_ASSERT(f);

  jcntrl_mask_lop_function_set_op(f, p->op);

  if (!jcntrl_mask_lop_function_set_number_of_inputs(f, ninput))
    return 0;

  input = jcntrl_input_next_port(input_head);
  for (jcntrl_size_type i = 0; input; input = jcntrl_input_next_port(input)) {
    jcntrl_mask_function *upf;

    if (!jcntrl_input_upstream_information(input))
      continue;

    upf = jcntrl_input_get_data_object_as(input, jcntrl_mask_function);
    JCNTRL_ASSERT(upf);

    if (!jcntrl_mask_lop_function_set_input(f, i, upf))
      return 0;

    ++i;
  }

  return 1;
}

JCNTRL_VIRTUAL_WRAP(jcntrl_mask_lop, jcntrl_executive, process_update_data)

static void jcntrl_mask_lop_init_func(jcntrl_shared_object_funcs *p)
{
  p->downcast = jcntrl_mask_lop_downcast_v;
  p->initializer = jcntrl_mask_lop_initializer;
  p->destructor = jcntrl_mask_lop_destructor;
  p->allocator = jcntrl_mask_lop_allocator;
  p->deleter = jcntrl_mask_lop_deleter;
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_mask_lop, jcntrl_executive,
                          fill_input_port_information);
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_mask_lop, jcntrl_executive,
                          fill_output_port_information);
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_mask_lop, jcntrl_executive,
                          process_update_data);
}

JCNTRL_SHARED_METADATA_INIT_DEFINE(jcntrl_mask_lop, jcntrl_mask_lop_init_func)

//----

#define jcntrl_mask_or__ancestor jcntrl_mask_lop
#define jcntrl_mask_sub__ancestor jcntrl_mask_lop
#define jcntrl_mask_and__ancestor jcntrl_mask_lop
#define jcntrl_mask_xor__ancestor jcntrl_mask_lop
#define jcntrl_mask_eqv__ancestor jcntrl_mask_lop
#define jcntrl_mask_nor__ancestor jcntrl_mask_lop
#define jcntrl_mask_nand__ancestor jcntrl_mask_lop
#define jcntrl_mask_xnor__ancestor jcntrl_mask_lop
#define jcntrl_mask_neqv__ancestor jcntrl_mask_lop

#define jcntrl_mask_or__dnmem lop.jcntrl_mask_lop__dnmem
#define jcntrl_mask_sub__dnmem lop.jcntrl_mask_lop__dnmem
#define jcntrl_mask_and__dnmem lop.jcntrl_mask_lop__dnmem
#define jcntrl_mask_xor__dnmem lop.jcntrl_mask_lop__dnmem
#define jcntrl_mask_eqv__dnmem lop.jcntrl_mask_lop__dnmem
#define jcntrl_mask_nor__dnmem lop.jcntrl_mask_lop__dnmem
#define jcntrl_mask_nand__dnmem lop.jcntrl_mask_lop__dnmem
#define jcntrl_mask_xnor__dnmem lop.jcntrl_mask_lop__dnmem
#define jcntrl_mask_neqv__dnmem lop.jcntrl_mask_lop__dnmem

#define JCNTRL_MASK_LOP_DEFINE_STRUCT(name) \
  struct jcntrl_mask_##name                 \
  {                                         \
    jcntrl_mask_lop lop;                    \
  };                                        \
  JCNTRL_VTABLE_NONE(jcntrl_mask_##name)

#define JCNTRL_MASK_LOP_DEFINE_DOWNCAST_IMPL(name)                        \
  static jcntrl_mask_##name *jcntrl_mask_##name##_downcast_impl(          \
    jcntrl_shared_object *obj)                                            \
  {                                                                       \
    return JCNTRL_DOWNCAST_IMPL(jcntrl_mask_##name, obj);                 \
  }                                                                       \
                                                                          \
  static void *jcntrl_mask_##name##_downcast_v(jcntrl_shared_object *obj) \
  {                                                                       \
    return jcntrl_mask_##name##_downcast_impl(obj);                       \
  }

#define JCNTRL_MASK_LOP_DEFINE_INIT(name, opval)                         \
  static int jcntrl_mask_##name##_initializer(jcntrl_shared_object *obj) \
  {                                                                      \
    jcntrl_mask_##name *p = jcntrl_mask_##name##_downcast_impl(obj);     \
    p->lop.op = opval;                                                   \
    return 1;                                                            \
  }

#define JCNTRL_MASK_LOP_DEFINE_ALLOCATOR(name)                      \
  static jcntrl_shared_object *jcntrl_mask_##name##_allocator(void) \
  {                                                                 \
    jcntrl_mask_##name *p;                                          \
    p = jcntrl_shared_object_default_allocator(jcntrl_mask_##name); \
    return p ? jcntrl_mask_##name##_object(p) : NULL;               \
  }

#define JCNTRL_MASK_LOP_DEFINE_DELETER(name)                          \
  static void jcntrl_mask_##name##_deleter(jcntrl_shared_object *obj) \
  {                                                                   \
    jcntrl_shared_object_default_deleter(obj);                        \
  }

#define JCNTRL_MASK_LOP_DEFINE_INIT_FUNC(name)                              \
  static void jcntrl_mask_##name##_init_func(jcntrl_shared_object_funcs *p) \
  {                                                                         \
    p->downcast = jcntrl_mask_##name##_downcast_v;                          \
    p->initializer = jcntrl_mask_##name##_initializer;                      \
    p->allocator = jcntrl_mask_##name##_allocator;                          \
    p->deleter = jcntrl_mask_##name##_deleter;                              \
  }                                                                         \
                                                                            \
  JCNTRL_SHARED_METADATA_INIT_DEFINE(jcntrl_mask_##name,                    \
                                     jcntrl_mask_##name##_init_func)

#define JCNTRL_MASK_LOP_DEFINE_ALL(name, op) \
  JCNTRL_MASK_LOP_DEFINE_STRUCT(name);       \
  JCNTRL_MASK_LOP_DEFINE_DOWNCAST_IMPL(name) \
  JCNTRL_MASK_LOP_DEFINE_INIT(name, op)      \
  JCNTRL_MASK_LOP_DEFINE_ALLOCATOR(name)     \
  JCNTRL_MASK_LOP_DEFINE_DELETER(name)       \
  JCNTRL_MASK_LOP_DEFINE_INIT_FUNC(name)

JCNTRL_MASK_LOP_DEFINE_ALL(or, JCNTRL_LOP_OR)
JCNTRL_MASK_LOP_DEFINE_ALL(sub, JCNTRL_LOP_SUB)
JCNTRL_MASK_LOP_DEFINE_ALL(and, JCNTRL_LOP_AND)
JCNTRL_MASK_LOP_DEFINE_ALL(xor, JCNTRL_LOP_XOR)
JCNTRL_MASK_LOP_DEFINE_ALL(eqv, JCNTRL_LOP_EQV)
JCNTRL_MASK_LOP_DEFINE_ALL(nor, JCNTRL_LOP_NOR)
JCNTRL_MASK_LOP_DEFINE_ALL(nand, JCNTRL_LOP_NAND)
JCNTRL_MASK_LOP_DEFINE_ALL(xnor, JCNTRL_LOP_XNOR)
JCNTRL_MASK_LOP_DEFINE_ALL(neqv, JCNTRL_LOP_NEQV)

//----

jcntrl_mask_lop *jcntrl_mask_lop_new(void)
{
  return jcntrl_shared_object_new(jcntrl_mask_lop);
}

jcntrl_mask_or *jcntrl_mask_or_new(void)
{
  return jcntrl_shared_object_new(jcntrl_mask_or);
}

jcntrl_mask_sub *jcntrl_mask_sub_new(void)
{
  return jcntrl_shared_object_new(jcntrl_mask_sub);
}

jcntrl_mask_and *jcntrl_mask_and_new(void)
{
  return jcntrl_shared_object_new(jcntrl_mask_and);
}

jcntrl_mask_xor *jcntrl_mask_xor_new(void)
{
  return jcntrl_shared_object_new(jcntrl_mask_xor);
}

jcntrl_mask_eqv *jcntrl_mask_eqv_new(void)
{
  return jcntrl_shared_object_new(jcntrl_mask_eqv);
}

jcntrl_mask_nand *jcntrl_mask_nand_new(void)
{
  return jcntrl_shared_object_new(jcntrl_mask_nand);
}

jcntrl_mask_nor *jcntrl_mask_nor_new(void)
{
  return jcntrl_shared_object_new(jcntrl_mask_nor);
}

jcntrl_mask_xnor *jcntrl_mask_xnor_new(void)
{
  return jcntrl_shared_object_new(jcntrl_mask_xnor);
}

jcntrl_mask_neqv *jcntrl_mask_neqv_new(void)
{
  return jcntrl_shared_object_new(jcntrl_mask_neqv);
}

//----

void jcntrl_mask_lop_delete(jcntrl_mask_lop *m)
{
  jcntrl_shared_object_delete(jcntrl_mask_lop_object(m));
}

void jcntrl_mask_or_delete(jcntrl_mask_or *m)
{
  jcntrl_shared_object_delete(jcntrl_mask_or_object(m));
}

void jcntrl_mask_sub_delete(jcntrl_mask_sub *m)
{
  jcntrl_shared_object_delete(jcntrl_mask_sub_object(m));
}

void jcntrl_mask_and_delete(jcntrl_mask_and *m)
{
  jcntrl_shared_object_delete(jcntrl_mask_and_object(m));
}

void jcntrl_mask_xor_delete(jcntrl_mask_xor *m)
{
  jcntrl_shared_object_delete(jcntrl_mask_xor_object(m));
}

void jcntrl_mask_eqv_delete(jcntrl_mask_eqv *m)
{
  jcntrl_shared_object_delete(jcntrl_mask_eqv_object(m));
}

void jcntrl_mask_nand_delete(jcntrl_mask_nand *m)
{
  jcntrl_shared_object_delete(jcntrl_mask_nand_object(m));
}

void jcntrl_mask_nor_delete(jcntrl_mask_nand *m)
{
  jcntrl_shared_object_delete(jcntrl_mask_nor_object(m));
}

void jcntrl_mask_xnor_delete(jcntrl_mask_xnor *m)
{
  jcntrl_shared_object_delete(jcntrl_mask_xnor_object(m));
}

void jcntrl_mask_neqv_delete(jcntrl_mask_neqv *m)
{
  jcntrl_shared_object_delete(jcntrl_mask_neqv_object(m));
}

//----

jcntrl_shared_object *jcntrl_mask_lop_object(jcntrl_mask_lop *m)
{
  return jcntrl_executive_object(jcntrl_mask_lop_executive(m));
}

jcntrl_shared_object *jcntrl_mask_or_object(jcntrl_mask_or *m)
{
  return jcntrl_executive_object(jcntrl_mask_or_executive(m));
}

jcntrl_shared_object *jcntrl_mask_sub_object(jcntrl_mask_sub *m)
{
  return jcntrl_executive_object(jcntrl_mask_sub_executive(m));
}

jcntrl_shared_object *jcntrl_mask_and_object(jcntrl_mask_and *m)
{
  return jcntrl_executive_object(jcntrl_mask_and_executive(m));
}

jcntrl_shared_object *jcntrl_mask_xor_object(jcntrl_mask_xor *m)
{
  return jcntrl_executive_object(jcntrl_mask_xor_executive(m));
}

jcntrl_shared_object *jcntrl_mask_eqv_object(jcntrl_mask_eqv *m)
{
  return jcntrl_executive_object(jcntrl_mask_eqv_executive(m));
}

jcntrl_shared_object *jcntrl_mask_nand_object(jcntrl_mask_nand *m)
{
  return jcntrl_executive_object(jcntrl_mask_nand_executive(m));
}

jcntrl_shared_object *jcntrl_mask_nor_object(jcntrl_mask_nor *m)
{
  return jcntrl_executive_object(jcntrl_mask_nor_executive(m));
}

jcntrl_shared_object *jcntrl_mask_xnor_object(jcntrl_mask_xnor *m)
{
  return jcntrl_executive_object(jcntrl_mask_xnor_executive(m));
}

jcntrl_shared_object *jcntrl_mask_neqv_object(jcntrl_mask_neqv *m)
{
  return jcntrl_executive_object(jcntrl_mask_neqv_executive(m));
}

//----

jcntrl_executive *jcntrl_mask_lop_executive(jcntrl_mask_lop *m)
{
  return &m->exec;
}

jcntrl_executive *jcntrl_mask_or_executive(jcntrl_mask_or *m)
{
  return jcntrl_mask_lop_executive(jcntrl_mask_or_lop(m));
}

jcntrl_executive *jcntrl_mask_sub_executive(jcntrl_mask_sub *m)
{
  return jcntrl_mask_lop_executive(jcntrl_mask_sub_lop(m));
}

jcntrl_executive *jcntrl_mask_and_executive(jcntrl_mask_and *m)
{
  return jcntrl_mask_lop_executive(jcntrl_mask_and_lop(m));
}

jcntrl_executive *jcntrl_mask_xor_executive(jcntrl_mask_xor *m)
{
  return jcntrl_mask_lop_executive(jcntrl_mask_xor_lop(m));
}

jcntrl_executive *jcntrl_mask_eqv_executive(jcntrl_mask_eqv *m)
{
  return jcntrl_mask_lop_executive(jcntrl_mask_eqv_lop(m));
}

jcntrl_executive *jcntrl_mask_nand_executive(jcntrl_mask_nand *m)
{
  return jcntrl_mask_lop_executive(jcntrl_mask_nand_lop(m));
}

jcntrl_executive *jcntrl_mask_nor_executive(jcntrl_mask_nor *m)
{
  return jcntrl_mask_lop_executive(jcntrl_mask_nor_lop(m));
}

jcntrl_executive *jcntrl_mask_xnor_executive(jcntrl_mask_xnor *m)
{
  return jcntrl_mask_lop_executive(jcntrl_mask_xnor_lop(m));
}

jcntrl_executive *jcntrl_mask_neqv_executive(jcntrl_mask_neqv *m)
{
  return jcntrl_mask_lop_executive(jcntrl_mask_neqv_lop(m));
}

//----

jcntrl_mask_lop *jcntrl_mask_lop_downcast(jcntrl_shared_object *obj)
{
  return jcntrl_shared_object_downcast(jcntrl_mask_lop, obj);
}

jcntrl_mask_or *jcntrl_mask_or_downcast(jcntrl_shared_object *obj)
{
  return jcntrl_shared_object_downcast(jcntrl_mask_or, obj);
}

jcntrl_mask_sub *jcntrl_mask_sub_downcast(jcntrl_shared_object *obj)
{
  return jcntrl_shared_object_downcast(jcntrl_mask_sub, obj);
}

jcntrl_mask_and *jcntrl_mask_and_downcast(jcntrl_shared_object *obj)
{
  return jcntrl_shared_object_downcast(jcntrl_mask_and, obj);
}

jcntrl_mask_xor *jcntrl_mask_xor_downcast(jcntrl_shared_object *obj)
{
  return jcntrl_shared_object_downcast(jcntrl_mask_xor, obj);
}

jcntrl_mask_eqv *jcntrl_mask_eqv_downcast(jcntrl_shared_object *obj)
{
  return jcntrl_shared_object_downcast(jcntrl_mask_eqv, obj);
}

jcntrl_mask_nand *jcntrl_mask_nand_downcast(jcntrl_shared_object *obj)
{
  return jcntrl_shared_object_downcast(jcntrl_mask_nand, obj);
}

jcntrl_mask_nor *jcntrl_mask_nor_downcast(jcntrl_shared_object *obj)
{
  return jcntrl_shared_object_downcast(jcntrl_mask_nor, obj);
}

jcntrl_mask_xnor *jcntrl_mask_xnor_downcast(jcntrl_shared_object *obj)
{
  return jcntrl_shared_object_downcast(jcntrl_mask_xnor, obj);
}

jcntrl_mask_neqv *jcntrl_mask_neqv_downcast(jcntrl_shared_object *obj)
{
  return jcntrl_shared_object_downcast(jcntrl_mask_neqv, obj);
}

//----

jcntrl_mask_lop *jcntrl_mask_or_lop(jcntrl_mask_or *m) { return &m->lop; }
jcntrl_mask_lop *jcntrl_mask_sub_lop(jcntrl_mask_sub *m) { return &m->lop; }
jcntrl_mask_lop *jcntrl_mask_and_lop(jcntrl_mask_and *m) { return &m->lop; }
jcntrl_mask_lop *jcntrl_mask_xor_lop(jcntrl_mask_xor *m) { return &m->lop; }
jcntrl_mask_lop *jcntrl_mask_eqv_lop(jcntrl_mask_eqv *m) { return &m->lop; }
jcntrl_mask_lop *jcntrl_mask_nand_lop(jcntrl_mask_nand *m) { return &m->lop; }
jcntrl_mask_lop *jcntrl_mask_nor_lop(jcntrl_mask_nor *m) { return &m->lop; }
jcntrl_mask_lop *jcntrl_mask_xnor_lop(jcntrl_mask_xnor *m) { return &m->lop; }
jcntrl_mask_lop *jcntrl_mask_neqv_lop(jcntrl_mask_neqv *m) { return &m->lop; }

//----

int jcntrl_mask_lop_set_op(jcntrl_mask_lop *m, jcntrl_logical_operator op)
{
  if (!jcntrl_logical_op_calcfunc(op)) {
    jcntrl_raise_argument_error(__FILE__, __LINE__,
                                "Invalid logical operator given");
    return 0;
  }

  if (jcntrl_shared_object_class(jcntrl_mask_lop_object(m)) !=
      jcntrl_mask_lop_metadata_init()) {
    jcntrl_raise_argument_error(__FILE__, __LINE__,
                                "Cannot set operator for subclassed objects");
    return 0;
  }

  m->op = op;
  return 1;
}

jcntrl_logical_operator jcntrl_mask_lop_op(jcntrl_mask_lop *m)
{
  return m->op;
}

//----

jcntrl_shared_object *
jcntrl_mask_lop_function_object(jcntrl_mask_lop_function *f)
{
  return jcntrl_mask_function_object(jcntrl_mask_lop_function_function(f));
}

jcntrl_mask_function *
jcntrl_mask_lop_function_function(jcntrl_mask_lop_function *f)
{
  return &f->func;
}

jcntrl_mask_lop_function *
jcntrl_mask_lop_function_downcast(jcntrl_shared_object *obj)
{
  return jcntrl_shared_object_downcast(jcntrl_mask_lop_function, obj);
}

int jcntrl_mask_lop_function_set_op(jcntrl_mask_lop_function *m,
                                    jcntrl_logical_operator op)
{
  if (!jcntrl_logical_op_calcfunc(op)) {
    jcntrl_raise_argument_error(__FILE__, __LINE__,
                                "Invalid logical operator given");
    return 0;
  }

  m->op = op;
  return 1;
}

jcntrl_logical_operator
jcntrl_mask_lop_function_op(jcntrl_mask_lop_function *m)
{
  return m->op;
}

int jcntrl_mask_lop_function_set_number_of_inputs(jcntrl_mask_lop_function *m,
                                                  jcntrl_size_type n)
{
  jcntrl_mask_function **funcs;
  jcntrl_size_type bsz;
  JCNTRL_ASSERT(m);
  JCNTRL_ASSERT(n >= 0);

  if (n == m->ninputs)
    return 1;

  bsz = sizeof(jcntrl_mask_function *);
  if (jcntrl_s_mul_overflow(n, bsz, &bsz))
    return 0;

  if (n < m->ninputs) {
    for (jcntrl_size_type i = n; i < m->ninputs; ++i)
      jcntrl_mask_lop_function_set_input(m, i, NULL);
  }

  if (bsz > 0) {
    funcs = realloc(m->inputs, bsz);
    if (!funcs) {
      jcntrl_raise_allocation_failed(__FILE__, __LINE__);
      return 0;
    }
  } else {
    if (m->inputs)
      free(m->inputs);
    funcs = NULL;
  }

  if (n > m->ninputs) {
    for (jcntrl_size_type i = m->ninputs; i < n; ++i)
      funcs[i] = NULL;
  }

  m->inputs = funcs;
  m->ninputs = n;
  return 1;
}

jcntrl_size_type
jcntrl_mask_lop_function_get_number_of_inputs(jcntrl_mask_lop_function *m)
{
  return m->ninputs;
}

int jcntrl_mask_lop_function_set_input(jcntrl_mask_lop_function *m,
                                       jcntrl_size_type index,
                                       jcntrl_mask_function *f)
{
  if (index < 0 || index > m->ninputs) {
    jcntrl_raise_index_error(__FILE__, __LINE__, index);
    return 0;
  }

  if (m->inputs[index] == f)
    return 1;

  if (m->inputs[index]) {
    jcntrl_shared_object *o;
    o = jcntrl_mask_function_object(m->inputs[index]);
    jcntrl_shared_object_release_ownership(o);
  }
  if (f)
    jcntrl_shared_object_take_ownership(jcntrl_mask_function_object(f));
  m->inputs[index] = f;
  return 1;
}

jcntrl_mask_function *
jcntrl_mask_lop_function_get_input(jcntrl_mask_lop_function *m,
                                   jcntrl_size_type index)
{
  if (index < 0 || index > m->ninputs) {
    jcntrl_raise_index_error(__FILE__, __LINE__, index);
    return 0;
  }

  return m->inputs[index];
}
