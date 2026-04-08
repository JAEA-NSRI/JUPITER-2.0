
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "geom_assert.h"
#include "common.h"

int geom_asprintf(char **buf, const char *format, ...)
{
  va_list ap;
  int r;
  va_start(ap, format);
  r = geom_vasprintf(buf, format, ap);
  va_end(ap);
  return r;
}

int geom_vasprintf(char **buf, const char *format, va_list ap)
{
#if defined(HAVE_VASPRINTF) || defined(_GNU_SOURCE)
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
  return vasprintf(buf, format, ap);

#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L || \
  defined(_BSD_SOURCE) || defined(_ISOC99_SOURCE) ||              \
  defined(_XOPEN_SOURCE) && _XOPEN_SOURCE >= 500 ||               \
  defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 200112L
  /*
   * C99 supported platforms, POSIX 2001.12 and VS2015 or later.
   *
   * [CAUTION]: Some (v)snprintf implementations behave in different way
   *            from specified by C99, with ***same prototype***.
   */
  va_list c;
  char *p;
  int n;
  if (!buf) return -1;
  va_copy(c, ap);
  p = NULL;
  n = vsnprintf(p, 0, format, ap);
  if (n < 0) return n;
  p = (char*)malloc(sizeof(char) * (n + 1));
  if (!p) return -1;
  n = vsnprintf(p, n + 1, format, c);
  *buf = p;
  return n;

#elif defined(_MSC_VER)
  /*
   * Old Visual Studio
   */

  int r, len;
  size_t size;
  char *str;
  /* _vscprintf tells you how big the buffer needs to be */
  len = _vscprintf(format, ap);
  if (len == -1) {
    return -1;
  }
  size = (size_t)len + 1;
  str = malloc(size);
  if (!str) {
    return -1;
  }
  /* _vsprintf_s is the "secure" version of vsprintf */
  r = _vsprintf_s(str, len + 1, format, ap);
  if (r == -1) {
    free(str);
    return -1;
  }
  *buf = str;
  return r;

#else
#error "Could not determine how to format a text in a safe way"
#endif
}

int geom_double_to_str(char **buf, double val, const char *flags,
                       int width, int precision, char fmt)
{
  char *t;
  int r;

  switch(fmt) {
  case 'a': case 'A':
  case 'e': case 'E':
  case 'f': case 'F':
  case 'g': case 'G':
    break;
  default:
    return -1;
  }

  r = geom_asprintf(&t, "%%%s%c", flags ? flags : "", fmt);
  if (r < 0) return r;
  if (width >= 0) {
    if (precision >= 0) {
      r = geom_asprintf(buf, t, width, precision, val);
    } else {
      r = geom_asprintf(buf, t, width, val);
    }
  } else {
    if (precision >= 0) {
      r = geom_asprintf(buf, t, precision, val);
    } else {
      r = geom_asprintf(buf, t, val);
    }
  }
  free(t);
  return r;
}

static
int geom_vector_parse_fmt(const char *fmt,
                          const char **flgs, const char **flge,
                          int *wid, int *prec, char *fmtc)
{
  int inp;
  int end;
  char ch;
  const char *st;
  GEOM_ASSERT(fmt);
  GEOM_ASSERT(flgs);
  GEOM_ASSERT(flge);
  GEOM_ASSERT(wid);
  GEOM_ASSERT(prec);

  inp = 0;
  end = 0;
  *wid = 0;
  *prec = 0;
  *flgs = NULL;
  *flge = NULL;
  while ((ch = *fmt++) != '\0') {
    switch(ch) {
    case '%':
      if (inp) {
        /* some charaters present between two '%'s */
        if (fmt - st > 1) return 1;
        if (!end) {
          *flgs = NULL;
          *flge = NULL;
        }
      }
      st = NULL;
      inp = !inp;
      if (inp) {
        st = fmt;
        if (!end) {
          *flgs = fmt;
        }
      }
      break;
    case '*':
      if (inp) {
        if (end) return 1;
        if (*wid) {
          *prec = 1;
        } else {
          *wid = 1;
        }
      }
      break;
    case 'a': case 'A': case 'e': case 'E':
    case 'f': case 'F': case 'g': case 'G':
      if (inp) {
        if (end) return 1;
        inp = 0;
        if (!*flge) *flge = fmt - 1;
        end = 1;
        *fmtc = ch;
      }
      break;
    case '.':
      if (inp) {
        if (end) return 1;
        if (!*wid) *wid = -1;
      }
      break;
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      if (inp) {
        if (end) return 1;
      }
      break;
    case '#': case '-': case '+': case ' ':
      if (inp) { /* other acceptable chars in conversion */
        /* if end is flagged, too many conversions present */
        if (end) return 1;
      }
      break;
    default:
      if (inp) { /* invalid or not supported chars in conversion */
        return 1;
      }
      break;
    }
  }
  return 0;
}

static int
geom_vector_get_args(const char *fmt, char **flg, int *wid, int *prec,
                     char *fmtc)
{
  int r;
  const char *flgs, *flge;

  r = geom_vector_parse_fmt(fmt, &flgs, &flge, wid, prec, fmtc);
  if (r) return -1;

  if (flgs && flge && flge >= flgs) {
    ptrdiff_t n;
    n = flge - flgs + 1;
    *flg = (char *)calloc(sizeof(char), n);
    if (*flg) {
      strncpy(*flg, flgs, n - 1);
    }
  } else {
    *flg = NULL;
  }
  return 0;
}

int geom_matrix_to_str(char *buf[], const double *vals, int nx, int ny,
                       const char *sep, const char *parst, const char *pared,
                       int align, const char *fmt, int width, int precision)
{
  int w;
  int p;
  int r;
  char *flg;
  char **bufsub;
  char *tmp;
  char fmtc;
  size_t sz;
  int ali;
  int i;
  int j;

  GEOM_ASSERT(buf);
  GEOM_ASSERT(vals);
  GEOM_ASSERT(nx >= 0);
  GEOM_ASSERT(ny >  0);

  if (!sep) sep = ", ";
  if (!parst) parst = "(";
  if (!pared) pared = ")";

  if (nx == 0) {
    r = geom_asprintf(buf, "%s%s", parst, pared);
    if (r < 0) return r;
    for (i = 1; i < ny; ++i) {
      buf[i] = buf[0];
    }
    return r;
  }

  bufsub = (char **)calloc(sizeof(char *), ny);
  if (!bufsub) return -1;

  r = geom_vector_get_args(fmt, &flg, &w, &p, &fmtc);
  if (r < 0) {
    free(bufsub);
    return r;
  }

  if (w > 0) w = width;
  if (p > 0) p = precision;

  for (j = 0; j < ny; ++j) {
    r = geom_asprintf(&buf[j], "%s", parst);
    if (r < 0) {
      for (j--; j >= 0; j--) {
        free(buf[j]);
      }
      free(flg);
      free(bufsub);
      return r;
    }
  }
  for (i = 0; i < nx; ++i) {
    ali = -1;
    for (j = 0; j < ny; ++j) {
      r = geom_double_to_str(&bufsub[j], vals[j * nx + i], flg, w, p, fmtc);
      if (r < 0) goto clean;
      if (align) {
        r = strlen(bufsub[j]);
        if (r > ali) {
          ali = r;
        }
      }
    }
    for (j = 0; j < ny; ++j) {
      r = geom_asprintf(&tmp, "%s%*s%s", buf[j], ali, bufsub[j],
                        (i != nx - 1) ? sep : pared);
      if (r < 0) {
        j = ny;
        goto clean;
      }
      free(bufsub[j]);
      free(buf[j]);
      buf[j] = tmp;
    }
  }
  free(bufsub);
  free(flg);

  sz = 0;
  for (j = 0; j < ny; ++j) {
    size_t t;
    t = strlen(buf[j]) + 1;
    if (sz > INT_MAX - t) {
      j = ny;
      for (j = 0; j < ny; ++j) {
        free(buf[j]);
      }
      return -1;
    }
    sz += t;
  }

  tmp = (char *)realloc(buf[0], sz);
  j = 0;
  if (tmp && sz > 0) {
    size_t n;
    buf[0] = tmp;
    n = strlen(tmp) + 1;
    for (j = 1; j < ny; ++j) {
      tmp += n;
      n = strlen(buf[j]) + 1;
      strcpy(tmp, buf[j]);
      free(buf[j]);
      buf[j] = tmp;
    }
    r = n;
  } else {
    if (tmp && tmp != buf[0]) {
      free(tmp);
      buf[0] = NULL;
    }
    for (j = 0; j < ny; ++j) {
      free(buf[j]);
    }
    r = -1;
  }

  return r;

clean:
  for (j--; j >= 0; j--) {
    free(bufsub[j]);
  }
  for (j = 0; j < ny; ++j) {
    free(buf[j]);
  }
  free(bufsub);
  free(flg);
  return r;
}
