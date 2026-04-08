#include <stdio.h>
#include <limits.h>
#include <float.h>
#include <math.h>

#include <jupiter/struct.h>

#include "convert.h"

#ifdef JUPITER_DOUBLE
#define JUPITER_TYPE_MAX DBL_MAX
#define JUPITER_TYPE_MIN DBL_MIN
#define JUPITER_TYPE_AFTER nextafter
#define JUPITER_TYPE_HUGE_VAL HUGE_VAL
#define JUPITER_TYPE_CONST(val) val
#define JUPITER_TYPE_MATH_FUNC(func) func
#else
#define JUPITER_TYPE_MAX FLT_MAX
#define JUPITER_TYPE_MIN FLT_MIN
#ifdef HUGE_VALF
#define JUPITER_TYPE_HUGE_VAL HUGE_VALF
#else
#define JUPITER_TYPE_HUGE_VAL HUGE_VAL
#endif
#define JUPITER_TYPE_AFTER nextafterf
#define JUPITER_TYPE_CONST(val) val##f
#define JUPITER_TYPE_MATH_FUNC(func) func##f
#endif

int main(int argc, char **argv)
{
  convert_fp *fp;
  enum { nval = 50 };
  type val[nval];
  long lval[nval];
  int ival[nval];
  unsigned uval[nval];
  int i;

  fp = convert_file_bind(stdout);
  if (!fp) {
    return 1;
  }

  i = 0;
  val[i++] = JUPITER_TYPE_CONST(0.0);
  val[i++] = JUPITER_TYPE_CONST(1.0);
  val[i++] = JUPITER_TYPE_CONST(2.0);
  val[i++] = JUPITER_TYPE_CONST(10.0);
  val[i++] = JUPITER_TYPE_CONST(15.0);
  val[i++] = JUPITER_TYPE_AFTER(1.0, JUPITER_TYPE_MAX);
  val[i++] = JUPITER_TYPE_AFTER(1.0, JUPITER_TYPE_MIN);
  val[i++] = JUPITER_TYPE_MAX;
  val[i++] = JUPITER_TYPE_MIN;
  val[i++] = JUPITER_TYPE_HUGE_VAL;  /* Prints 'H' */
  val[i++] = -JUPITER_TYPE_HUGE_VAL; /* Prints 'L' */
#ifdef NAN
  val[i++] = NAN;
#else
  /* Trying invalid calculation */
  val[i++] = JUPITER_TYPE_MATH_FUNC(sqrt)(JUPITER_TYPE_CONST(-1.0));
#endif
  val[i++] = JUPITER_TYPE_CONST(66.11);
  val[i++] = JUPITER_TYPE_CONST(-66.11);
#ifdef JUPITER_DOUBLE
  val[i++] = JUPITER_TYPE_CONST(1e+30);
  val[i++] = JUPITER_TYPE_CONST(1e-30);
#else
  val[i++] = JUPITER_TYPE_CONST(1e+10);
  val[i++] = JUPITER_TYPE_CONST(1e-10);
#endif
  val[i++] = JUPITER_TYPE_CONST(0.5);    /*   2^(-1): ".1/1" */
  val[i++] = JUPITER_TYPE_CONST(0.4375); /* 7*2^(-4): ".7/4" */
  val[i++] = JUPITER_TYPE_AFTER(0x1p+10, JUPITER_TYPE_MAX);
  val[i  ] = val[i-1]; i++;
  val[i++] = JUPITER_TYPE_CONST(1024.0);
  val[i++] = JUPITER_TYPE_CONST(-0.0);
  val[i  ] = val[i-1]; i++;
  val[i++] = JUPITER_TYPE_CONST(0.0);
  val[i++] = JUPITER_TYPE_CONST(-0.0);
  val[i  ] = (type)i; i++;

  bin2txt_convert(val, 1, i, 1, fp);
  convert_file_close(fp);

  fp = convert_file_bind(stdout);
  if (!fp) {
    return 1;
  }

  i = 0;
  ival[i++] = 0;
  ival[i++] = 1;
  ival[i++] = 2;
  ival[i++] = 2;
  ival[i++] = 9;
  ival[i++] = 10;
  ival[i++] = 15;
  ival[i++] = -1;
  ival[i++] = -5;
  ival[i++] = -5;
  ival[i++] = 1024;
  ival[i++] = -1024;
  ival[i++] = 1025;
  ival[i++] = -1025;
  ival[i++] = 1600;
  ival[i++] = -1600;
  ival[i++] = 2048;
  ival[i++] = 4096;
  ival[i++] = 0x80000;
  ival[i++] = 0x22000;
  ival[i++] = 0x22800;
  ival[i++] = 0x22400;
  ival[i++] = 0x228000;
  ival[i++] = 0x224000;
  ival[i++] = 0x226000;
  ival[i++] = 0x226001;
  ival[i++] = INT_MAX;
  ival[i++] = INT_MIN;
  bin2txt_convert_i(ival, 1, i, 1, fp);
  convert_file_close(fp);

  fp = convert_file_bind(stdout);
  if (!fp) {
    return 1;
  }

  i = 0;
  lval[i++] = 0;
  lval[i++] = 1;
  lval[i++] = 2;
  lval[i++] = 2;
  lval[i++] = 9;
  lval[i++] = 10;
  lval[i++] = 15;
  lval[i++] = -1;
  lval[i++] = -5;
  lval[i++] = -5;
  lval[i++] = 1024;
  lval[i++] = -1024;
  lval[i++] = 1025;
  lval[i++] = -1025;
  lval[i++] = 1600;
  lval[i++] = -1600;
  lval[i++] = 2048;
  lval[i++] = 4096;
  lval[i++] = 0x80000;
  lval[i++] = 0x22000;
  lval[i++] = 0x22800;
  lval[i++] = 0x22400;
  lval[i++] = 0x228000;
  lval[i++] = 0x224000;
  lval[i++] = 0x226000;
  lval[i++] = 0x226001;
  lval[i++] = LONG_MAX;
  lval[i++] = (LONG_MAX >> 1) + 1;
  lval[i++] = LONG_MAX - 4095;
  lval[i++] = LONG_MIN;
  bin2txt_convert_l(lval, 1, i, 1, fp);
  convert_file_close(fp);

  fp = convert_file_bind(stdout);
  if (!fp) {
    return 1;
  }

  i = 0;
  uval[i++] = 0;
  uval[i++] = 1;
  uval[i++] = 2;
  uval[i++] = 2;
  uval[i++] = 9;
  uval[i++] = 10;
  uval[i++] = 15;
  uval[i++] = 1024;
  uval[i++] = 1025;
  uval[i++] = 1025;
  uval[i++] = 1600;
  uval[i++] = 2048;
  uval[i++] = 4096;
  uval[i++] = 0x80000;
  uval[i++] = 0x22000;
  uval[i++] = 0x22800;
  uval[i++] = 0x22400;
  uval[i++] = 0x228000;
  uval[i++] = 0x224000;
  uval[i++] = 0x226000;
  uval[i++] = 0x226001;
  uval[i++] = INT_MAX;
  uval[i++] = UINT_MAX;
  bin2txt_convert_u(uval, 1, i, 1, fp);
  convert_file_close(fp);

  return 0;
}
