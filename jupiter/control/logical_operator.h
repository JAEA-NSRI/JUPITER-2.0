#ifndef JUPITER_CONTROL_LOGICAL_OPERATOR_H
#define JUPITER_CONTROL_LOGICAL_OPERATOR_H

#include "defs.h"

JUPITER_CONTROL_DECL_BEGIN

struct jcntrl_logical_evaluator
{
  int (*f)(const void *arg);
  const void *arg;
};
typedef struct jcntrl_logical_evaluator jcntrl_logical_evaluator;

static inline int jcntrl_logical_evaluate(const jcntrl_logical_evaluator *e,
                                          int index)
{
  return e[index].f(e[index].arg);
}

static inline int jcntrl_logical_set_n(int n, const jcntrl_logical_evaluator *a)
{
  return jcntrl_logical_evaluate(a, n - 1);
}

static inline int jcntrl_logical_or_n(int n, const jcntrl_logical_evaluator *a)
{
  for (int i = 0; i < n; ++i)
    if (jcntrl_logical_evaluate(a, i))
      return 1;
  return 0;
}

static inline int jcntrl_logical_nor_n(int n,
                                       const jcntrl_logical_evaluator *a);
static inline int jcntrl_logical_sub_n(int n, const jcntrl_logical_evaluator *a)
{
  int f;
  if (n == 1)
    return jcntrl_logical_evaluate(a, 0);
  f = jcntrl_logical_evaluate(a, 0);
  if (!f)
    return 0;
  return !!f && jcntrl_logical_nor_n(n - 1, a + 1);
}

static inline int jcntrl_logical_and_n(int n, const jcntrl_logical_evaluator *a)
{
  for (int i = 0; i < n; ++i)
    if (!jcntrl_logical_evaluate(a, i))
      return 0;
  return 1;
}

static inline int jcntrl_logical_xor_n(int n, const jcntrl_logical_evaluator *a)
{
  int nt = 0;
  for (int i = 0; i < n; ++i)
    if (jcntrl_logical_evaluate(a, i))
      nt += 1;
  if (nt < n && nt % 2 == 1)
    return 1;
  return 0;
}

static inline int jcntrl_logical_eqv_n(int n, const jcntrl_logical_evaluator *a)
{
  // return jcntrl_logical_and_n(n, a) || jcntrl_logical_nor_n(n, a);
  int r = !!jcntrl_logical_evaluate(a, 0);
  for (int i = 1; i < n; ++i)
    if (r != !!jcntrl_logical_evaluate(a, i))
      return 0;
  return 1;
}

static inline int jcntrl_logical_nor_n(int n, const jcntrl_logical_evaluator *a)
{
  // nothing can be improved to or().
  return !jcntrl_logical_or_n(n, a);
}

static inline int jcntrl_logical_nand_n(int n,
                                        const jcntrl_logical_evaluator *a)
{
  // nothing can be improved to and().
  return !jcntrl_logical_and_n(n, a);
}

static inline int jcntrl_logical_xnor_n(int n,
                                        const jcntrl_logical_evaluator *a)
{
  // nothing can be improved to xor().
  return !jcntrl_logical_xor_n(n, a);
}

static inline int jcntrl_logical_neqv_n(int n,
                                        const jcntrl_logical_evaluator *a)
{
  // nothing can be improved to eqv().
  return !jcntrl_logical_eqv_n(n, a);
}

typedef int jcntrl_logical_calc_func(int n, const jcntrl_logical_evaluator *a);

static inline jcntrl_logical_calc_func *
jcntrl_logical_op_calcfunc(jcntrl_logical_operator op)
{
  switch (op) {
  case JCNTRL_LOP_SET:
    return jcntrl_logical_set_n;
  case JCNTRL_LOP_ADD:
  case JCNTRL_LOP_OR:
    return jcntrl_logical_or_n;
  case JCNTRL_LOP_SUB:
    return jcntrl_logical_sub_n;
  case JCNTRL_LOP_MUL:
  case JCNTRL_LOP_AND:
    return jcntrl_logical_and_n;
  case JCNTRL_LOP_XOR:
    return jcntrl_logical_xor_n;
  case JCNTRL_LOP_EQV:
    return jcntrl_logical_eqv_n;
  case JCNTRL_LOP_NOR:
    return jcntrl_logical_nor_n;
  case JCNTRL_LOP_NAND:
    return jcntrl_logical_nand_n;
  case JCNTRL_LOP_XNOR:
    return jcntrl_logical_xnor_n;
  case JCNTRL_LOP_NEQV:
    return jcntrl_logical_neqv_n;
  case JCNTRL_LOP_INVALID:
    break;
  }
  return NULL;
}

static inline int jcntrl_logical_calc_n(jcntrl_logical_operator op, int n,
                                        const jcntrl_logical_evaluator *a)
{
  jcntrl_logical_calc_func *f = jcntrl_logical_op_calcfunc(op);
  if (f)
    return f(n, a);
  return 0;
}

static inline int jcntrl_logical_wrap_f(const void *arg)
{
  return *(const char *)arg;
}

static inline int jcntrl_logical_wrap_c(int n, const char *a,
                                        jcntrl_logical_calc_func *f)
{
  jcntrl_logical_evaluator e[n];
  for (int i = 0; i < n; ++i)
    e[i] = (jcntrl_logical_evaluator){.f = jcntrl_logical_wrap_f, .arg = &a[i]};
  return f(n, e);
}

static inline int jcntrl_logical_set_c(int n, const char *a)
{
  return jcntrl_logical_wrap_c(n, a, jcntrl_logical_set_n);
}

static inline int jcntrl_logical_or_c(int n, const char *a)
{
  return jcntrl_logical_wrap_c(n, a, jcntrl_logical_or_n);
}

static inline int jcntrl_logical_sub_c(int n, const char *a)
{
  return jcntrl_logical_wrap_c(n, a, jcntrl_logical_sub_n);
}

static inline int jcntrl_logical_and_c(int n, const char *a)
{
  return jcntrl_logical_wrap_c(n, a, jcntrl_logical_and_n);
}

static inline int jcntrl_logical_xor_c(int n, const char *a)
{
  return jcntrl_logical_wrap_c(n, a, jcntrl_logical_xor_n);
}

static inline int jcntrl_logical_eqv_c(int n, const char *a)
{
  return jcntrl_logical_wrap_c(n, a, jcntrl_logical_eqv_n);
}

static inline int jcntrl_logical_nor_c(int n, const char *a)
{
  return jcntrl_logical_wrap_c(n, a, jcntrl_logical_nor_n);
}

static inline int jcntrl_logical_nand_c(int n,
                                        const char *a)
{
  return jcntrl_logical_wrap_c(n, a, jcntrl_logical_nand_n);
}

static inline int jcntrl_logical_xnor_c(int n,
                                        const char *a)
{
  return jcntrl_logical_wrap_c(n, a, jcntrl_logical_xnor_n);
}

static inline int jcntrl_logical_neqv_c(int n,
                                        const char *a)
{
  return jcntrl_logical_wrap_c(n, a, jcntrl_logical_neqv_n);
}

static inline int jcntrl_logical_calc_c(jcntrl_logical_operator op,
                                        int n, const char *a)
{
  jcntrl_logical_calc_func *f = jcntrl_logical_op_calcfunc(op);
  if (f)
    return jcntrl_logical_wrap_c(n, a, f);
  return 0;
}

JUPITER_CONTROL_DECL_END

#endif
