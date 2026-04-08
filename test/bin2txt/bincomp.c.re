/* This is -*- c -*- source. */
/* This is vim: set ft=c: source. */

#include <errno.h>
#include <float.h>
#include <inttypes.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <jupiter/re2c_lparser/re2c_lparser.h>

#ifndef __STDC_IEC_559__
#define BINARY_FORMAT_NOTE                                                  \
  "- SINGLE and DOUBLE binary are formatted by the binary representation\n" \
  "  in your system. This does not mean the representation in your\n"       \
  "  system is not IEEE-754.\n"
#else
#define BINARY_FORMAT_NOTE                                                  \
  "- SINGLE and DOUBLE binary are formatted by the binary representation\n" \
  "  of IEEE-754. However the endianess of each bytes must be in that of\n" \
  "  your system.\n"
#endif

static const char *argv0;

enum file_type
{
  FILE_TYPE_INVALID,
  FILE_TYPE_SINGLE,
  FILE_TYPE_DOUBLE,
  FILE_TYPE_TEXTIZED,

  FILE_TYPE_INT8,
  FILE_TYPE_INT16,
  FILE_TYPE_INT32,
  FILE_TYPE_INT64,
  FILE_TYPE_UINT8,
  FILE_TYPE_UINT16,
  FILE_TYPE_UINT32,
  FILE_TYPE_UINT64,

  FILE_TYPE_CHAR,
  FILE_TYPE_SCHAR,
  FILE_TYPE_UCHAR,
  FILE_TYPE_INT,
  FILE_TYPE_UINT,
  FILE_TYPE_LONG,
  FILE_TYPE_ULONG,

  FILE_TYPE_INTMAX_T,
  FILE_TYPE_UINTMAX_T,
  FILE_TYPE_PTRDIFF_T,
  FILE_TYPE_SIZE_T,
};

#ifndef INT8_MAX
#define FILE_TYPE_INT8 FILE_TYPE_INVALID
#endif
#ifndef INT16_MAX
#define FILE_TYPE_INT16 FILE_TYPE_INVALID
#endif
#ifndef INT32_MAX
#define FILE_TYPE_INT32 FILE_TYPE_INVALID
#endif
#ifndef INT64_MAX
#define FILE_TYPE_INT64 FILE_TYPE_INVALID
#endif
#ifndef UINT8_MAX
#define FILE_TYPE_UINT8 FILE_TYPE_INVALID
#endif
#ifndef UINT16_MAX
#define FILE_TYPE_UINT16 FILE_TYPE_INVALID
#endif
#ifndef UINT32_MAX
#define FILE_TYPE_UINT32 FILE_TYPE_INVALID
#endif
#ifndef UINT64_MAX
#define FILE_TYPE_UINT64 FILE_TYPE_INVALID
#endif

struct bincomp_value
{
  double d;
  intmax_t i;
  uintmax_t u;
  int error_d;
  int error_i;
  int error_u;
};

struct bin_comp_file
{
  enum file_type ftype;
  const char *fname;
  FILE *fp;
  struct re2c_lparser textized_parser;
  struct bincomp_value last_value;
};
typedef struct bin_comp_file bin_comp_file;

enum file_type get_file_type(const char *arg);

int get_double(double *out, const char *text);
int bin_comp_file_init(bin_comp_file *dest, const char *fname,
                       const char *ftype);
void bin_comp_file_clean(bin_comp_file *f);
int bin_comp_main(bin_comp_file *got, bin_comp_file *expected, double reltol,
                  double abstol, int verbose);

static void bin_comp_help(void);

int main(int argc, char **argv)
{
  int r;
  int verbose;
  bin_comp_file got_file;
  bin_comp_file exp_file;
  double reltol;
  double abstol;

  r = EXIT_SUCCESS;
  argv0 = argv[0];
  if (argc < 7 || argc > 8) {
    bin_comp_help();
    return EXIT_FAILURE;
  }

  if (!bin_comp_file_init(&got_file, argv[1], argv[3])) {
    r = EXIT_FAILURE;
  }
  if (!bin_comp_file_init(&exp_file, argv[2], argv[4])) {
    r = EXIT_FAILURE;
  }

  if (!get_double(&reltol, argv[5])) {
    r = EXIT_FAILURE;
  }
  if (!get_double(&abstol, argv[6])) {
    r = EXIT_FAILURE;
  }

  verbose = 0;
  if (argc > 7) {
    if (strcmp(argv[7], "1") == 0) {
      verbose = 1;
    }
  }

  if (r == EXIT_SUCCESS) {
    if (!bin_comp_main(&got_file, &exp_file, reltol, abstol, verbose)) {
      r = EXIT_FAILURE;
    }
  }

  bin_comp_file_clean(&got_file);
  bin_comp_file_clean(&exp_file);
  return r;
}

/*!re2c
  re2c:define:YYCTYPE = "unsigned char";
  re2c:define:YYCURSOR = "cur";
  re2c:define:YYMARKER = "mrk";
  re2c:yyfill:enable = 0;
  re2c:indent:string = "  ";
  re2c:flags:8 = 1;
*/

enum file_type get_file_type(const char *arg)
{
  const char *cur;
  const char *mrk;

  cur = arg;
  /*!re2c
    re2c:indent:top = 1;

    end = "\x00";

    * { return FILE_TYPE_INVALID; }
    "SINGLE"/end    { return FILE_TYPE_SINGLE; }
    "DOUBLE"/end    { return FILE_TYPE_DOUBLE; }
    "TEXTIZED"/end  { return FILE_TYPE_TEXTIZED; }
    "INT8"/end      { return FILE_TYPE_INT8; }
    "INT16"/end     { return FILE_TYPE_INT16; }
    "INT32"/end     { return FILE_TYPE_INT32; }
    "INT64"/end     { return FILE_TYPE_INT64; }
    "UINT8"/end     { return FILE_TYPE_UINT8; }
    "UINT16"/end    { return FILE_TYPE_UINT16; }
    "UINT32"/end    { return FILE_TYPE_UINT32; }
    "UINT64"/end    { return FILE_TYPE_UINT64; }
    "CHAR"/end      { return FILE_TYPE_CHAR; }
    "SCHAR"/end     { return FILE_TYPE_SCHAR; }
    "UCHAR"/end     { return FILE_TYPE_UCHAR; }
    "INT"/end       { return FILE_TYPE_INT; }
    "UINT"/end      { return FILE_TYPE_UINT; }
    "LONG"/end      { return FILE_TYPE_LONG; }
    "ULONG"/end     { return FILE_TYPE_ULONG; }
    "INTMAX_T"/end  { return FILE_TYPE_INTMAX_T; }
    "UINTMAX_T"/end { return FILE_TYPE_UINTMAX_T; }
    "PTRDIFF_T"/end { return FILE_TYPE_PTRDIFF_T; }
    "SIZE_T"/end    { return FILE_TYPE_SIZE_T; }
  */

  /* unreachable */
  return FILE_TYPE_INVALID;
}

void bin_comp_value_init(struct bincomp_value *p)
{
  p->d = HUGE_VAL;
  p->i = 0;
  p->u = 0;
  p->error_d = EINVAL;
  p->error_i = EINVAL;
  p->error_u = EINVAL;
}

int bin_comp_file_type_init(bin_comp_file *dest, const char *ftype)
{
  dest->ftype = get_file_type(ftype);
  switch (dest->ftype) {
  case FILE_TYPE_SINGLE:
  case FILE_TYPE_DOUBLE:
  case FILE_TYPE_CHAR:
  case FILE_TYPE_SCHAR:
  case FILE_TYPE_UCHAR:
  case FILE_TYPE_INT:
  case FILE_TYPE_UINT:
  case FILE_TYPE_LONG:
  case FILE_TYPE_ULONG:
  case FILE_TYPE_INTMAX_T:
  case FILE_TYPE_UINTMAX_T:
  case FILE_TYPE_PTRDIFF_T:
  case FILE_TYPE_SIZE_T:
  case FILE_TYPE_INT8:
  case FILE_TYPE_INT16:
  case FILE_TYPE_INT32:
  case FILE_TYPE_INT64:
  case FILE_TYPE_UINT8:
  case FILE_TYPE_UINT16:
  case FILE_TYPE_UINT32:
  case FILE_TYPE_UINT64:
    /* NOP */
    return 1;
  case FILE_TYPE_TEXTIZED:
    if (re2c_lparser_fill(&dest->textized_parser, dest->fp, dest->fname, 0,
                          NULL)) {
      if (errno != 0) {
        fprintf(stderr, "%s: %s\n", dest->fname, strerror(errno));
      } else {
        fprintf(stderr, "%s: Failed to prepare stream\n", dest->fname);
      }
      return 0;
    }
    return 1;
  case FILE_TYPE_INVALID:
    break;
  }
  fprintf(stderr, "%s: %s: Invalid file type\n", dest->fname, ftype);
  return 0;
}

int bin_comp_file_init(bin_comp_file *dest, const char *fname,
                       const char *ftype)
{
  re2c_lparser_init(&dest->textized_parser);

  bin_comp_value_init(&dest->last_value);
  dest->fname = fname;
  errno = 0;
  dest->fp = fopen(fname, "rb");
  if (!dest->fp) {
    if (errno != 0) {
      fprintf(stderr, "%s: %s\n", fname, strerror(errno));
    } else {
      fprintf(stderr, "%s: failed to open (details unknown)\n", fname);
    }
    return 0;
  }

  if (!bin_comp_file_type_init(dest, ftype))
    return 0;
  return 1;
}

void bin_comp_file_clean(bin_comp_file *f)
{
  re2c_lparser_clean(&f->textized_parser);
  if (f->fp) {
    fclose(f->fp);
  }
}

int get_double(double *out, const char *text)
{
  char *chend;

  errno = 0;
  *out = strtod(text, &chend);
  if (errno != 0) {
    fprintf(stderr, "%s: %s\n", text, strerror(errno));
    return 0;
  } else if (chend == text || *chend != '\0') {
    fprintf(stderr, "%s: Invalid floating point value format\n", text);
    return 0;
  }
  return 1;
}

typedef int bin_comp_next_func(bin_comp_file *f, struct bincomp_value *out);

int bin_comp_next_double(bin_comp_file *f, struct bincomp_value *out);
int bin_comp_next_single(bin_comp_file *f, struct bincomp_value *out);
int bin_comp_next_char(bin_comp_file *f, struct bincomp_value *out);
int bin_comp_next_schar(bin_comp_file *f, struct bincomp_value *out);
int bin_comp_next_uchar(bin_comp_file *f, struct bincomp_value *out);
int bin_comp_next_int(bin_comp_file *f, struct bincomp_value *out);
int bin_comp_next_uint(bin_comp_file *f, struct bincomp_value *out);
int bin_comp_next_long(bin_comp_file *f, struct bincomp_value *out);
int bin_comp_next_ulong(bin_comp_file *f, struct bincomp_value *out);
int bin_comp_next_int8(bin_comp_file *f, struct bincomp_value *out);
int bin_comp_next_int16(bin_comp_file *f, struct bincomp_value *out);
int bin_comp_next_int32(bin_comp_file *f, struct bincomp_value *out);
int bin_comp_next_int64(bin_comp_file *f, struct bincomp_value *out);
int bin_comp_next_uint8(bin_comp_file *f, struct bincomp_value *out);
int bin_comp_next_uint16(bin_comp_file *f, struct bincomp_value *out);
int bin_comp_next_uint32(bin_comp_file *f, struct bincomp_value *out);
int bin_comp_next_uint64(bin_comp_file *f, struct bincomp_value *out);
int bin_comp_next_intmax_t(bin_comp_file *f, struct bincomp_value *out);
int bin_comp_next_uintmax_t(bin_comp_file *f, struct bincomp_value *out);
int bin_comp_next_ptrdiff_t(bin_comp_file *f, struct bincomp_value *out);
int bin_comp_next_size_t(bin_comp_file *f, struct bincomp_value *out);
int bin_comp_next_textized(bin_comp_file *f, struct bincomp_value *out);

bin_comp_next_func *bin_comp_next_f(enum file_type ftype)
{
  switch (ftype) {
  case FILE_TYPE_DOUBLE:
    return bin_comp_next_double;
  case FILE_TYPE_SINGLE:
    return bin_comp_next_single;
  case FILE_TYPE_CHAR:
    return bin_comp_next_char;
  case FILE_TYPE_SCHAR:
    return bin_comp_next_schar;
  case FILE_TYPE_UCHAR:
    return bin_comp_next_uchar;
  case FILE_TYPE_INT:
    return bin_comp_next_int;
  case FILE_TYPE_LONG:
    return bin_comp_next_long;
  case FILE_TYPE_UINT:
    return bin_comp_next_uint;
  case FILE_TYPE_ULONG:
    return bin_comp_next_ulong;
  case FILE_TYPE_INT8:
    return bin_comp_next_int8;
  case FILE_TYPE_INT16:
    return bin_comp_next_int16;
  case FILE_TYPE_INT32:
    return bin_comp_next_int32;
  case FILE_TYPE_INT64:
    return bin_comp_next_int64;
  case FILE_TYPE_UINT8:
    return bin_comp_next_uint8;
  case FILE_TYPE_UINT16:
    return bin_comp_next_uint16;
  case FILE_TYPE_UINT32:
    return bin_comp_next_uint32;
  case FILE_TYPE_UINT64:
    return bin_comp_next_uint64;
  case FILE_TYPE_INTMAX_T:
    return bin_comp_next_intmax_t;
  case FILE_TYPE_UINTMAX_T:
    return bin_comp_next_uintmax_t;
  case FILE_TYPE_PTRDIFF_T:
    return bin_comp_next_ptrdiff_t;
  case FILE_TYPE_SIZE_T:
    return bin_comp_next_size_t;
  case FILE_TYPE_TEXTIZED:
    return bin_comp_next_textized;
  case FILE_TYPE_INVALID:
    return NULL;
  }
  return NULL;
}

int bin_comp_next(bin_comp_file *f, struct bincomp_value *out)
{
  bin_comp_next_func *func;
  int r;

  r = 0;
  bin_comp_value_init(out);
  func = bin_comp_next_f(f->ftype);
  if (func)
    r = func(f, out);
  if (r)
    f->last_value = *out;
  return r;
}

double bin_comp_max(double x, double y)
{
  if (x > y)
    return x;
  return y;
}

int bin_comp_is_twos_complement(void)
{
  /* See https://stackoverflow.com/a/3819531 */
  return (((intmax_t)-1) & ((intmax_t)3)) == (intmax_t)3;
}

double bin_comp_value_as_double(const struct bincomp_value *v, int *err)
{
  uintmax_t dblimax = ((uintmax_t)1 << DBL_MANT_DIG);

  if (!v->error_d)
    return v->d;
  if (!v->error_u) {
    if (v->u < (uintmax_t)1 << DBL_MANT_DIG)
      return (double)v->u;
    if (err)
      *err = ERANGE;
    return HUGE_VAL;
  }
  if (!v->error_i) {
    if (v->i >= 0) {
      if ((uintmax_t)v->i < dblimax)
        return (double)v->i;
      if (err)
        *err = ERANGE;
      return HUGE_VAL;
    }
    if (bin_comp_is_twos_complement() && v->i == INTMAX_MIN) {
      if ((uintmax_t)INTMAX_MAX + 1 < dblimax)
        return (double)v->i;
    } else if ((uintmax_t)-v->i < dblimax) {
      return (double)v->i;
    }
    if (err)
      *err = ERANGE;
    return -HUGE_VAL;
  }
  if (err)
    *err = EINVAL;
#ifdef NAN
  return NAN;
#else
  return sqrt(-1.0);
#endif
}

intmax_t bin_comp_value_as_int(const struct bincomp_value *v, int *err)
{
  if (!v->error_i)
    return v->i;
  if (!v->error_u) {
    if (v->u <= (uintmax_t)INTMAX_MAX)
      return v->u;
    if (err)
      *err = ERANGE;
    return -1;
  }
  if (!v->error_d) {
    double fr, ip;
    fr = modf(v->d, &ip);
    if (fr != 0.0) {
      if (err)
        *err = EDOM;
    }
    if (ip <= (double)INTMAX_MAX && ip >= (double)INTMAX_MIN)
      return (intmax_t)ip;
    if (isfinite(fr)) {
      if (err)
        *err = ERANGE;
    } else {
      if (err)
        *err = EINVAL;
    }
    return -1;
  }
  if (err)
    *err = EINVAL;
  return -1;
}

uintmax_t bin_comp_value_as_uint(const struct bincomp_value *v, int *err)
{
  if (!v->error_u)
    return v->u;
  if (!v->error_i) {
    if (v->i < 0) {
      if (err)
        *err = ERANGE;
      return (uintmax_t)-1;
    }
    return v->i;
  }
  if (!v->error_d) {
    double fr, ip;
    if (v->d < 0.0) {
      if (err)
        *err = ERANGE;
      return (uintmax_t)-1;
    }
    fr = modf(v->d, &ip);
    if (fr != 0.0) {
      if (err)
        *err = EDOM;
    }
    if (ip <= (double)UINTMAX_MAX)
      return (uintmax_t)ip;
    if (isfinite(fr)) {
      if (err)
        *err = ERANGE;
    } else {
      if (err)
        *err = EINVAL;
    }
    return -1;
  }
  if (err)
    *err = EINVAL;
  return -1;
}

double bin_comp_rtoldd(double x, double y)
{
  if (x == 0.0 && y == 0.0)
    return 0.0;
  return fabs(x - y) / bin_comp_max(fabs(x), fabs(y));
}

double bin_comp_rtol(const struct bincomp_value *x,
                     const struct bincomp_value *y)
{
  int iex, iey;
  double dx, dy;
  iex = iey = 0;
  dx = bin_comp_value_as_double(x, &iex);
  dy = bin_comp_value_as_double(y, &iey);
  if (iex == 0 && iey == 0)
    return bin_comp_rtoldd(dx, dy);
  return HUGE_VAL;
}

double bin_comp_atoldd(double x, double y) { return x - y; }
double bin_comp_atolii(intmax_t x, intmax_t y) { return x - y; }
double bin_comp_atoluu(uintmax_t x, uintmax_t y)
{
  if (y > x)
    return -(double)(y - x);
  return x - y;
}

double bin_comp_atol(const struct bincomp_value *x,
                     const struct bincomp_value *y)
{
  int iex, iey;
  intmax_t ix, iy;
  uintmax_t ux, uy;
  double dx, dy;

  iex = iey = 0;
  ix = bin_comp_value_as_int(x, &iex);
  iy = bin_comp_value_as_int(y, &iey);
  if (iex == 0 && iey == 0)
    return bin_comp_atolii(ix, iy);

  iex = iey = 0;
  ux = bin_comp_value_as_uint(x, &iex);
  uy = bin_comp_value_as_uint(y, &iey);
  if (iex == 0 && iey == 0)
    return bin_comp_atoluu(ux, uy);

  iex = iey = 0;
  dx = bin_comp_value_as_double(x, &iex);
  dy = bin_comp_value_as_double(y, &iey);
  if (iex == 0 && iey == 0)
    return bin_comp_atoldd(dx, dy);

  return HUGE_VAL;
}

int bin_comp_fprintf(FILE *fp, int width, int d_precision,
                     const struct bincomp_value *value)
{
  if (value->error_i == 0)
    return fprintf(fp, "%*" PRIdMAX, width, value->i);
  if (value->error_u == 0)
    return fprintf(fp, "%*" PRIuMAX, width, value->u);
  if (value->error_d == 0)
    return fprintf(fp, "%*.*e", width, d_precision, value->d);
  return fprintf(fp, "%*s", width, "(error)");
}

int bin_comp_main(bin_comp_file *got, bin_comp_file *expected, double reltol,
                  double abstol, int verbose)
{
  int r;
  int got_stat;
  int exp_stat;
  struct bincomp_value got_val, exp_val;
  ptrdiff_t loc;
  double maxreltol;
  double maxabstol;
  double atol, rtol;

  maxreltol = 0.0;
  maxabstol = 0.0;

  r = 1;
  loc = 0;
  while (1) {
    got_stat = bin_comp_next(got, &got_val);
    exp_stat = bin_comp_next(expected, &exp_val);

    if (got_stat && exp_stat) {
      atol = bin_comp_atol(&got_val, &exp_val);
      rtol = bin_comp_rtol(&got_val, &exp_val);
      if (verbose) {
        if (loc == 0) {
          fprintf(stdout, "# %19s %21s %14s %14s %s\n", "Value got",
                  "Value expected", "RelativeTol", "AbsoluteTol", "i");
        }
        bin_comp_fprintf(stdout, 21, 13, &got_val);
        fprintf(stdout, " ");
        bin_comp_fprintf(stdout, 21, 13, &exp_val);
        fprintf(stdout, " %14.6e %14.6e %" PRIdMAX "\n", rtol, atol,
                (intmax_t)loc);
      }

      if (fabs(atol) > fabs(maxabstol)) {
        maxabstol = atol;
      }
      if (fabs(rtol) > fabs(maxreltol)) {
        maxreltol = rtol;
      }
      loc++;
      continue;
    }

    if (!got_stat && exp_stat) {
      fprintf(stderr, "%s(i=%" PRIdMAX "): Data shorter than expected\n",
              got->fname, (intmax_t)loc);
      r = 0;
    }
    if (got_stat && !exp_stat) {
      fprintf(stderr, "%s(i=%" PRIdMAX "): Data longer than expected\n",
              got->fname, (intmax_t)loc);
      r = 0;
    }
    /* There is a chance to error at same time, but ignore currently. */
    break;
  }

  if (r) {
    int stat;
    if (verbose) {
      fprintf(stdout, "\n------8<----------------------------\n");
    }

    fprintf(stdout, "Maximum Absolute Tolerance: %18.9e", maxabstol);
    if (abstol >= 0.0) {
      const char *cond;
      if (fabs(maxabstol) > abstol) {
        cond = "bad";
        r = 0;
      } else {
        cond = "good";
      }
      fprintf(stdout, " (assumes <= %.6e: %s)\n", abstol, cond);
    } else {
      fprintf(stdout, "\n");
    }

    fprintf(stdout, "Maximum Relative Tolerance: %18.9e", maxreltol);
    if (reltol >= 0.0) {
      const char *cond;
      if (fabs(maxreltol) > reltol) {
        cond = "bad";
        r = 0;
      } else {
        cond = "good";
      }
      fprintf(stdout, " (assumes <= %.6e: %s)\n", reltol, cond);
    } else {
      fprintf(stdout, "\n");
    }
  }
  return r;
}

int bin_comp_next_base(bin_comp_file *f, struct bincomp_value *out, void *buf,
                       size_t sz)
{
  errno = 0;
  if (fread(buf, sz, 1, f->fp) != 1) {
    if (errno != 0) {
      fprintf(stderr, "%s: %s\n", f->fname, strerror(errno));
    }
    return 0;
  }
  return 1;
}

int bin_comp_next_base_d(bin_comp_file *f, struct bincomp_value *out, void *buf,
                         size_t sz, double (*to_double)(void *buf))
{
  int err;
  if (!bin_comp_next_base(f, out, buf, sz))
    return 0;

  err = errno;
  out->d = to_double(buf);
  out->error_d = err;
  return 1;
}

int bin_comp_next_base_i(bin_comp_file *f, struct bincomp_value *out, void *buf,
                         size_t sz, intmax_t (*to_imax)(void *buf))
{
  int err;
  if (!bin_comp_next_base(f, out, buf, sz))
    return 0;

  err = errno;
  out->i = to_imax(buf);
  out->error_i = err;
  if (err == 0 && out->i >= 0) {
    out->u = out->i;
    out->error_u = 0;
  }
  return 1;
}

int bin_comp_next_base_u(bin_comp_file *f, struct bincomp_value *out, void *buf,
                         size_t sz, uintmax_t (*to_umax)(void *buf))
{
  int err;
  if (!bin_comp_next_base(f, out, buf, sz))
    return 0;

  err = errno;
  out->u = to_umax(buf);
  out->error_u = err;
  if (err == 0 && out->u <= INTMAX_MAX) {
    out->i = out->u;
    out->error_i = 0;
  }
  return 1;
}

#define DEFINE_NEXT_FUNC(func, ctype, base_func, conv_func) \
  int func(bin_comp_file *f, struct bincomp_value *out)     \
  {                                                         \
    ctype d;                                                \
    return base_func(f, out, &d, sizeof(ctype), conv_func); \
  }

#define DEFINE_NEXT_FUNC_UNSUPPORTED(func) \
  int func(bin_comp_file *f, struct bincomp_value *out) { return 0; }

static double from_double(void *buf) { return *(double *)buf; }
static double from_single(void *buf) { return *(float *)buf; }

DEFINE_NEXT_FUNC(bin_comp_next_double, double, bin_comp_next_base_d,
                 from_double)
DEFINE_NEXT_FUNC(bin_comp_next_single, float, bin_comp_next_base_d, from_single)

#if CHAR_MIN < 0
static intmax_t from_char(void *buf) { return *(char *)buf; }
DEFINE_NEXT_FUNC(bin_comp_next_char, char, bin_comp_next_base_i, from_char)
#else
static uintmax_t from_char(void *buf) { return *(char *)buf; }
DEFINE_NEXT_FUNC(bin_comp_next_char, char, bin_comp_next_base_u, from_char)
#endif

static intmax_t from_schar(void *buf) { return *(signed char *)buf; }
static intmax_t from_int(void *buf) { return *(int *)buf; }
static intmax_t from_long(void *buf) { return *(long *)buf; }
static intmax_t from_ptrdiff_t(void *buf) { return *(ptrdiff_t *)buf; }
static intmax_t from_intmax_t(void *buf) { return *(intmax_t *)buf; }

DEFINE_NEXT_FUNC(bin_comp_next_schar, signed char, bin_comp_next_base_i,
                 from_schar)
DEFINE_NEXT_FUNC(bin_comp_next_int, int, bin_comp_next_base_i, from_int)
DEFINE_NEXT_FUNC(bin_comp_next_long, long, bin_comp_next_base_i, from_long)
DEFINE_NEXT_FUNC(bin_comp_next_ptrdiff_t, ptrdiff_t, bin_comp_next_base_i,
                 from_ptrdiff_t)
DEFINE_NEXT_FUNC(bin_comp_next_intmax_t, intmax_t, bin_comp_next_base_i,
                 from_intmax_t)

#ifdef INT8_MAX
static intmax_t from_int8(void *buf) { return *(int8_t *)buf; }
DEFINE_NEXT_FUNC(bin_comp_next_int8, int8_t, bin_comp_next_base_i, from_int8)
#else
DEFINE_NEXT_FUNC_UNSUPPORTED(bin_comp_next_int8)
#endif

#ifdef INT16_MAX
static intmax_t from_int16(void *buf) { return *(int16_t *)buf; }
DEFINE_NEXT_FUNC(bin_comp_next_int16, int16_t, bin_comp_next_base_i, from_int16)
#else
DEFINE_NEXT_FUNC_UNSUPPORTED(bin_comp_next_int16)
#endif

#ifdef INT32_MAX
static intmax_t from_int32(void *buf) { return *(int32_t *)buf; }
DEFINE_NEXT_FUNC(bin_comp_next_int32, int32_t, bin_comp_next_base_i, from_int32)
#else
DEFINE_NEXT_FUNC_UNSUPPORTED(bin_comp_next_int32)
#endif

#ifdef INT64_MAX
static intmax_t from_int64(void *buf) { return *(int64_t *)buf; }
DEFINE_NEXT_FUNC(bin_comp_next_int64, int64_t, bin_comp_next_base_i, from_int64)
#else
DEFINE_NEXT_FUNC_UNSUPPORTED(bin_comp_next_int64)
#endif

static uintmax_t from_uchar(void *buf) { return *(unsigned char *)buf; }
static uintmax_t from_uint(void *buf) { return *(unsigned int *)buf; }
static uintmax_t from_ulong(void *buf) { return *(unsigned long *)buf; }
static uintmax_t from_size_t(void *buf) { return *(size_t *)buf; }
static uintmax_t from_uintmax_t(void *buf) { return *(uintmax_t *)buf; }

DEFINE_NEXT_FUNC(bin_comp_next_uchar, unsigned char, bin_comp_next_base_u,
                 from_uchar)
DEFINE_NEXT_FUNC(bin_comp_next_uint, unsigned int, bin_comp_next_base_u,
                 from_uint)
DEFINE_NEXT_FUNC(bin_comp_next_ulong, unsigned long, bin_comp_next_base_u,
                 from_ulong)
DEFINE_NEXT_FUNC(bin_comp_next_size_t, size_t, bin_comp_next_base_u,
                 from_size_t)
DEFINE_NEXT_FUNC(bin_comp_next_uintmax_t, uintmax_t, bin_comp_next_base_u,
                 from_uintmax_t)

#ifdef UINT8_MAX
static uintmax_t from_uint8(void *buf) { return *(uint8_t *)buf; }
DEFINE_NEXT_FUNC(bin_comp_next_uint8, uint8_t, bin_comp_next_base_u, from_uint8)
#else
DEFINE_NEXT_FUNC_UNSUPPORTED(bin_comp_next_uint8)
#endif

#ifdef UINT16_MAX
static uintmax_t from_uint16(void *buf) { return *(uint16_t *)buf; }
DEFINE_NEXT_FUNC(bin_comp_next_uint16, uint16_t, bin_comp_next_base_u,
                 from_uint16)
#else
DEFINE_NEXT_FUNC_UNSUPPORTED(bin_comp_next_uint16)
#endif

#ifdef UINT32_MAX
static uintmax_t from_uint32(void *buf) { return *(uint32_t *)buf; }
DEFINE_NEXT_FUNC(bin_comp_next_uint32, uint32_t, bin_comp_next_base_u,
                 from_uint32)
#else
DEFINE_NEXT_FUNC_UNSUPPORTED(bin_comp_next_uint32)
#endif

#ifdef UINT64_MAX
static uintmax_t from_uint64(void *buf) { return *(uint64_t *)buf; }
DEFINE_NEXT_FUNC(bin_comp_next_uint64, uint64_t, bin_comp_next_base_u,
                 from_uint64)
#else
DEFINE_NEXT_FUNC_UNSUPPORTED(bin_comp_next_uint64)
#endif

static int tok2hex_v(uintmax_t *r, int i, int *err)
{
  if (*r <= (UINTMAX_MAX >> 4) && i >= 0 && i < 16) {
    *r = *r * 16 + i;
    return 1;
  }
  if (err)
    *err = ERANGE;
  return 0;
}

uintmax_t bin_comp_tok2hex(struct re2c_lparser *p, int *err)
{
  uintmax_t r;
  const char *tok;
  const char *cur;
  const char *mrk;

  cur = p->tok;
  r = 0;
  while (cur < p->cur) {
    tok = cur;
    /*!re2c
      re2c:indent:top = 2;

      * { if (err) *err = EINVAL; return (uintmax_t)-1; }
      [0-9] { if (!tok2hex_v(&r, *tok - '0', err)) return -1; continue; }
      [a-f] { if (!tok2hex_v(&r, *tok - 'a' + 10, err)) return -1; continue; }
      [A-F] { if (!tok2hex_v(&r, *tok - 'A' + 10, err)) return -1; continue; }
     */

    /* unreachable */
    if (err)
      *err = EINVAL;
    return (uintmax_t)-1;
  }
  return r;
}

int bin_comp_next_textized(bin_comp_file *f, struct bincomp_value *out)
{
  struct re2c_lparser *p;
  int start;

  p = &f->textized_parser;
  /*!re2c
    re2c:define:YYCURSOR = "p->cur";
    re2c:define:YYMARKER = "p->mrk";
    re2c:define:YYCTXMARKER = "p->ctxmrk";
    re2c:define:YYLIMIT = "p->lim";
    re2c:define:YYFILL:naked = 1;
    re2c:define:YYFILL = "{ if (re2c_lparser_fill(p, f->fp, f->fname, @@, NULL) < 0) goto error; }";
    re2c:yyfill:enable = 1;
  */

  if (re2c_lparser_eof(&f->textized_parser)) {
    return 0;
  }

  start = 0;
  if (f->textized_parser.line == 1 && f->textized_parser.col == 1) {
    start = 1;
  } else {
    while (1) {
      re2c_lparser_start_token(p);
      /*!re2c
        re2c:indent:top = 3;

        // Go back.
        * { p->cur = p->tok; break; }

        ("\n"|"\r\n"|"\r") { re2c_lparser_loc_upd_utf8(p); start = 1; break; }
      */

      /* unreachable */
      return 0;
    }
    if (re2c_lparser_eof(p)) {
      return 0;
    }
  }

  while (start) { /* just to be like `if (start)` with `break`-able */
    re2c_lparser_start_token(p);
    /*!re2c
      re2c:indent:top = 2;

      * { goto error; }
      [0-9a-fA-F]+":"" "+ { re2c_lparser_loc_upd_utf8(p); break; }
      [0-9a-fA-F]+("\n"|"\r\n"|"\r") {
        re2c_lparser_loc_upd_utf8(p); return 0;
      }
    */

    /* unreachable */
    return 0;
  }
  if (re2c_lparser_eof(p)) {
    goto error;
  }

  while (!re2c_lparser_eof(p)) {
    enum fini_type
    {
      finite,
      positive_inf,
      negative_inf,
      not_a_number,
      repeat
    };

    int ier = 0;
    int negative = 0;
    double dout;
    enum fini_type fini = finite;
    int psr = 0;
    uintmax_t mantissa = 0;
    uintmax_t mantissa_exp = 0;
    uintmax_t fraction = 0;
    uintmax_t fraction_exp = 0;

    re2c_lparser_start_token(p);
    while (1) {
      /*!re2c
        re2c:indent:top = 3;

        * { goto error; }
        [0-9a-fA-F] {
          re2c_lparser_loc_upd_utf8(p);
          mantissa = bin_comp_tok2hex(p, &ier);
          if (ier != 0)
            goto error;
          break;
        }
        [hH] { re2c_lparser_loc_upd_utf8(p); fini = positive_inf; break; }
        [lL] { re2c_lparser_loc_upd_utf8(p); fini = negative_inf; break; }
        [nN] { re2c_lparser_loc_upd_utf8(p); fini = not_a_number; break; }
        [rR] { re2c_lparser_loc_upd_utf8(p); fini = repeat; break; }
        '-'  { re2c_lparser_loc_upd_utf8(p); psr = 1; negative = 1; break; }
        '+'  { re2c_lparser_loc_upd_utf8(p); psr = 1; negative = 0; break; }
      */

      /* unreachable */
      return 0;
    }

    if (psr) {
      enum context
      {
        psr_mant,
        psr_mant_exp,
        psr_frac,
        psr_frac_exp,
        psr_end
      } stat = psr_mant;

      while (1) {
        if (re2c_lparser_eof(p))
          goto error;

        re2c_lparser_start_token(p);
        /*!re2c
          re2c:indent:top = 4;

          *   { goto error; }
          "*" { re2c_lparser_loc_upd_utf8(p); stat = psr_end; break; }
          "." { re2c_lparser_loc_upd_utf8(p); stat = psr_frac; break; }
          "/" {
            re2c_lparser_loc_upd_utf8(p);
            if (mantissa <= 0) goto error;
            stat = psr_mant_exp;
            break;
          }

          [0-9a-fA-F]+ {
            re2c_lparser_loc_upd_utf8(p);
            mantissa = bin_comp_tok2hex(p, &ier);
            if (ier)
              goto error;
            continue;
          }
        */

        /* Unreachable */
        goto error;
      }

      if (stat == psr_mant_exp) {
        while (1) {
          if (re2c_lparser_eof(p))
            goto error;

          re2c_lparser_start_token(p);
          /*!re2c
            re2c:indent:top = 5;

            *   { goto error; }
            "*" { re2c_lparser_loc_upd_utf8(p); stat = psr_end; break; }
            "." { re2c_lparser_loc_upd_utf8(p); stat = psr_frac; break; }

            [0-9a-fA-F]+ {
              re2c_lparser_loc_upd_utf8(p);
              mantissa_exp = bin_comp_tok2hex(p, &ier);
              if (ier)
                goto error;
              continue;
            }
          */

          /* Unreachable */
          goto error;
        }
      }

      if (stat == psr_frac) {
        while (1) {
          if (re2c_lparser_eof(p))
            goto error;

          re2c_lparser_start_token(p);
          /*!re2c
            re2c:indent:top = 5;

            *   { goto error; }
            "*" { re2c_lparser_loc_upd_utf8(p); stat = psr_end; break; }
            "/" {
              re2c_lparser_loc_upd_utf8(p);
              stat = psr_frac_exp;
              break;
            }

            [0-9a-fA-F]+ {
              re2c_lparser_loc_upd_utf8(p);
              fraction = bin_comp_tok2hex(p, &ier);
              if (ier)
                goto error;
              continue;
            }
          */

          /* Unreachable */
          goto error;
        }
      }

      if (stat == psr_frac_exp) {
        while (1) {
          if (re2c_lparser_eof(p))
            goto error;

          re2c_lparser_start_token(p);
          /*!re2c
            re2c:indent:top = 5;

            *   { goto error; }
            "*" { re2c_lparser_loc_upd_utf8(p); stat = psr_end; break; }

            [0-9a-fA-F]+ {
              re2c_lparser_loc_upd_utf8(p);
              fraction_exp = bin_comp_tok2hex(p, &ier);
              if (ier)
                goto error;
              continue;
            }
          */

          /* Unreachable */
          goto error;
        }
      }

      if (stat != psr_end)
        goto error;
    }

    switch (fini) {
    case finite:
      if (fraction == 0 && mantissa <= (UINTMAX_MAX >> mantissa_exp)) {
        uintmax_t u = mantissa << mantissa_exp;
        if (!negative) {
          out->u = u;
          out->error_u = 0;
        }
        if (u < INTMAX_MAX) {
          if (negative) {
            out->i = -(intmax_t)u;
          } else {
            out->i = u;
          }
          out->error_i = 0;
        }
      }
      if (mantissa >= ((uintmax_t)1 << DBL_MANT_DIG)) {
        out->error_d = ERANGE;
      } else {
        out->error_d = 0; /* other errors are ignored */
      }
      dout = ldexp((double)fraction, -fraction_exp);
      dout += ldexp((double)mantissa, mantissa_exp);
      if (negative) {
        dout *= -1.0;
      }
      out->d = dout;
      break;
    case positive_inf:
      out->d = HUGE_VAL;
      out->error_d = 0;
      break;
    case negative_inf:
      out->d = -HUGE_VAL;
      out->error_d = 0;
      break;
    case not_a_number:
#ifndef NAN
#define NAN (sqrt(-1.0))
#define BINCOMP_DEFINED_NAN
#endif
      out->d = NAN;
      out->error_d = 0;
#ifdef BINCOMP_DEFINED_NAN
#undef NAN
#endif
      break;
    case repeat:
      *out = f->last_value;
      break;
    }
    break;
  }
  if (re2c_lparser_eof(p)) {
    return 0;
  }

  return 1;

error:
  if (p->readerr) {
    fprintf(stderr, "%s(%ld:%ld): %s\n", f->fname, p->line, p->col,
            strerror(p->readerr));
  } else {
    fprintf(stderr, "%s(%ld:%ld): Invalid format around here\n", f->fname,
            p->line, p->col);
  }
  return 0;
}

static void bin_comp_help(void)
{
  fprintf(
    stderr,
    "Usage: %s GOT EXPECTED INTYPE EXTYPE RELATIVE_TOL ABSOLUTE_TOL [0|1]\n"
    "\n"
    "  GOT           Filename to be compared\n"
    "  EXPECTED      Filename of the expected result\n"
    "  INTYPE        Format of [GOT] file\n"
    "  EXTYPE        Format of [EXPETCED] file\n"
    "  RELATIVE_TOL  Maximum acceptable value of relative tolerance\n"
    "  ABSOLUTE_TOL  Maximum acceptable value of absolute tolerance\n"
    "  [0|1]         If 1, write down all data (optional)\n"
    "\n"
    "INTYPE and EXTYPE must be one of:\n"
    "  CHAR          System's default char binary\n"
    "  SCHAR         System's default signed char binary\n"
    "  UCHAR         System's default unsigned char binary\n"
    "  INT           System's default int binary\n"
    "  LONG          System's default long binary\n"
    "  UINT          System's default unsigned binary\n"
    "  ULONG         System's default unsigned long binary\n"
    "  DOUBLE        Double precision (`double` type) binary\n"
    "  SINGLE        Single precision (`float` type) binary\n"
#ifdef INT8_MAX
    "  INT8          8-bit int binary\n"
#endif
#ifdef INT16_MAX
    "  INT16         16-bit int binary\n"
#endif
#ifdef INT32_MAX
    "  INT32         32-bit int binary\n"
#endif
#ifdef INT64_MAX
    "  INT64         64-bit int binary\n"
#endif
#ifdef UINT8_MAX
    "  UINT8         8-bit unsigned int binary\n"
#endif
#ifdef UINT16_MAX
    "  UINT16        16-bit unsigned int binary\n"
#endif
#ifdef UINT32_MAX
    "  UINT32        32-bit unsigned int binary\n"
#endif
#ifdef UINT64_MAX
    "  UINT64        64-bit unsigned int binary\n"
#endif
    "  INTMAX_T      System's intmax_t binary\n"
    "  UINTMAX_T     System's uintmax_t binary\n"
    "  PTRDIFF_T     System's ptrdiff_t binary\n"
    "  SIZE_T        System's size_t binary\n"
    "  TEXTIZED      Textized data by bin2text utility\n"
    "\n"
    "- Giving negative tolerance skips checking of it.\n"
    "- Tolerance values must be in the format of a floating point value\n"
    "  defined by `strtod(3)` function.\n"
    "- Relative tolerance is defined by Relative Percent Difference,\n"
    "  and not common relative error:\n"
    "    d_{inf} = |x - y|/max(|x|, |y|). (and 0 if x = y = 0)\n"
    BINARY_FORMAT_NOTE "\n",
    argv0);
}
