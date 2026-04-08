#ifndef JUPITER_CONTROL_OVERFLOW_H
#define JUPITER_CONTROL_OVERFLOW_H

#include "defs.h"
#include "error.h"

#include <limits.h>
#include <stdint.h>

JUPITER_LPTX_DECL_START

static inline intmax_t LPTX__add_overflow(intmax_t a, intmax_t b, intmax_t min,
                                          intmax_t max, int *f)
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

static inline intmax_t LPTX__sub_overflow(intmax_t a, intmax_t b, intmax_t min,
                                          intmax_t max, int *f)
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

static inline intmax_t LPTX__mul_overflow(intmax_t a, intmax_t b, intmax_t min,
                                          intmax_t max, int *f)
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

static inline int LPTX_i_add_overflow(int a, int b, int *res)
{
#ifdef JUPITER_LPTX_HAVE_OVERFLOW
  return __builtin_add_overflow(a, b, res);
#else
  int f;
  *res = LPTX__add_overflow(a, b, INT_MIN, INT_MAX, &f);
  return f;
#endif
}

static inline int LPTX_s_add_overflow(LPTX_idtype a, LPTX_idtype b,
                                      LPTX_idtype *res)
{
#ifdef JUPITER_LPTX_HAVE_OVERFLOW
  return __builtin_add_overflow(a, b, res);
#else
  int f;
  *res = LPTX__add_overflow(a, b, LPTX_IDTYPE_MIN, LPTX_IDTYPE_MAX, &f);
  return f;
#endif
}

static inline int LPTX_m_add_overflow(intmax_t a, intmax_t b, intmax_t *res)
{
#ifdef JUPITER_LPTX_HAVE_OVERFLOW
  return __builtin_add_overflow(a, b, res);
#else
  int f;
  *res = LPTX__add_overflow(a, b, INTMAX_MIN, INTMAX_MAX, &f);
  return f;
#endif
}

static inline int LPTX_i_sub_overflow(int a, int b, int *res)
{
#ifdef JUPITER_LPTX_HAVE_OVERFLOW
  return __builtin_sub_overflow(a, b, res);
#else
  int f;
  *res = LPTX__sub_overflow(a, b, INT_MIN, INT_MAX, &f);
  return f;
#endif
}

static inline int LPTX_s_sub_overflow(LPTX_idtype a, LPTX_idtype b,
                                      LPTX_idtype *res)
{
#ifdef JUPITER_LPTX_HAVE_OVERFLOW
  return __builtin_sub_overflow(a, b, res);
#else
  int f;
  *res = LPTX__sub_overflow(a, b, LPTX_IDTYPE_MIN, LPTX_IDTYPE_MAX, &f);
  return f;
#endif
}

static inline int LPTX_m_sub_overflow(intmax_t a, intmax_t b, intmax_t *res)
{
#ifdef JUPITER_LPTX_HAVE_OVERFLOW
  return __builtin_sub_overflow(a, b, res);
#else
  int f;
  *res = LPTX__sub_overflow(a, b, INTMAX_MIN, INTMAX_MAX, &f);
  return f;
#endif
}

static inline int LPTX_i_mul_overflow(int a, int b, int *res)
{
#ifdef JUPITER_LPTX_HAVE_OVERFLOW
  return __builtin_mul_overflow(a, b, res);
#else
  int f;
  *res = LPTX__mul_overflow(a, b, INT_MIN, INT_MAX, &f);
  return f;
#endif
}

static inline int LPTX_s_mul_overflow(LPTX_idtype a, LPTX_idtype b,
                                      LPTX_idtype *res)
{
#ifdef JUPITER_LPTX_HAVE_OVERFLOW
  return __builtin_mul_overflow(a, b, res);
#else
  int f;
  *res = LPTX__mul_overflow(a, b, LPTX_IDTYPE_MIN, LPTX_IDTYPE_MAX, &f);
  return f;
#endif
}

static inline int LPTX_m_mul_overflow(intmax_t a, intmax_t b, intmax_t *res)
{
#ifdef JUPITER_LPTX_HAVE_OVERFLOW
  return __builtin_mul_overflow(a, b, res);
#else
  int f;
  *res = LPTX__mul_overflow(a, b, INTMAX_MIN, INTMAX_MAX, &f);
  return f;
#endif
}

static inline void LPTX_raise_overflow_error(const char *file, long line)
{
  LPTX__assert_impl(0, "(overflow)", file, line, "Overflow error occured");
}

#define LPTX__overflow_szchk(rtype, v) \
  (((int[(sizeof(v + (signed char)0) <= sizeof(rtype)) ? 1 : -1]){0})[0] == 0)

#define LPTX__overflow_macro_f(t, rtype, op, x, y, res, file, line)         \
  (((void)sizeof(LPTX__overflow_szchk(rtype, x) &&                          \
                 LPTX__overflow_szchk(rtype, y) && (rtype *)NULL == (res)), \
    LPTX_##t##_##op##_overflow(x, y, res)) &&                               \
   (LPTX_raise_overflow_error(file, line), 1))

#define LPTX__overflow_macro(t, rtype, op, x, y, res) \
  LPTX__overflow_macro_f(t, rtype, op, x, y, res, __FILE__, __LINE__)

#define LPTX_i_add_overflow(x, y, res) \
  LPTX__overflow_macro(i, int, add, x, y, res)

#define LPTX_s_add_overflow(x, y, res) \
  LPTX__overflow_macro(s, LPTX_idtype, add, x, y, res)

#define LPTX_a_add_overflow(x, y, res) \
  LPTX__overflow_macro(a, LPTX_aint_type, add, x, y, res)

#define LPTX_m_add_overflow(x, y, res) \
  LPTX__overflow_macro(m, intmax_t, add, x, y, res)

#define LPTX_i_sub_overflow(x, y, res) \
  LPTX__overflow_macro(i, int, sub, x, y, res)

#define LPTX_s_sub_overflow(x, y, res) \
  LPTX__overflow_macro(s, LPTX_idtype, sub, x, y, res)

#define LPTX_a_sub_overflow(x, y, res) \
  LPTX__overflow_macro(a, LPTX_aint_type, sub, x, y, res)

#define LPTX_m_sub_overflow(x, y, res) \
  LPTX__overflow_macro(m, intmax_t, sub, x, y, res)

#define LPTX_i_mul_overflow(x, y, res) \
  LPTX__overflow_macro(i, int, mul, x, y, res)

#define LPTX_s_mul_overflow(x, y, res) \
  LPTX__overflow_macro(s, LPTX_idtype, mul, x, y, res)

#define LPTX_a_mul_overflow(x, y, res) \
  LPTX__overflow_macro(a, LPTX_aint_type, mul, x, y, res)

#define LPTX_m_mul_overflow(x, y, res) \
  LPTX__overflow_macro(m, intmax_t, mul, x, y, res)

JUPITER_LPTX_DECL_END

#endif
