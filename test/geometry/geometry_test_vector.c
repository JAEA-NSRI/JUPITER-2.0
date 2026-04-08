
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <float.h>

#include "test-util.h"

#include "geometry_test.h"

#include <jupiter/geometry/vector.h>
#include <jupiter/geometry/matrix.h>
#include <jupiter/geometry/quat.h>
#include <jupiter/geometry/common.h>

enum {
#ifdef DBL_DIG
  double_digits = DBL_DIG + 1,
#else
  double_digits = 16,
#endif
};

static int v2_comp_i(geom_vec2 a, geom_vec2 b,
                     const char *comp, const char *ca, const char *cb,
                     const char *file, long line, const char *fmt, ...)
{
  va_list ap;
  int stat;
  int r;
  char *m;

  va_start(ap, fmt);
  stat = test_compare_failv(geom_vec2_eql(a, b) == 0, file, line, comp, fmt, ap);
  va_end(ap);

  if (ca) {
    r = geom_vec2_to_str(&m, a, "%.*g", -1, double_digits);
    if (r >= 0) {
      fprintf(stderr, "..... %s = %s\n", ca, m);
      free(m);
    }
  }
  if (cb) {
    r = geom_vec2_to_str(&m, b, "%.*g", -1, double_digits);
    if (r >= 0) {
      fprintf(stderr, "..... %s = %s\n", cb, m);
      free(m);
    }
  }
  return stat;
}

#define test_vec2_comp_both_var(a, b, ...)                              \
  v2_comp_i(a, b, #a " == " #b, #a, #b, __FILE__, __LINE__, __VA_ARGS__)

#define test_vec2_comp(a, b, ...)                                       \
  v2_comp_i(a, b, #a " == " #b, #a, NULL, __FILE__, __LINE__, __VA_ARGS__)

static int v3_comp_i(geom_vec3 a, geom_vec3 b,
                     const char *comp, const char *ca, const char *cb,
                     const char *file, long line, const char *fmt, ...)
{
  va_list ap;
  int stat;
  int r;
  char *m;

  va_start(ap, fmt);
  stat = test_compare_failv(geom_vec3_eql(a, b) == 0, file, line, comp, fmt, ap);
  va_end(ap);

  if (ca) {
    r = geom_vec3_to_str(&m, a, "%.*g", -1, double_digits);
    if (r >= 0) {
      fprintf(stderr, "..... %s = %s\n", ca, m);
      free(m);
    }
  }
  if (cb) {
    r = geom_vec3_to_str(&m, b, "%.*g", -1, double_digits);
    if (r >= 0) {
      fprintf(stderr, "..... %s = %s\n", cb, m);
      free(m);
    }
  }
  return stat;
}

#define test_vec3_comp_both_var(a, b, ...)                              \
  v3_comp_i(a, b, #a " == " #b, #a, #b, __FILE__, __LINE__, __VA_ARGS__)

#define test_vec3_comp(a, b, ...)                                       \
  v3_comp_i(a, b, #a " == " #b, #a, NULL, __FILE__, __LINE__, __VA_ARGS__)


static int qr_comp_i(geom_quat a, geom_quat b,
                     const char *comp, const char *ca, const char *cb,
                     const char *file, long line, const char *fmt, ...)
{
  va_list ap;
  int stat;
  int r;
  char *m;

  va_start(ap, fmt);
  stat = test_compare_failv(geom_quat_eql(a, b) == 0, file, line, comp, fmt, ap);
  va_end(ap);

  if (ca) {
    r = geom_quat_to_str(&m, a, "%.*g", -1, double_digits);
    if (r >= 0) {
      fprintf(stderr, "..... %s = %s\n", ca, m);
      free(m);
    }
  }
  if (cb) {
    r = geom_quat_to_str(&m, b, "%.*g", -1, double_digits);
    if (r >= 0) {
      fprintf(stderr, "..... %s = %s\n", cb, m);
      free(m);
    }
  }
  return stat;
}

#define test_quat_comp_both_var(a, b, ...)                              \
  qr_comp_i(a, b, #a " == " #b, #a, #b, __FILE__, __LINE__, __VA_ARGS__)

#define test_quat_comp(a, b, ...)                                       \
  qr_comp_i(a, b, #a " == " #b, #a, NULL, __FILE__, __LINE__, __VA_ARGS__)

int vector_test(void)
{
  /*
   * If volatile is not used, these tests might not test actual
   * hardware; the compiler does compute these calculations and
   * replace the result with constant value while optimization. (605.0
   * instead of geom_vec2_length(c), for example)
   */
  volatile geom_vec2 a, b, c;
  volatile geom_vec3 v3, a3, b3, c3;
  volatile geom_mat43 m1, m2, m3, m4;
  volatile geom_mat33 m33_1, m33_2, m33_3, m33_4;
  volatile geom_quat q1, q2, q3;
  int ecnt = 0;

  fprintf(stderr, "\n---- vec2 add/sub/length ----\n");
  a = geom_vec2_c(257.0,  11.0);
  b = geom_vec2_c(141.0, 684.0);
  c = geom_vec2_add(a, b);
  if (test_vec2_comp(c, geom_vec2_c(398.0, 695.0), NULL)) ecnt++;

  a = geom_vec2_c( 35.0, 211.0);
  c = geom_vec2_sub(c, a);
  if (test_vec2_comp(c, geom_vec2_c(363.0, 484.0), NULL)) ecnt++;

  /* Result expressible in integer (i.e., pythagorean triple) */
  if (test_compare(geom_vec2_length(c), 605.0)) ecnt++;

  fprintf(stderr, "\n---- vec3 add/sub/length ----\n");

  //a3 = geom_vec3_c(12.0, 16.0, 21.0);
  a3 = geom_vec3_c(63.0, 11.0, -22.0);
  b3 = geom_vec3_c(16.0, -6.0,   7.0);
  c3 = geom_vec3_add(a3, b3);
  if (test_vec3_comp(c3, geom_vec3_c(79.0, 5.0, -15.0), NULL)) ecnt++;

  a3 = geom_vec3_c(67.0, -11.0, -36.0);
  c3 = geom_vec3_sub(c3, a3);
  if (test_vec3_comp(c3, geom_vec3_c(12.0, 16.0, 21.0), NULL)) ecnt++;

  if (test_compare_f(geom_vec3_length(c3), 29.0, "|c3| = %.*g",
                     double_digits, geom_vec3_length(c3))) ecnt++;

  c3 = geom_vec3_cross_prod(c3, b3);


  fprintf(stderr, "\n---- basic quaternions ----\n");
  q1 = geom_quat_c(1.0, 2.0, 3.0, 0.0);
  q2 = geom_quat_c(3.0, 2.0, 4.0, 1.0);
  q3 = geom_quat_mul(q1, q2);
  q1 = geom_quat_c(1.0, 2.0, 3.0, -19.0);
  q2 = geom_quat_c(2.0, 5.0, -4.0, 0.0);
  q1 = geom_quat_add(q1, q2);
  if (test_quat_comp_both_var(q1, q3, NULL)) ecnt++;

  fprintf(stderr, "\n---- quaternion rotations ----\n");
  q1 = geom_quat_rotation(geom_vec3_c(1.0, 1.0, 1.0), 30.0);
  q2 = geom_quat_rotation(geom_vec3_c(1.0, 1.0, 1.0), 90.0);
  q1 = geom_quat_mul(q1, q2);
  q3 = geom_quat_rotation(geom_vec3_c(1.0, 1.0, 1.0), 120.0);
  test_quat_comp_both_var(q1, q3, "(ignored; sin/cos precision is not predictable)");

  q2 = geom_quat_sub(q3, q1);
  test_compare_f(geom_quat_norm(q2), 0.0, "(ignored: ||q3-q1||=%.*g)",
                 double_digits, geom_quat_norm(q2));

  q1 = geom_vec3_get_rotation(geom_vec3_c(1.0, 1.0, 1.0),
                              geom_vec3_c(1.0, 1.0, 3.0));
  q3 = geom_quat_rotation(geom_vec3_c(2.0, -2.0, 0.0), 29.4962089496566432);
  test_quat_comp_both_var(q1, q3, "(ignored: ||q3-q1||=%.*g)", double_digits,
                          geom_quat_norm(geom_quat_sub(q3, q1)));

  q1 = geom_vec3_get_rotation(geom_vec3_c(1.0, 1.0, 3.0),
                              geom_vec3_c(2.0, 2.0, 6.0));
  q3 = geom_quat_rotation(geom_vec3_c(1.0, 0.0, 0.0), 0.0);
  test_quat_comp_both_var(q1, q3, "(ignored: ||q3-q1||=%.*g)", double_digits,
                          geom_quat_norm(geom_quat_sub(q3, q1)));

  q1 = geom_vec3_get_rotation(geom_vec3_c(1.0, 1.0, 3.0),
                              geom_vec3_c(-2.0, -2.0, -6.0));
  q3 = geom_quat_rotation(geom_vec3_c(10.0, -1.0, -3.0), 180.0);
  test_quat_comp_both_var(q1, q3, "(ignored: ||q3-q1||=%.*g)", double_digits,
                          geom_quat_norm(geom_quat_sub(q3, q1)));

  q1 = geom_vec3_get_rotation(geom_vec3_c(0.0, 2.0, 0.0),
                              geom_vec3_c(0.0, -1.0, 0.0));
  q3 = geom_quat_rotation(geom_vec3_c(1.0, 0.0, 0.0), 180.0);
  test_quat_comp_both_var(q1, q3, "(ignored: ||q3-q1||=%.*g)", double_digits,
                          geom_quat_norm(geom_quat_sub(q3, q1)));

  q1 = geom_vec3_get_rotation(geom_vec3_c(1.0, 0.0, 0.0),
                              geom_vec3_c(-2.0, 0.0, 0.0));
  q3 = geom_quat_rotation(geom_vec3_c(0.0, 1.0, 0.0), 180.0);
  test_quat_comp_both_var(q1, q3, "(ignored: ||q3-q1||=%.*g)", double_digits,
                          geom_quat_norm(geom_quat_sub(q3, q1)));

  q1 = geom_vec3_get_rotation(geom_vec3_c(-2.0, 0.0, 0.0),
                              geom_vec3_c(1.0, 0.0, 0.0));
  q3 = geom_quat_rotation(geom_vec3_c(0.0, 1.0, 0.0), 180.0);
  test_quat_comp_both_var(q1, q3, "(ignored: ||q3-q1||=%.*g)", double_digits,
                          geom_quat_norm(geom_quat_sub(q3, q1)));

  q1 = geom_vec3_get_rotation(geom_vec3_c(1.0, 0.0, 0.0),
                              geom_vec3_c(0.0, 0.0, 3.0));
  q3 = geom_quat_rotation(geom_vec3_c(0.0, -1.0, 0.0), 90.0);
  test_quat_comp_both_var(q1, q3, "(ignored: ||q3-q1||=%.*g)",
                          double_digits, geom_quat_norm(geom_quat_sub(q3, q1)));

  q1 = geom_vec3_get_rotation(geom_vec3_c(0.0, 1.0, 0.0),
                              geom_vec3_c(0.0, 0.0, 3.0));
  q3 = geom_quat_rotation(geom_vec3_c(1.0, 0.0, 0.0), 90.0);
  test_quat_comp_both_var(q1, q3, "(ignored: ||q3-q1||=%.*g)", double_digits,
                          geom_quat_norm(geom_quat_sub(q3, q1)));

  fprintf(stderr, "\n---- rotate (0, 0, 1) by 120deg around (1, 1, 1) ----\n");
  v3 = geom_quat_rotate_vp(geom_vec3_c(1.0, 1.0, 1.0), 120.0,
                           geom_vec3_c(0.0, 0.0, 1.0));
  test_vec3_comp(v3, geom_vec3_c(1.0, 0.0, 0.0), "(ignored; contains sqrt(3) in calc)");

  v3 = geom_vec3_sub(v3, geom_vec3_c(1.0, 0.0, 0.0));
  test_compare_f(geom_vec3_length(v3), 0.0,
                 "(ignored: distance to exp: %.*g)", double_digits,
                 geom_vec3_length(v3));

  /*
   * When inverse, the determinant of m3 is -4, so following oparations must
   * not be lossy for both of when double is binary or decimal.
   */
  fprintf(stderr, "\n---- 3x3 matrix inv/mul ----\n");
  m33_1 = geom_mat33_c(0, 1, -2, 2, 4, 6, 1, 3, 2);
  m33_2 = geom_mat33_c(4, 1,  0, 2, 2, 8, 6, 2, 3);
  m33_3 = geom_mat33_mul(m33_1, m33_2);
  if (test_compare(geom_mat33_det(m33_3), -4)) ecnt++;
  m33_4 = geom_mat33_inv(m33_3);
  m33_1 = geom_mat33_mul(m33_3, m33_4);
  if (test_compare(!!geom_mat33_eql(m33_1, geom_mat33_E()), 1)) ecnt++;

  fprintf(stderr, "\n---- 4x3 matrix inv/mul ----\n");
  m1 = geom_mat43_c(0, 1, -2, 1, 2, 4, 6, 3, 1, 3, 2, 1);
  m2 = geom_mat43_c(4, 1, 0, 3, 2, 2, 8, -2, 6, 2, 3, 1);
  m3 = geom_mat43_mul(m1, m2);
  if (test_compare(geom_mat43_det(m3), -4)) ecnt++;
  m4 = geom_mat43_inv(m3);
  m1 = geom_mat43_mul(m3, m4);
  if (test_compare(!!geom_mat43_eql(m1, geom_mat43_E()), 1)) ecnt++;

  return ecnt;
}
