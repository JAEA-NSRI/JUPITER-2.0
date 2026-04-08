/* This is -*- c -*- source. */
/* This is vim: set ft=c: source. */

/**
 * @addtogroup doxygen_filter
 * @{
 * @file doxygen-filter.re
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <errno.h>
#include <assert.h>
#include <inttypes.h>

#include "doxygen-filter.h"

#include <jupiter/re2c_lparser/re2c_lparser.h>

/*!re2c
  re2c:indent:string = "  ";
  re2c:flags:8 = 1; // parse input as UTF-8.
*/

/**
 * @brief current program name
 */
static const char *prog_name = DEFAULT_PROG_NAME;

void set_prog_name(const char *argv0)
{
  const char *cur;
  const char *tok;

  cur = argv0;
  tok = cur;
  for (;;) {
    /*!re2c
      re2c:define:YYCTYPE = "unsigned char";
      re2c:define:YYCURSOR = "cur";
      re2c:define:YYMARKER = "mrk";
      re2c:indent:top = 2;
      re2c:yyfill:enable = 0;

      *      { continue; }
      '\x00' { break; }
      '/'    { tok = cur; continue; }
      '\\'   { goto win32_bkslash; }
    */
    assert(0 && "Unreachable reached");

  win32_bkslash:
#ifdef WIN32
    tok = cur;
#endif
    continue;
  }
  if (strcmp(tok, "") == 0) {
    prog_name = DEFAULT_PROG_NAME;
  } else {
    prog_name = tok;
  }
}

void print_error(const char *fmt, ...)
{
  char *buf;
  int r;
  va_list ap, aq;

  va_start(ap, fmt);
  va_copy(aq, ap);
  r = doxfilter_vasprintf(&buf, fmt, ap);
  va_end(ap);
  if (r < 0) {
    fprintf(stderr, "%s: error: ", prog_name);
    vfprintf(stderr, fmt, aq);
    fprintf(stderr, "\n");
    fprintf(stderr, "%s: note: allocation failed while writing error\n", prog_name);
  } else {
    int n;
    n = strlen(buf);
    if (n > 0 && !(n == 1 && buf[0] == '\n')) {
      fprintf(stderr, "%s: error: %s%s", prog_name,
              buf, (buf[n - 1] == '\n') ? "" : "\n");
    }
    free(buf);
  }
  va_end(aq);
}

int doxfilter_vasprintf(char **t, const char *fmt, va_list ap)
{
#if defined(_GNU_SOURCE)
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
  (defined(_MSC_VER) && _MSC_VER >= 1900)   /* Visual Studio 2015 */
  /*
   * C99 supported platforms, POSIX 2001.12 and VS2015 or later.
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
  if (n < 0) return n;
  p = (char*)malloc(sizeof(char) * (n + 1));
  if (!p) return -1;
  n = vsnprintf(p, n + 1, fmt, c);
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

int doxfilter_asprintf(char **t, const char *fmt, ...)
{
  va_list ap;
  int r;
  va_start(ap, fmt);
  r = doxfilter_vasprintf(t, fmt, ap);
  va_end(ap);
  return r;
}

/** @} */
