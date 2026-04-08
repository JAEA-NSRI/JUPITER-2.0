
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "defs.h"
#include "error.h"
#include "information.h"

void jcntrl_assert_impl(const char *file, long line, const char *func,
                        const char *cond, const char *format, ...)
{
  va_list ap;
  va_start(ap, format);

#ifdef JCNTRL_USE_OPENMP
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
#else
  abort();
#endif
}
