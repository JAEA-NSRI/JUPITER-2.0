
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <inttypes.h>

#include "test-util.h"

#include "geometry_test.h"

#include <jupiter/geometry/polynomial.h>
#include <jupiter/geometry/svector.h>
#include <jupiter/geometry/common.h>

static int sv3_comp_i(geom_svec3 a, geom_svec3 b,
                      const char *comp, const char *ca, const char *cb,
                      const char *file, long line, const char *fmt, ...)
{
  va_list ap;
  int stat;
  int r;
  char *m;

  va_start(ap, fmt);
  stat = test_compare_failv(geom_svec3_eql(a, b) == 0, file, line, comp, fmt, ap);
  va_end(ap);

  if (ca) {
    r = geom_svec3_to_str(&m, a, -1, -1);
    if (r >= 0) {
      fprintf(stderr, "..... %s = %s\n", ca, m);
      free(m);
    }
  }
  if (cb) {
    r = geom_svec3_to_str(&m, b, -1, -1);
    if (r >= 0) {
      fprintf(stderr, "..... %s = %s\n", cb, m);
      free(m);
    }
  }
  return stat;
}

#define test_svec3_comp_both_var(a, b, ...)                             \
  sv3_comp_i(a, b, #a " == " #b, #a, #b, __FILE__, __LINE__, __VA_ARGS__)

#define test_svec3_comp(a, b, ...)                                      \
  sv3_comp_i(a, b, #a " == " #b, #a, NULL, __FILE__, __LINE__, __VA_ARGS__)

int polynomial_test(void)
{
  int ecnt;
  int r;
  geom_size_type i, j;
  char *m;
  geom_size_type nc;

  ecnt = 0;

  for (i = 0; i <= 40; ++i) {
    nc = geom_polynomial_coeff_count(i);
    for (j = 0; j < nc; ++j) {
      geom_svec3 cf, cn;
      geom_size_type l;
      cf = geom_polynomial_coeff_degree_at(i, j, NULL);
      l = geom_polynomial_coeff_loc(i, cf);
      cn = geom_polynomial_coeff_degree_at(i, l, NULL);
      r = geom_svec3_to_str(&m, cf, -1, -1);
      if (r >= 0) {
        fprintf(stderr, "..... Degree %2d: Loc %8" PRIdMAX " "
                "Rev: %8" PRIdMAX ": cf = %s\n",
                (int)i, (intmax_t)j, (intmax_t)l, m);
        free(m);
      }
      if (test_svec3_comp(cn, cf, "%" PRIdMAX " -- %" PRIdMAX,
                          (intmax_t)j, (intmax_t)l)) {
        ecnt++;
      }
    }
  }
  return ecnt;
}
