#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include <limits.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>

#include "convert.h"

/*
 * If the compiler defines ISO/IEC/IEEE 60559 flag these macros should
 * be defined. (see Annex F.1 in ISO/IEC 9899:1999 (C99 standard))
 *
 * If not so, we assume the system can not represent NaN, Inf and -0
 * (but if the system provides these macros, use them)
 */
#ifndef __STDC_IEC_559__
#ifndef isnan
static int isnan(double x)
{
  return 0;
}
#endif

#ifndef isinf
static int isinf(double x)
{
  return 0;
}
#endif

#ifndef signbit
static int signbit(double x)
{
  if (x < 0.0) return 1;
  return 0;
}
#endif
#endif

void bin2txt_convert_single(convert_fp *stream, bin2txt_type value)
{
  int exponent;
  size_t n;
  bin2txt_type mantissa;
  bin2txt_type v;
  bin2txt_type lval;
  enum constants {
    maxi = INT_MAX,
  };

  lval = bin2txt_convert_last(stream);
  n = stream->nout;
  v = value;
  if (v == 0.0) {
    if (signbit(v)) {
      if (stream->iout != 0 && lval == v && signbit(lval)) {
        n += bin2txt_fprintf(stream, "R");
      } else {
        n += bin2txt_fprintf(stream, "-0*");
      }
    } else {
      n += bin2txt_fprintf(stream, "0");
    }
  } else if (v == 1.0) {
    n += bin2txt_fprintf(stream, "1");
  } else if (v == 2.0) {
    n += bin2txt_fprintf(stream, "2");
  } else if (v == 3.0) {
    n += bin2txt_fprintf(stream, "3");
  } else if (v == 4.0) {
    n += bin2txt_fprintf(stream, "4");
  } else if (v == 5.0) {
    n += bin2txt_fprintf(stream, "5");
  } else if (v == 6.0) {
    n += bin2txt_fprintf(stream, "6");
  } else if (v == 7.0) {
    n += bin2txt_fprintf(stream, "7");
  } else if (v == 8.0) {
    n += bin2txt_fprintf(stream, "8");
  } else if (v == 9.0) {
    n += bin2txt_fprintf(stream, "9");
  } else if (v == 10.0) {
    n += bin2txt_fprintf(stream, "A");
  } else if (v == 11.0) {
    n += bin2txt_fprintf(stream, "B");
  } else if (v == 12.0) {
    n += bin2txt_fprintf(stream, "C");
  } else if (v == 13.0) {
    n += bin2txt_fprintf(stream, "D");
  } else if (v == 14.0) {
    n += bin2txt_fprintf(stream, "E");
  } else if (v == 15.0) {
    n += bin2txt_fprintf(stream, "F");
  } else {
    if (isnan(v)) {
      n += bin2txt_fprintf(stream, "N");
    } else if (isinf(v)) {
      if (v > 0) {
        n += bin2txt_fprintf(stream, "H");
      } else {
        n += bin2txt_fprintf(stream, "L");
      }
    } else {
      if (stream->iout != 0 && lval == v) {
        n += bin2txt_fprintf(stream, "R");
      } else {
        bin2txt_type u = 0.0, x, z, va;
        int m;
        if (v < 0.0) {
          n += bin2txt_fprintf(stream, "-");
          va = -v;
        } else {
          n += bin2txt_fprintf(stream, "+");
          va = v;
        }
        mantissa = bin2txt_frexp(va, &exponent);
        u = bin2txt_modf(va, &x);
        z = x;
        for (m = 0; m < exponent; ++m) {
          bin2txt_type t, m;
          m = bin2txt_modf(z, &t);
          if (m != 0.0) break;
          z = t / 2.0;
        }
        if (m > 8) {
          n += bin2txt_fprintf(stream, "%" PRIxMAX "/%x",
                           (intmax_t)(z * 2.0),
                           (int)m - 1);
        } else {
          if (x != 0.0) {
            n += bin2txt_fprintf(stream, "%" PRIxMAX "", (intmax_t)x);
          }
        }
        if (u != 0.0) {
          int t;
          n += bin2txt_fprintf(stream, ".");
          t = 0;
          x = u;
          while (u > 0.0) {
            x = x * 2.0;
            t += 1;
            u = bin2txt_modf(x, &mantissa);
          }
          n += bin2txt_fprintf(stream, "%" PRIxMAX "/%x", (intmax_t)x, t);
        }
        n += bin2txt_fprintf(stream, "*");
      }
    }
  }
  stream->last_valf = v;
  stream->last_vald = v;
  stream->iout++;
  stream->nout = n;
}

int bin2txt_convert(bin2txt_type *input, int mx, int my, int mz,
                    convert_fp *stream)
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

  for (is = 0; is < m; ++is, ++input) {
    k = is / mxy;
    i = is % mxy;
    j = i / mx;
    i = i % mx;

    if (i == 0 || stream->nout >= 512) {
      bin2txt_convert_newline(stream);
      stream->nout = 0;
    }

    bin2txt_convert_single(stream, *input);
  }
  bin2txt_convert_finalize(stream);

  return 0;
}

bin2txt_type bin2txt_convert_last(convert_fp *stream)
{
#ifdef JUPITER_DOUBLE
  return stream->last_vald;
#else
  return stream->last_valf;
#endif
}
