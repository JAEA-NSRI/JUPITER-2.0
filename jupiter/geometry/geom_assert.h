#ifndef JUPITER_GEOMETRY_GEOM_ASSERT_H
#define JUPITER_GEOMETRY_GEOM_ASSERT_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __GNUC__
#define GEOM_EXPECT(cond) __builtin_expect(!!(cond), 1)
#else
#define GEOM_EXPECT(cond) ((!!(cond)) == 1)
#endif

static inline void geom_assert_impl(const char *file, long line,
                                    const char *func, const char *cond,
                                    const char *format, ...)
{
  va_list ap;
  va_start(ap, format);

#ifdef _OPENMP
#pragma omp critical
#endif
  {
    fprintf(stderr, "Assertion failed%s%s%s",
            cond ? ": " : "", cond ? cond : "", cond ? ", " : "");
    fprintf(stderr, "%s at %s(%ld)%s", func, file, line, format ? ": " : "");
    if (format) {
      vfprintf(stderr, format, ap);
    }
    fprintf(stderr, "\n");
    fflush(stderr);
  }

  va_end(ap);

#if defined(__GNUC__) && !(defined(__PGI) || defined(__NVCOMPILER))
  __builtin_trap();
#endif
  abort();
}

#ifndef NDEBUG
#define GEOM_ASSERT_X(cond, ...)                                          \
  do {                                                                    \
    if (!GEOM_EXPECT(cond))                                               \
      geom_assert_impl(__FILE__, __LINE__, __func__, #cond, __VA_ARGS__); \
  } while (0)

#define GEOM_UNREACHABLE() \
  geom_assert_impl(__FILE__, __LINE__, __func__, NULL, "Unreachable reached");

#else
#define GEOM_ASSERT_X(cond, ...) ((void)0)

#ifdef __GNUC__
#define GEOM_UNREACHABLE() __builtin_unreachable()
#else
#define GEOM_UNREACHABLE()
#endif
#endif

#define GEOM_ASSERT(cond) GEOM_ASSERT_X(cond, NULL)

#ifdef __cplusplus
}
#endif

#endif
