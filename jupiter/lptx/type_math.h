#ifndef JUPITER_LPTX_TYPE_MATH_H
#define JUPITER_LPTX_TYPE_MATH_H

#include "defs.h"

#include <math.h>

JUPITER_LPTX_DECL_START

static inline LPTX_bool LPTX_type_isnan(LPTX_type a)
{
#ifdef isnan
  /* isnan is type-generic by default */
  return isnan(a);
#else
  return LPTX_false;
#endif
}

static inline LPTX_type LPTX_type_min(LPTX_type a, LPTX_type b)
{
#ifdef isnan
  if (LPTX_type_isnan(a))
    return b;
  if (LPTX_type_isnan(b))
    return a;
#endif
  return (a < b) ? a : b;
}

static inline LPTX_type LPTX_type_max(LPTX_type a, LPTX_type b)
{
#ifdef isnan
  if (LPTX_type_isnan(a))
    return b;
  if (LPTX_type_isnan(b))
    return a;
#endif
  return (a < b) ? b : a;
}

#define LPTX__define_type_math_arg(x, y) x y
#define LPTX__define_type_math_var(x, y) y

#define LPTX__define_type_math_c(a) a(const char *, a1)

#define LPTX__define_type_math_d(a) a(LPTX_type, a1)
#define LPTX__define_type_math_dd(a) \
  LPTX__define_type_math_d(a), a(LPTX_type, a2)
#define LPTX__define_type_math_ddd(a) \
  LPTX__define_type_math_dd(a), a(LPTX_type, a3)
#define LPTX__define_type_math_dr(a) \
  LPTX__define_type_math_d(a), a(LPTX_type *, a3)
#define LPTX__define_type_math_ddr(a) \
  LPTX__define_type_math_dd(a), a(LPTX_type *, a3)
#define LPTX__define_type_math_dp(a) LPTX__define_type_math_d(a), a(int *, a2)
#define LPTX__define_type_math_ddp(a) LPTX__define_type_math_dd(a), a(int *, a3)
#define LPTX__define_type_math_di(a) LPTX__define_type_math_d(a), a(int, a2)
#define LPTX__define_type_math_dl(a) LPTX__define_type_math_d(a), a(long, a2)

#define LPTX__define_type_math_args(n) \
  LPTX__define_type_math_##n(LPTX__define_type_math_arg)
#define LPTX__define_type_math_vars(n) \
  LPTX__define_type_math_##n(LPTX__define_type_math_var)

#define LPTX_DEFINE_TYPE_MATH(name, rtype, ap)                          \
  static inline rtype LPTX_type_##name(LPTX__define_type_math_args(ap)) \
  {                                                                     \
    return LPTX_math(name)(LPTX__define_type_math_vars(ap));            \
  }

LPTX_DEFINE_TYPE_MATH(acos, LPTX_type, d)
LPTX_DEFINE_TYPE_MATH(acosh, LPTX_type, d)
LPTX_DEFINE_TYPE_MATH(cos, LPTX_type, d)
LPTX_DEFINE_TYPE_MATH(cosh, LPTX_type, d)
LPTX_DEFINE_TYPE_MATH(asin, LPTX_type, d)
LPTX_DEFINE_TYPE_MATH(asinh, LPTX_type, d)
LPTX_DEFINE_TYPE_MATH(sin, LPTX_type, d)
LPTX_DEFINE_TYPE_MATH(sinh, LPTX_type, d)
LPTX_DEFINE_TYPE_MATH(atan, LPTX_type, d)
LPTX_DEFINE_TYPE_MATH(atanh, LPTX_type, d)
LPTX_DEFINE_TYPE_MATH(tan, LPTX_type, d)
LPTX_DEFINE_TYPE_MATH(tanh, LPTX_type, d)
LPTX_DEFINE_TYPE_MATH(atan2, LPTX_type, dd)

LPTX_DEFINE_TYPE_MATH(sqrt, LPTX_type, d)
LPTX_DEFINE_TYPE_MATH(cbrt, LPTX_type, d)
LPTX_DEFINE_TYPE_MATH(exp, LPTX_type, d)
LPTX_DEFINE_TYPE_MATH(exp2, LPTX_type, d)
LPTX_DEFINE_TYPE_MATH(pow, LPTX_type, dd)
LPTX_DEFINE_TYPE_MATH(log, LPTX_type, d)
LPTX_DEFINE_TYPE_MATH(log2, LPTX_type, d)
LPTX_DEFINE_TYPE_MATH(log10, LPTX_type, d)

LPTX_DEFINE_TYPE_MATH(ceil, LPTX_type, d)
LPTX_DEFINE_TYPE_MATH(round, LPTX_type, d)
LPTX_DEFINE_TYPE_MATH(floor, LPTX_type, d)

LPTX_DEFINE_TYPE_MATH(remainder, LPTX_type, dd)
LPTX_DEFINE_TYPE_MATH(remquo, LPTX_type, ddp)
LPTX_DEFINE_TYPE_MATH(fma, LPTX_type, ddd)

LPTX_DEFINE_TYPE_MATH(copysign, LPTX_type, dd)
LPTX_DEFINE_TYPE_MATH(nextafter, LPTX_type, dd)
LPTX_DEFINE_TYPE_MATH(nexttoward, LPTX_type, dd)
LPTX_DEFINE_TYPE_MATH(nan, LPTX_type, c)

LPTX_DEFINE_TYPE_MATH(modf, LPTX_type, dr)
LPTX_DEFINE_TYPE_MATH(frexp, LPTX_type, dp)

JUPITER_LPTX_DECL_END

#endif
