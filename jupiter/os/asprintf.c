#include "asprintf.h"

#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

int jupiter_vasprintf(char **t, const char *fmt, va_list ap)
{
#if defined(_GNU_SOURCE) || defined(HAVE_VASPRINTF)
  /*
   * for glibc
   *
   * NOTE: The compiler won't define _GNU_SOURCE. Please define by hand.
   *
   * Known available systems:
   *   - glibc (most of Linux)
   *   - uClibc (some kind of Linux)
   *   - musl (some kind of Linux)
   *   - BSD libc (includes OS X and macOS)
   *   - newlib (includes Cygwin and MSYS2)
   *   - MinGW
   */
  return vasprintf(t, fmt, ap);

#elif defined(_BSD_SOURCE) ||                                           \
  (defined(_XOPEN_SOURCE) && _XOPEN_SOURCE >= 500) ||                   \
  defined(_ISOC99_SOURCE) ||                                            \
  (defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 200112L) ||           \
  defined(_UCRT) ||                                                     \
  (defined(_MSC_VER) && _MSC_VER >= 1900)   /* Visual Studio 2015 */
  /*
   * C99 supported platforms, POSIX 2001.12, Windows Universal CRT and
   * VS2015 or later.
   *
   * [CAUTION]: Some (v)snprintf implementations behave in different way
   *            from specified by C99, with ***same prototype***.
   */

  va_list c;
  char *p;
  int n;
  if (!t) return -1;
  va_copy(c, ap);
  p = NULL;
  n = vsnprintf(p, 0, fmt, ap);
  if (n < 0) {
    va_end(c);
    return n;
  }
  p = (char*)malloc(sizeof(char) * (n + 1));
  if (!p) {
    va_end(c);
    return -1;
  }
  n = vsnprintf(p, n + 1, fmt, c);
  va_end(c);
  *t = p;
  return n;

#elif defined(_MSC_VER)
  /*
   * Old Visual Studio
   */

  int r, len;
  size_t size;
  char *str;
  /* _vscprintf tells you how big the buffer needs to be */
  len = _vscprintf(fmt, ap);
  if (len == -1) {
    return -1;
  }
  size = (size_t)len + 1;
  str = malloc(size);
  if (!str) {
    return -1;
  }
  /* _vsprintf_s is the "secure" version of vsprintf */
  r = _vsprintf_s(str, len + 1, fmt, ap);
  if (r == -1) {
    free(str);
    return -1;
  }
  *t = str;
  return r;

#else
#   error "Could not determine how to format a text in a safe way."
#endif
}

int jupiter_asprintf(char **t, const char *fmt, ...)
{
  int n;
  va_list ap;
  va_start(ap, fmt);
  n = jupiter_vasprintf(t, fmt, ap);
  va_end(ap);
  return n;
}

char *jupiter_strdup(const char *s)
{
  size_t n;
  char *r;
  n = strlen(s);
  r = (char *)malloc(sizeof(char) * (n + 1));
  if (!r) return NULL;
  memcpy(r, s, n + 1);
  return r;
}

char *jupiter_strndup(const char *s, size_t n)
{
  char *r;
  if (n == SIZE_MAX)
    return NULL;
  r = (char *)malloc(sizeof(char) * (n + 1));
  if (!r)
    return NULL;
  strncpy(r, s, n);
  r[n] = '\0';
  return r;
}
