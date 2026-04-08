#include <stdarg.h>
#include <limits.h>
#include <stdint.h>
#include <inttypes.h>

#include "convert.h"

static size_t bin2txt_convert_single_small(convert_fp *stream, uintmax_t value)
{
  switch (value) {
  case 0:
    return bin2txt_fprintf(stream, "0");
  case 1:
    return bin2txt_fprintf(stream, "1");
  case 2:
    return bin2txt_fprintf(stream, "2");
  case 3:
    return bin2txt_fprintf(stream, "3");
  case 4:
    return bin2txt_fprintf(stream, "4");
  case 5:
    return bin2txt_fprintf(stream, "5");
  case 6:
    return bin2txt_fprintf(stream, "6");
  case 7:
    return bin2txt_fprintf(stream, "7");
  case 8:
    return bin2txt_fprintf(stream, "8");
  case 9:
    return bin2txt_fprintf(stream, "9");
  case 10:
    return bin2txt_fprintf(stream, "A");
  case 11:
    return bin2txt_fprintf(stream, "B");
  case 12:
    return bin2txt_fprintf(stream, "C");
  case 13:
    return bin2txt_fprintf(stream, "D");
  case 14:
    return bin2txt_fprintf(stream, "E");
  case 15:
    return bin2txt_fprintf(stream, "F");
  }
  return 0;
}

void bin2txt_convert_single_u(convert_fp *stream, uintmax_t value)
{
  size_t n;
  uintmax_t v;
  uintmax_t lval;

  lval = bin2txt_convert_last_i(stream);
  n = stream->nout;
  v = value;
  if (v < 16) {
    n += bin2txt_convert_single_small(stream, value);
  } else {
    if (stream->iout != 0 && lval == v) {
      n += bin2txt_fprintf(stream, "R");
    } else {
      uintmax_t mantissa, exponent, fexp, dexp;
      uintmax_t va, vb, ve;
      n += bin2txt_fprintf(stream, "+");
      va = v;
      for (exponent = 0, vb = va; vb > 0 && (vb & 1) == 0; vb >>= 1)
        ++exponent;
      for (fexp = 0, ve = va; ve > 0; ve >>= 4)
        ++fexp;
      for (dexp = 0, ve = exponent; ve > 0; ve >>= 4)
        ++dexp;

      /*
       * - LHS is number of digits for original number
       * - RHS is number of digits in mant/exp form.
       */
      if (exponent / 4 + 1 <= dexp + 2) {
        n += bin2txt_fprintf(stream, "%" PRIxMAX "", va);
      } else {
        n += bin2txt_fprintf(stream, "%" PRIxMAX "", vb);
        if (exponent > 0) {
          n += bin2txt_fprintf(stream, "/%" PRIxMAX "", exponent);
        }
      }
      n += bin2txt_fprintf(stream, "*");
    }
  }
  stream->last_valf = v;
  stream->last_vald = v;
  stream->last_vali = v;
  stream->last_valu = v;
  stream->iout++;
  stream->nout = n;
}

void bin2txt_convert_single_i(convert_fp *stream, intmax_t value)
{
  size_t n;
  intmax_t v;
  intmax_t lval;

  lval = bin2txt_convert_last_i(stream);
  n = stream->nout;
  v = value;
  if (v >= 0 && v < 16) {
    n += bin2txt_convert_single_small(stream, value);
  } else {
    if (stream->iout != 0 && lval == v) {
      n += bin2txt_fprintf(stream, "R");
    } else {
      uintmax_t mantissa, exponent, fexp, dexp;
      uintmax_t va, vb, ve;
      if (v < 0) {
        n += bin2txt_fprintf(stream, "-");
        va = -v;
      } else {
        n += bin2txt_fprintf(stream, "+");
        va = v;
      }
      for (exponent = 0, vb = va; vb > 0 && (vb & 1) == 0; vb >>= 1)
        ++exponent;
      for (fexp = 0, ve = va; ve > 0; ve >>= 4)
        ++fexp;
      for (dexp = 0, ve = exponent; ve > 0; ve >>= 4)
        ++dexp;

      /*
       * - LHS is number of digits for original number
       * - RHS is number of digits in mant/exp form.
       */
      if (exponent / 4 + 1 <= dexp + 2) {
        n += bin2txt_fprintf(stream, "%" PRIxMAX "", va);
      } else {
        n += bin2txt_fprintf(stream, "%" PRIxMAX "", vb);
        if (exponent > 0) {
          n += bin2txt_fprintf(stream, "/%" PRIxMAX "", exponent);
        }
      }
      n += bin2txt_fprintf(stream, "*");
    }
  }
  stream->last_valf = v;
  stream->last_vald = v;
  stream->last_vali = v;
  stream->last_valu = v;
  stream->iout++;
  stream->nout = n;
}

static int bin2txt_convert_ib(void *input, int mx, int my, int mz,
                              convert_fp *stream,
                              void (*inc_input)(void **input),
                              void (*convert)(convert_fp *stream, void *input))
{
  int is, i, j, k;
  int m;
  int mxy;
  int ml;

  mxy = mx * my;
  m = mxy * mz;
  i = m;

  ml = 1;
  while (i > 0) {
    i = i >> 4;
    ++ml;
  }

  i = 0;
  j = 0;
  k = 0;

  for (is = 0; is < m; ++is, inc_input(&input)) {
    k = is / mxy;
    i = is % mxy;
    j = i / mx;
    i = i % mx;

    if (i == 0 || stream->nout >= 512) {
      bin2txt_convert_newline(stream);
      stream->nout = 0;
    }

    convert(stream, input);
  }
  bin2txt_convert_finalize(stream);

  return 0;
}

static void inc_input_i(void **input) { ++*(int **)input; }
static void inc_input_l(void **input) { ++*(long **)input; }
static void inc_input_u(void **input) { ++*(unsigned **)input; }

static void convert_i(convert_fp *stream, void *input)
{
  bin2txt_convert_single_i(stream, *(int *)input);
}

static void convert_l(convert_fp *stream, void *input)
{
  bin2txt_convert_single_i(stream, *(long *)input);
}

static void convert_u(convert_fp *stream, void *input)
{
  bin2txt_convert_single_u(stream, *(unsigned *)input);
}

int bin2txt_convert_i(int *input, int mx, int my, int mz, convert_fp *stream)
{
  return bin2txt_convert_ib(input, mx, my, mz, stream, inc_input_i, convert_i);
}

int bin2txt_convert_l(long *input, int mx, int my, int mz, convert_fp *stream)
{
  return bin2txt_convert_ib(input, mx, my, mz, stream, inc_input_l, convert_l);
}

int bin2txt_convert_u(unsigned *input, int mx, int my, int mz,
                      convert_fp *stream)
{
  return bin2txt_convert_ib(input, mx, my, mz, stream, inc_input_u, convert_u);
}

intmax_t bin2txt_convert_last_i(convert_fp *stream)
{
  return stream->last_vali;
}

uintmax_t bin2txt_convert_last_u(convert_fp *stream)
{
  return stream->last_valu;
}
