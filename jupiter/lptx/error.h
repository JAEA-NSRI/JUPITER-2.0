#ifndef JUPITER_LPTX_ERROR_H
#define JUPITER_LPTX_ERROR_H

#include "defs.h"

JUPITER_LPTX_DECL_START

#if defined(__GNUC__) && (!defined(__PGI) || defined(__NVCOMPILER))
#define LPTX_trap() __builtin_trap()
#else
#define LPTX_trap() abort()
#endif

/**
 * Let compiler to optimize for the condition is `true` in most cases.
 */
#if defined(__GNUC__)
#define LPTX_expect(condition) __builtin_expect(!!(condition), 1)
#else
#define LPTX_expect(condition) (!!(condition) == 1)
#endif

#if defined(__GNUC__)
__attribute__((format(printf, 5, 6)))
#endif
JUPITER_LPTX_DECL void
LPTX__assert_impl(int cond, const char *cond_text, const char *file, long line,
                  const char *msg, ...);

#define LPTX_assert_Xf(cond, file, line, ...)                \
  do {                                                       \
    if (LPTX_expect(cond))                                   \
      break;                                                 \
    LPTX__assert_impl(cond, #cond, file, line, __VA_ARGS__); \
    LPTX_trap();                                             \
  } while (1)

#define LPTX_assert_X(cond, ...) \
  LPTX_assert_Xf(cond, __FILE__, __LINE__, __VA_ARGS__)

#define LPTX_assert(cond) LPTX_assert_X(cond, NULL)

JUPITER_LPTX_DECL_END

#endif
