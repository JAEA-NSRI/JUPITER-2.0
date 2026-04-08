#include "priv_util.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef JUPITER_LPTX_MPI
#include <mpi.h>
#endif

int LPTX_aaprintf(LPTX_asprintf_alloc *allocator, void *arg, const char *fmt,
                  ...)
{
  int ret;
  va_list ap;
  va_start(ap, fmt);
  ret = LPTX_vaaprintf(allocator, arg, fmt, ap);
  va_end(ap);
  return ret;
}

#ifdef HAVE_C99SNPRINTF
#undef HAVE_C99SNPRINTF
#endif
#if (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L) ||      \
  (defined(_XOPEN_SOURCE) && _XOPEN_SOURCE >= 500) ||                  \
  (defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 200112L) ||          \
  defined(_ISOC99_SOURCE) || defined(_BSD_SOURCE) || defined(_UCRT) || \
  (defined(_MSC_VER) && _MSC_VER >= 1900)
#define HAVE_C99SNPRINTF
#endif

int LPTX_vaaprintf(LPTX_asprintf_alloc *allocator, void *arg, const char *fmt,
                   va_list ap)
{
#ifdef HAVE_C99SNPRINTF
  /* C99 standard */
  va_list c;
  char *p;
  int n;

  p = NULL;
  va_copy(c, ap);
  n = vsnprintf(p, 0, fmt, c);
  va_end(c);
  if (n < 0)
    return n;

  p = allocator(n, arg);
  if (!p)
    return -1;

  return vsnprintf(p, n, fmt, ap);
#else
#error Cannot implement LPTX_vaaprintf for this compiler
#endif
}

int LPTX_asprintf(char **buf, const char *fmt, ...)
{
  int ret;
  va_list ap;
  va_start(ap, fmt);
  ret = LPTX_vasprintf(buf, fmt, ap);
  va_end(ap);
  return ret;
}

static char *LPTX_vasprintf_alloc(int len, void *buf)
{
  char *p;
  p = calloc((size_t)len + 1, sizeof(char));
  if (!p)
    return NULL;

  *(char **)buf = p;
  return p;
}

int LPTX_vasprintf(char **buf, const char *fmt, va_list ap)
{
#if defined(HAVE_VASPRINTF)
  (void)LPTX_vasprintf_alloc;
  return vasprintf(buf, fmt, ap);
#else
  if (!buf)
    return -1;
  return LPTX_vaaprintf(LPTX_vasprintf_alloc, buf, fmt, ap);
#endif
}
