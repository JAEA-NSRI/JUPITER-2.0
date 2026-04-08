#ifndef JUPITER_CONTROL_OVERFLOW_H
#define JUPITER_CONTROL_OVERFLOW_H

#include "defs.h"
#include "error.h"

#include <stdint.h>
#include <limits.h>

JUPITER_CONTROL_DECL_BEGIN

static inline intmax_t jcntrl__add_overflow(intmax_t a, intmax_t b,
                                            intmax_t min, intmax_t max, int *f)
{
  *f = 0;
  if (b < 0) {
    if (a < min - b)
      *f = 1;
  } else {
    if (a > max - b)
      *f = 1;
  }
  return a + b;
}

static inline intmax_t jcntrl__sub_overflow(intmax_t a, intmax_t b,
                                            intmax_t min, intmax_t max, int *f)
{
  *f = 0;
  if (b < 0) {
    if (a > max + b)
      *f = 1;
  } else {
    if (a < min + b)
      *f = 1;
  }
  return a - b;
}

static inline intmax_t jcntrl__mul_overflow(intmax_t a, intmax_t b,
                                            intmax_t min, intmax_t max, int *f)
{
  *f = 0;
  if (a == 0 || b == 0)
    return a * b;

  if ((a < 0 && b < 0) || (a > 0 && b > 0)) {
    if (a == min || b == min)
      *f = 1;

    if (b > 0) {
      if (a > max / b || a < min / b)
        *f = 1;
    } else {
      if (a > min / b || a < max / b)
        *f = 1;
    }
    return a * b;
  }

  if (a == 1 || b == 1)
    return a * b;

  if (a < 0) {
    if (a == min) {
      *f = 1;
      return a * b;
    }
    a = -a;
  } else if (b < 0) {
    if (b == min) {
      *f = 1;
      return a * b;
    }
    b = -b;
  }

  if (a > max / b || a < min / b)
    *f = 1;
  return -(a * b);
}

static inline int jcntrl_i_add_overflow(int a, int b, int *res)
{
#ifdef JCNTRL_HAVE_OVERFLOW
  return __builtin_add_overflow(a, b, res);
#else
  int f;
  *res = jcntrl__add_overflow(a, b, INT_MIN, INT_MAX, &f);
  return f;
#endif
}

static inline int jcntrl_s_add_overflow(jcntrl_size_type a, jcntrl_size_type b,
                                        jcntrl_size_type *res)
{
#ifdef JCNTRL_HAVE_OVERFLOW
  return __builtin_add_overflow(a, b, res);
#else
  int f;
  *res = jcntrl__add_overflow(a, b, JCNTRL_SIZE_MIN, JCNTRL_SIZE_MAX, &f);
  return f;
#endif
}

static inline int jcntrl_a_add_overflow(jcntrl_aint_type a, jcntrl_aint_type b,
                                        jcntrl_aint_type *res)
{
#ifdef JCNTRL_HAVE_OVERFLOW
  return __builtin_add_overflow(a, b, res);
#else
  int f;
  *res = jcntrl__add_overflow(a, b, INTPTR_MIN, INTPTR_MAX, &f);
  return f;
#endif
}

static inline int jcntrl_m_add_overflow(intmax_t a, intmax_t b, intmax_t *res)
{
#ifdef JCNTRL_HAVE_OVERFLOW
  return __builtin_add_overflow(a, b, res);
#else
  int f;
  *res = jcntrl__add_overflow(a, b, INTMAX_MIN, INTMAX_MAX, &f);
  return f;
#endif
}

static inline int jcntrl_i_sub_overflow(int a, int b, int *res)
{
#ifdef JCNTRL_HAVE_OVERFLOW
  return __builtin_sub_overflow(a, b, res);
#else
  int f;
  *res = jcntrl__sub_overflow(a, b, INT_MIN, INT_MAX, &f);
  return f;
#endif
}

static inline int jcntrl_s_sub_overflow(jcntrl_size_type a, jcntrl_size_type b,
                                        jcntrl_size_type *res)
{
#ifdef JCNTRL_HAVE_OVERFLOW
  return __builtin_sub_overflow(a, b, res);
#else
  int f;
  *res = jcntrl__sub_overflow(a, b, JCNTRL_SIZE_MIN, JCNTRL_SIZE_MAX, &f);
  return f;
#endif
}

static inline int jcntrl_a_sub_overflow(jcntrl_aint_type a, jcntrl_aint_type b,
                                        jcntrl_aint_type *res)
{
#ifdef JCNTRL_HAVE_OVERFLOW
  return __builtin_sub_overflow(a, b, res);
#else
  int f;
  *res = jcntrl__sub_overflow(a, b, INTPTR_MIN, INTPTR_MAX, &f);
  return f;
#endif
}

static inline int jcntrl_m_sub_overflow(intmax_t a, intmax_t b, intmax_t *res)
{
#ifdef JCNTRL_HAVE_OVERFLOW
  return __builtin_sub_overflow(a, b, res);
#else
  int f;
  *res = jcntrl__sub_overflow(a, b, INTMAX_MIN, INTMAX_MAX, &f);
  return f;
#endif
}

static inline int jcntrl_i_mul_overflow(int a, int b, int *res)
{
#ifdef JCNTRL_HAVE_OVERFLOW
  return __builtin_mul_overflow(a, b, res);
#else
  int f;
  *res = jcntrl__mul_overflow(a, b, INT_MIN, INT_MAX, &f);
  return f;
#endif
}

static inline int jcntrl_s_mul_overflow(jcntrl_size_type a, jcntrl_size_type b,
                                        jcntrl_size_type *res)
{
#ifdef JCNTRL_HAVE_OVERFLOW
  return __builtin_mul_overflow(a, b, res);
#else
  int f;
  *res = jcntrl__mul_overflow(a, b, JCNTRL_SIZE_MIN, JCNTRL_SIZE_MAX, &f);
  return f;
#endif
}

static inline int jcntrl_a_mul_overflow(jcntrl_aint_type a, jcntrl_aint_type b,
                                        jcntrl_aint_type *res)
{
#ifdef JCNTRL_HAVE_OVERFLOW
  return __builtin_mul_overflow(a, b, res);
#else
  int f;
  *res = jcntrl__mul_overflow(a, b, INTPTR_MIN, INTPTR_MAX, &f);
  return f;
#endif
}

static inline int jcntrl_m_mul_overflow(intmax_t a, intmax_t b, intmax_t *res)
{
#ifdef JCNTRL_HAVE_OVERFLOW
  return __builtin_mul_overflow(a, b, res);
#else
  int f;
  *res = jcntrl__mul_overflow(a, b, INTMAX_MIN, INTMAX_MAX, &f);
  return f;
#endif
}

#define jcntrl__overflow_szchk(rtype, v)                                \
  (((int[(sizeof(v + (signed char)0) <= sizeof(rtype)) ? 1 : -1]){0})[0] == 0)

#define jcntrl__overflow_macro_f(t, rtype, op, x, y, res, file, line)         \
  (((void)sizeof(jcntrl__overflow_szchk(rtype, x) &&                          \
                 jcntrl__overflow_szchk(rtype, y) && (rtype *)NULL == (res)), \
    jcntrl_##t##_##op##_overflow(x, y, res)) &&                               \
   (jcntrl_raise_overflow_error(file, line), 1))

#define jcntrl__overflow_macro(t, rtype, op, x, y, res) \
  jcntrl__overflow_macro_f(t, rtype, op, x, y, res, __FILE__, __LINE__)

#define jcntrl_i_add_overflow(x, y, res) \
  jcntrl__overflow_macro(i, int, add, x, y, res)

#define jcntrl_s_add_overflow(x, y, res) \
  jcntrl__overflow_macro(s, jcntrl_size_type, add, x, y, res)

#define jcntrl_a_add_overflow(x, y, res) \
  jcntrl__overflow_macro(a, jcntrl_aint_type, add, x, y, res)

#define jcntrl_m_add_overflow(x, y, res) \
  jcntrl__overflow_macro(m, intmax_t, add, x, y, res)

#define jcntrl_i_sub_overflow(x, y, res) \
  jcntrl__overflow_macro(i, int, sub, x, y, res)

#define jcntrl_s_sub_overflow(x, y, res) \
  jcntrl__overflow_macro(s, jcntrl_size_type, sub, x, y, res)

#define jcntrl_a_sub_overflow(x, y, res) \
  jcntrl__overflow_macro(a, jcntrl_aint_type, sub, x, y, res)

#define jcntrl_m_sub_overflow(x, y, res) \
  jcntrl__overflow_macro(m, intmax_t, sub, x, y, res)

#define jcntrl_i_mul_overflow(x, y, res) \
  jcntrl__overflow_macro(i, int, mul, x, y, res)

#define jcntrl_s_mul_overflow(x, y, res) \
  jcntrl__overflow_macro(s, jcntrl_size_type, mul, x, y, res)

#define jcntrl_a_mul_overflow(x, y, res) \
  jcntrl__overflow_macro(a, jcntrl_aint_type, mul, x, y, res)

#define jcntrl_m_mul_overflow(x, y, res) \
  jcntrl__overflow_macro(m, intmax_t, mul, x, y, res)

JUPITER_CONTROL_DECL_END

#endif
