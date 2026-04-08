#ifndef JUPITER_CONTROL_COMPARATOR_H
#define JUPITER_CONTROL_COMPARATOR_H

#include "defs.h"

JUPITER_CONTROL_DECL_BEGIN

static inline int jcntrl_comparator_is_valid(jcntrl_comparator comp)
{
  switch (comp) {
  case JCNTRL_COMP_EQUAL:
  case JCNTRL_COMP_GREATER:
  case JCNTRL_COMP_GREATER_EQ:
  case JCNTRL_COMP_LESS:
  case JCNTRL_COMP_LESS_EQ:
  case JCNTRL_COMP_NOT_EQ:
    return 1;
  case JCNTRL_COMP_INVALID:
    break;
  }
  return 0;
}

typedef int jcntrl_comparator_func(const void *a, const void *b);

struct jcntrl_comparator_funcs
{
  jcntrl_comparator_func *less;
  jcntrl_comparator_func *less_eq;
  jcntrl_comparator_func *equal;
  jcntrl_comparator_func *not_equal;
  jcntrl_comparator_func *greater;
  jcntrl_comparator_func *greater_eq;
};

static inline jcntrl_comparator_func *
jcntrl_comparator_comp(jcntrl_comparator comp,
                       const struct jcntrl_comparator_funcs *f)
{
  switch (comp) {
  case JCNTRL_COMP_EQUAL:
    return f->equal;
  case JCNTRL_COMP_GREATER:
    return f->greater;
  case JCNTRL_COMP_GREATER_EQ:
    return f->greater_eq;
  case JCNTRL_COMP_LESS:
    return f->less;
  case JCNTRL_COMP_LESS_EQ:
    return f->less_eq;
  case JCNTRL_COMP_NOT_EQ:
    return f->not_equal;
  case JCNTRL_COMP_INVALID:
    break;
  }
  return NULL;
}

#define JCNTRL_DEFINE_SCALAR_COMP_IMPL(sym, op, type)                   \
  static inline int jcntrl_compare__##sym(const void *a, const void *b) \
  {                                                                     \
    return (*(const type *)a)op(*(const type *)b);                      \
  }

#define JCNTRL_DEFINE_SCALAR_FUNCS(sym)              \
  static inline const struct jcntrl_comparator_funcs \
    *jcntrl_compare_funcs_##sym(void)                \
  {                                                  \
    static struct jcntrl_comparator_funcs f = {      \
      .less = jcntrl_compare__lt_##sym,              \
      .less_eq = jcntrl_compare__le_##sym,           \
      .equal = jcntrl_compare__eq_##sym,             \
      .not_equal = jcntrl_compare__ne_##sym,         \
      .greater = jcntrl_compare__gt_##sym,           \
      .greater_eq = jcntrl_compare__ge_##sym,        \
    };                                               \
    return &f;                                       \
  }

#define JCNTRL_DEFINE_SCALAR_COMPS_DECL(sym, type) \
  static inline int jcntrl_compare_##sym(jcntrl_comparator comp, type a, type b)

#define JCNTRL_DEFINE_SCALAR_COMPS_IMPL(sym, type)                             \
  JCNTRL_DEFINE_SCALAR_COMPS_DECL(sym, type)                                   \
  {                                                                            \
    return jcntrl_comparator_comp(comp, jcntrl_compare_funcs_##sym())(&a, &b); \
  }

#define JCNTRL_DEFINE_SCALAR_COMPS(sym, type)        \
  JCNTRL_DEFINE_SCALAR_COMPS_DECL(sym, type);        \
  JCNTRL_DEFINE_SCALAR_COMP_IMPL(lt_##sym, <, type)  \
  JCNTRL_DEFINE_SCALAR_COMP_IMPL(le_##sym, <=, type) \
  JCNTRL_DEFINE_SCALAR_COMP_IMPL(eq_##sym, ==, type) \
  JCNTRL_DEFINE_SCALAR_COMP_IMPL(ne_##sym, !=, type) \
  JCNTRL_DEFINE_SCALAR_COMP_IMPL(gt_##sym, >, type)  \
  JCNTRL_DEFINE_SCALAR_COMP_IMPL(ge_##sym, >=, type) \
  JCNTRL_DEFINE_SCALAR_FUNCS(sym)                    \
  JCNTRL_DEFINE_SCALAR_COMPS_IMPL(sym, type)

/**
 * @warning comparator function does not check validity of @p comp. Please check
 * it with jcntrl_comparator_is_valid(comp) prior to call, if it's from an
 * unreliable source.
 */
JCNTRL_DEFINE_SCALAR_COMPS(d, double)
JCNTRL_DEFINE_SCALAR_COMPS(i, int)
JCNTRL_DEFINE_SCALAR_COMPS(s, jcntrl_size_type)
JCNTRL_DEFINE_SCALAR_COMPS(a, jcntrl_aint_type)

JUPITER_CONTROL_DECL_END

#endif
