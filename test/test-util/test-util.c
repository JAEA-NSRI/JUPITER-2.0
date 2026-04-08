
#include "test-util.h"

#include <limits.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <inttypes.h>
#include <ctype.h>

int test_compare_vasprintf(char **buf, const char *fmt, va_list ap)
{
  /* see csvutil.c in JUPITER */
#if defined(_GNU_SOURCE) || defined(HAVE_VASPRINTF)
  return vasprintf(buf, fmt, ap);
#elif (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L) ||     \
  defined(_BSD_SOURCE) || defined(_ISOC99_SOURCE) ||                    \
  (defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 200112L) ||           \
  (defined(_XOPEN_SOURCE) && _XOPEN_SOURCE >= 500) || defined(_UCRT) || \
  (defined(_MSC_VER) && _MSC_VER >= 1900)

  va_list c;
  char *p;
  int n;

  if (!buf)
    return -1;
  va_copy(c, ap);
  p = NULL;
  n = vsnprintf(p, 0, fmt, ap);
  if (n < 0)
    return n;
  p = (char *)malloc(sizeof(char) * (n + 1));
  if (!p)
    return -1;
  n = vsnprintf(p, n + 1, fmt, c);
  va_end(c);
  *buf = p;
  return n;
#else
#error Needs asprintf
#endif
}

int test_compare_asprintf(char **buf, const char *fmt, ...)
{
  va_list ap;
  int r;
  va_start(ap, fmt);
  r = test_compare_vasprintf(buf, fmt, ap);
  va_end(ap);
  return r;
}

static int test_compare_println(const char *head, const char *text, int len)
{
  if (len < 0)
    return fprintf(stderr, "%s%s\n", head, text);
  return fprintf(stderr, "%s%.*s\n", head, len, text);
}

static int test_compare_puts(const char *first_line, const char *cont_line,
                             const char *text)
{
  const char *beg;
  const char *ln;

  beg = text;
  ln = strchr(beg, '\n');
  for (; ln; beg = ln + 1, ln = strchr(beg, '\n')) {
    test_compare_println(first_line ? first_line : cont_line, beg, ln - beg);
    first_line = NULL;
  }
  if (*beg != '\0')
    test_compare_println(first_line ? first_line : cont_line, beg, -1);
  return 0;
}

int test_compare_vprintf(const char *first_line, const char *cont_line,
                         const char *fmt, va_list ap)
{
  int r;
  char *buf;
  r = test_compare_vasprintf(&buf, fmt, ap);
  if (r < 0)
    return r;
  r = test_compare_puts(first_line, cont_line, buf);
  free(buf);
  return r;
}

int test_compare_printf(const char *first_line, const char *cont_line,
                        const char *fmt, ...)
{
  int r;
  va_list ap;
  va_start(ap, fmt);
  r = test_compare_vprintf(first_line, cont_line, fmt, ap);
  va_end(ap);
  return r;
}

int test_compare_base(int stat, const char *file, long line,
                      const char *description, const char *fail_detail)
{
  test_compare_puts(stat ? "FAIL: " : "PASS: ", "..... ", description);
  if (stat) {
    if (fail_detail)
      test_compare_puts(NULL, "..... ", fail_detail);
    test_compare_printf(NULL, "..... ", "defined at %s(%ld)", file, line);
  }
  return stat;
}

int test_compare_fail(int stat, const char *file, long line,
                      const char *compare, const char *fmt, ...)
{
  int r;
  va_list ap;
  va_start(ap, fmt);
  r = test_compare_failv(stat, file, line, compare, fmt, ap);
  va_end(ap);
  return r;
}

int test_compare_failv(int stat, const char *file, long line,
                       const char *compare, const char *fmt, va_list ap)
{
  int r;
  char *detail = NULL;

  if (stat && fmt) {
    int r;
    r = test_compare_vasprintf(&detail, fmt, ap);
    if (r <= 0)
      detail = NULL;
  }

  r = test_compare_base(stat, file, line, compare, detail);
  if (detail)
    free(detail);
  return r;
}

int test_print_bytes(const void *data, ptrdiff_t len)
{
  const char *pp;
  unsigned char ch;
  intptr_t imax;
  int wid;
  int i;

  if (len < 0)
    return 1;

  if (!data) {
    fprintf(stderr, "..... data is (nil)\n");
    return 1;
  }

  pp = data;
  imax = (intptr_t)(pp + len);
  wid = 0;
  while (imax > 0) {
    imax >>= 4;
    wid++;
  }
  wid += 2;

  while (len >= 0) {
    fprintf(stderr, "..... %*p: ", wid, data);
    if (len == 0) {
      fprintf(stderr, "\n");
      break;
    }
    for (i = 0; i < 16; i += 2) {
      if (i + 1 < len) {
        fprintf(stderr, "%02x%02x ", (unsigned int)*(pp + i) & 0xff,
                (unsigned int)*(pp + i + 1) & 0xff);
      } else if (i < len) {
        fprintf(stderr, "%02x   ", (unsigned int)*(pp + i) & 0xff);
      } else {
        fprintf(stderr, "     ");
      }
    }
    for (i = 0; i < 16; i++) {
      if (i >= len)
        break;

      ch = *(pp + i);
      if (ch < 0x80 && isprint(ch)) {
        fprintf(stderr, "%c", (unsigned int)*(pp + i));
      } else {
        fprintf(stderr, ".");
      }
    }
    fprintf(stderr, "\n");
    len -= 16;
    pp += 16;
    data = pp;
  }
  return 1;
}

int test_is_little_endian(void)
{
  uint_least16_t u16f;

  u16f = 0x1234;
  if (memcmp(&u16f, (char[]){0x34, 0x12}, 2) == 0)
    return 1;
  return 0;
}

int test_compare_typed(void *got, void *exp, void *arg, const char *description,
                       const char *file, long line,
                       int (*cmp)(void *g, void *e, void *a),
                       int (*got_printer)(char **b, void *d, void *a),
                       int (*exp_printer)(char **b, void *d, void *a),
                       int (*ext_printer)(char **b, void *g, void *e, void *a))
{
  int r;
  char *failstr = NULL;
  int pass;
  pass = cmp(got, exp, arg);
  if (!pass) {
    char *gotstr, *expstr, *extstr;
    r = got_printer(&gotstr, got, arg);
    if (r < 0)
      gotstr = NULL;
    r = exp_printer(&expstr, exp, arg);
    if (r < 0)
      expstr = NULL;

    extstr = NULL;
    if (ext_printer) {
      r = ext_printer(&extstr, got, exp, arg);
      if (r < 0)
        extstr = NULL;
    }

    r =
      test_compare_asprintf(&failstr,
                            "       Expected: %s\n"
                            "            Got: %s\n"
                            "%s",
                            expstr ? expstr : "(error)",
                            gotstr ? gotstr : "(error)", extstr ? extstr : "");
    if (gotstr)
      free(gotstr);
    if (expstr)
      free(expstr);
    if (extstr)
      free(extstr);
  }

  test_compare_base(!pass, file, line, description, failstr);
  if (failstr)
    free(failstr);
  return pass;
}

int test_compare_ii_cmp(void *got, void *exp, void *arg)
{
  enum test_compare_op op = *(enum test_compare_op *)arg;
  intmax_t g = *(intmax_t *)got;
  intmax_t e = *(intmax_t *)exp;
  switch (op) {
  case test_compare_eq:
    return g == e;
  case test_compare_gt:
    return g > e;
  case test_compare_ge:
    return g >= e;
  case test_compare_le:
    return g <= e;
  case test_compare_lt:
    return g < e;
  case test_compare_ne:
    return g != e;
  }
  return 0;
}

int test_compare_iu_cmp(void *got, void *exp, void *arg)
{
  enum test_compare_op op = *(enum test_compare_op *)arg;
  intmax_t g = *(intmax_t *)got;
  uintmax_t e = *(uintmax_t *)exp;
  switch (op) {
  case test_compare_eq:
    if (g < 0)
      return 0;
    return g == e;
  case test_compare_gt:
    if (g < 0)
      return 0;
    return g > e;
  case test_compare_ge:
    if (g < 0)
      return 0;
    return g >= e;
  case test_compare_le:
    if (g < 0)
      return 1;
    return g <= e;
  case test_compare_lt:
    if (g < 0)
      return 1;
    return g < e;
  case test_compare_ne:
    if (g < 0)
      return 0;
    return g != e;
  }
  return 0;
}

int test_compare_ui_cmp(void *got, void *exp, void *arg)
{
  enum test_compare_op op = *(enum test_compare_op *)arg;
  uintmax_t g = *(uintmax_t *)got;
  intmax_t e = *(intmax_t *)exp;
  switch (op) {
  case test_compare_eq:
    if (e < 0)
      return 0;
    return g == e;
  case test_compare_gt:
    if (e < 0)
      return 1;
    return g > e;
  case test_compare_ge:
    if (e < 0)
      return 1;
    return g >= e;
  case test_compare_le:
    if (e < 0)
      return 0;
    return g <= e;
  case test_compare_lt:
    if (e < 0)
      return 0;
    return g < e;
  case test_compare_ne:
    if (e < 0)
      return 0;
    return g != e;
  }
  return 0;
}

int test_compare_uu_cmp(void *got, void *exp, void *arg)
{
  enum test_compare_op op = *(enum test_compare_op *)arg;
  uintmax_t g = *(uintmax_t *)got;
  uintmax_t e = *(uintmax_t *)exp;
  switch (op) {
  case test_compare_eq:
    return g == e;
  case test_compare_gt:
    return g > e;
  case test_compare_ge:
    return g >= e;
  case test_compare_le:
    return g <= e;
  case test_compare_lt:
    return g < e;
  case test_compare_ne:
    return g != e;
  }
  return 0;
}

int test_compare_dd_cmp(void *got, void *exp, void *arg)
{
  enum test_compare_op op = *(enum test_compare_op *)arg;
  double g = *(double *)got;
  double e = *(double *)exp;
  switch (op) {
  case test_compare_eq:
    return g == e;
  case test_compare_gt:
    return g > e;
  case test_compare_ge:
    return g >= e;
  case test_compare_le:
    return g <= e;
  case test_compare_lt:
    return g < e;
  case test_compare_ne:
    return g != e;
  }
  return 0;
}

int test_compare_eps_cmp(void *got, void *exp, void *arg)
{
  struct test_compare_eps_d *a = (struct test_compare_eps_d *)arg;
  double eps = a->eps;
  double g = *(double *)got;
  double e = *(double *)exp;
  a->delta = g - e;
  return fabs(a->delta) < eps;
}

int test_compare_eps_ext_prn(char **buf, void *g, void *e, void *a)
{
  int r;
  struct test_compare_eps_d *p = (struct test_compare_eps_d *)a;
  char *delstr, *epsstr;

  r = test_compare_d_prn(&delstr, &p->delta, NULL);
  if (r < 0)
    delstr = NULL;

  r = test_compare_d_prn(&epsstr, &p->eps, NULL);
  if (r < 0)
    epsstr = NULL;

  r = test_compare_asprintf(buf,
                            "          Delta: %s\n"
                            "            EPS: %s\n",
                            delstr ? delstr : "(error)",
                            epsstr ? epsstr : "(error)");
  if (delstr)
    free(delstr);
  if (epsstr)
    free(epsstr);
  return r;
}

int test_compare_ss_cmp(void *got, void *exp, void *arg)
{
  const char *g = *(const char **)got;
  const char *e = *(const char **)exp;
  return strcmp(g, e) == 0;
}

int test_compare_ssn_cmp(void *got, void *exp, void *arg)
{
  const char *g = *(const char **)got;
  const char *e = *(const char **)exp;
  size_t n = ((struct test_compare_ssn_d *)arg)->n;
  return strncmp(g, e, n) == 0;
}

int test_compare_i_prn(char **buf, void *d, void *a)
{
  return test_compare_asprintf(buf, "%" PRIdMAX, *(intmax_t *)d);
}

int test_compare_u_prn(char **buf, void *d, void *a)
{
  return test_compare_asprintf(buf, "%" PRIuMAX "u", *(uintmax_t *)d);
}

int test_compare_d_prn(char **buf, void *d, void *a)
{
  const char *postp = "";
  double ival, val = *(double *)d;
  if (isfinite(val) && modf(val, &ival) == 0.)
    postp = ".0";
#ifdef DBL_DIG
  return test_compare_asprintf(buf, "%.*g%s", DBL_DIG + 2, val, postp);
#else
  return test_compare_asprintf(buf, "%.17g%s", val, postp);
#endif
}

static int test_compare_s_prn_base(char **buf, const char *base, size_t nlim)
{
  char *ret;
  size_t n = 0, m;

  if (!base)
    return test_compare_asprintf(buf, "%s", base);

  for (int pass = 0; pass < 2; ++pass) {
    const char *p = base;
    char *wr;
    unsigned char ch;

    if (pass) {
      if (n == 0)
        return 0;
      ret = (char *)malloc(sizeof(char) * n);
      if (!ret)
        return -1;

      wr = ret;
      *wr++ = '"';
    } else {
      n += 1;
    }

    m = 0;
    while (m++ < nlim && (ch = *p++)) {
      char bkch;
      switch (ch) {
      case '\n':
        bkch = 'n';
        goto bkwr;
      case '\f':
        bkch = 'f';
        goto bkwr;
      case '\b':
        bkch = 'b';
        goto bkwr;
      case '\a':
        bkch = 'a';
        goto bkwr;
      case '\r':
        bkch = 'r';
        goto bkwr;
      case '\v':
        bkch = 'v';
        goto bkwr;
      case '\t':
        bkch = 't';
        goto bkwr;

      bkwr:
        if (pass) {
          *wr++ = '\\';
          *wr++ = bkch;
        } else {
          n += 2;
        }
        break;

      default:
        if (isprint(ch)) {
          if (pass) {
            *wr++ = ch;
          } else {
            n += 1;
          }
        } else {
          if (pass) {
            *wr++ = '\\';
            *wr++ = 'x';
            snprintf(wr, 3, "%02x", ch);
            wr += 2;
          } else {
            n += 4;
          }
        }
        break;
      }
    }
    if (pass) {
      *wr++ = '"';
      *wr++ = '\0';
    } else {
      n += 2;
    }
  }
  if (ret)
    *buf = ret;
  return n;
}

int test_compare_s_prn(char **buf, void *d, void *a)
{
  const char *base = *(const char **)d;
  return test_compare_s_prn_base(buf, base, (size_t)-1);
}

int test_compare_sn_prn(char **buf, void *d, void *a)
{
  const char *base = *(const char **)d;
  size_t n = ((struct test_compare_ssn_d *)a)->n;
  return test_compare_s_prn_base(buf, base, n);
}

int test_compare_p_prn(char **buf, void *d, void *a)
{
  return test_compare_asprintf(buf, "%p", (void *)(uintptr_t) * (uintmax_t *)d);
}

int test_compare_pd_cmp(void *got, void *exp, void *arg)
{
  return *(ptrdiff_t *)got == *(ptrdiff_t *)exp;
}

int test_compare_pd_prn(char **buf, void *d, void *a)
{
  return test_compare_asprintf(buf, "%+" PRIdMAX, (intmax_t) * (ptrdiff_t *)d);
}

int test_compare_pd_ext_prn(char **buf, void *g, void *e, void *a)
{
  int r;
  struct test_compare_pd_d *p = (struct test_compare_pd_d *)a;
  const char *alblsep, *blblsep;
  char *astr, *bstr;

  alblsep = (p->a.label_size <= 16) ? ": " : ":\n                 ";
  blblsep = (p->b.label_size <= 16) ? ": " : ":\n                 ";

  r = test_compare_p_prn(&astr, &((uintmax_t){p->a.value}), NULL);
  if (r < 0)
    astr = NULL;

  r = test_compare_p_prn(&bstr, &((uintmax_t){p->b.value}), NULL);
  if (r < 0)
    bstr = NULL;

  r = test_compare_asprintf(buf,
                            "%15s%s%s\n"
                            "%15s%s%s\n"
                            "           size: %l" PRIuMAX "\n",
                            p->a.label, alblsep, astr ? astr : "(error)",
                            p->b.label, blblsep, bstr ? bstr : "(error)",
                            (uintmax_t)p->sz);

  if (astr)
    free(astr);
  if (bstr)
    free(bstr);
  return r;
}
