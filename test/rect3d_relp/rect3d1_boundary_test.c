#include <jupiter/common_util.h>
#include <jupiter/random/random.h>
#include <jupiter/rect3d_boundary.h>
#include <jupiter/rect3d_relp.h>
#include <jupiter/struct.h>
#include <math.h>

#include "rect3d_relp_test.h"
#include "test-util.h"

#define test_fail_if(expr) \
  do {                     \
    if (expr)              \
      r = 1;               \
  } while (0)

int rect3d1_boundary_test(void)
{
  domain cdo;
  int r = 0;
  type rt = HUGE_VAL;
  struct rect3d1p_boundary rp1, rp2;
  struct rect3d1p pp;
  jupiter_random_seed seed = {0x6573bbef816103ec, 0xf255631cee2c3bc6,
                              0xe769115a4e5a9aea, 0x7da8d65c8c396ce3};
  type *val = random_type_value(12, 13, 14, -3, &seed);

  if (!make_cdo_v(&cdo, 12, 13, 14, 3, 4,

                  [0] = -0.3, -0.2, -0.1, 0.0, 0.1, 0.3, 0.35, 0.4, 0.45, 0.5,
                  0.55, 0.6, 0.65,

                  [13] = 0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0,
                  11.0, 12.0, 13.0,

                  [27] = 0.47, 0.48, 0.49, 0.5, 0.51, 0.53, 0.55, 0.57, 0.6,
                  0.64, 0.70, 0.78, 0.88, 0.98, 1.08))
    return 1;

  /*
   * Interpolation coeffs:
   *
   *      xc              yc           zc
   *  0: -0.25            0.5          0.475
   *  1: -0.15            1.5          0.485
   *  2: -0.05 (W)        2.5(S)       0.495 (B)
   *  3:  0.05  1:0.05    3.5 const.   0.505  1:0.005
   *  4:  0.2   1:0.10    4.5          0.52   1:0.010
   *            0:0.075                       0:0.015
   *  5:  0.325 0:0.05    5.5          0.54   0:0.005
   *  6:  0.375           6.5          0.56  11:0.015
   *  7:  0.425(E)        7.5          0.585 11:0.01
   *  8:  0.475 const.    8.5(N)       0.62  10:0.01
   *  9:  0.525           9.5 const.   0.67  10:0.04
   * 10:  0.575          10.5          0.74  (T)
   * 11:  0.625          11.5          0.83
   * 12:                 12.5          0.93   {0.015, -0.005} (0.47)
   * 13:                               1.03   {0.115, -0.105} (0.37)
   */

  test_fail_if(!test_compare_dd(cdo.xv[0], ==, -0.3));
  test_fail_if(!test_compare_dd(cdo.xv[12], ==, 0.65));
  test_fail_if(!test_compare_dd(cdo.yv[0], ==, 0.0));
  test_fail_if(!test_compare_dd(cdo.yv[13], ==, 13.0));
  test_fail_if(!test_compare_dd(cdo.zv[0], ==, 0.47));
  test_fail_if(!test_compare_dd(cdo.zv[14], ==, 1.08));

  rect3d1p_boundary_Bc(&rp1, val, &cdo, 0, 0);
  test_fail_if(!test_compare_dd(rp1.vcoord, ==, 0.5));
  test_fail_if(
    !test_compare_poff(rp1.boundary.values, val, (2 * 13 + 0) * 12 + 0));
  test_fail_if(
    !test_compare_poff(rp1.interior.values, val, (3 * 13 + 0) * 12 + 0));
  test_fail_if(!test_compare_poff(rp1.boundary.axis.coords, cdo.zc, 2));
  test_fail_if(!test_compare_ii(rp1.boundary.axis.coffset, ==, -1));
  test_fail_if(!test_compare_ii(rp1.boundary.axis.index.voffset, ==, -12 * 13));
  test_fail_if(!test_compare_ii(rp1.boundary.axis.index.nfpoints, ==, 2));
  test_fail_if(!test_compare_ii(rp1.boundary.axis.index.nbpoints, ==, 11));
  test_fail_if(!test_compare_poff(rp1.interior.axis.coords, cdo.zc, 3));
  test_fail_if(!test_compare_ii(rp1.interior.axis.coffset, ==, 1));
  test_fail_if(!test_compare_ii(rp1.interior.axis.index.voffset, ==, 12 * 13));
  test_fail_if(!test_compare_ii(rp1.interior.axis.index.nfpoints, ==, 10));
  test_fail_if(!test_compare_ii(rp1.interior.axis.index.nbpoints, ==, 3));
  test_fail_if(!test_compare_eps(rect3d1p_getc(&rp1.boundary), 0.495, 1.0e-6));
  test_fail_if(!test_compare_eps(rect3d1p_getc(&rp1.interior), 0.505, 1.0e-6));

  test_fail_if(
    !test_compare_ii(rect3d1p_boundary_next(&rp1, 1, 1, &rt), ==, 1));

  test_fail_if(!test_compare_eps(rt, 0.515, 1.0e-6));
  test_fail_if(!test_compare_dd(rp1.vcoord, ==, 0.5));
  test_fail_if(
    !test_compare_poff(rp1.boundary.values, val, (1 * 13 + 0) * 12 + 0));
  test_fail_if(
    !test_compare_poff(rp1.interior.values, val, (4 * 13 + 0) * 12 + 0));
  test_fail_if(!test_compare_poff(rp1.boundary.axis.coords, cdo.zc, 1));
  test_fail_if(!test_compare_ii(rp1.boundary.axis.coffset, ==, -1));
  test_fail_if(!test_compare_ii(rp1.boundary.axis.index.voffset, ==, -12 * 13));
  test_fail_if(!test_compare_ii(rp1.boundary.axis.index.nfpoints, ==, 1));
  test_fail_if(!test_compare_ii(rp1.boundary.axis.index.nbpoints, ==, 12));
  test_fail_if(!test_compare_poff(rp1.interior.axis.coords, cdo.zc, 4));
  test_fail_if(!test_compare_ii(rp1.interior.axis.coffset, ==, 1));
  test_fail_if(!test_compare_ii(rp1.interior.axis.index.voffset, ==, 12 * 13));
  test_fail_if(!test_compare_ii(rp1.interior.axis.index.nfpoints, ==, 9));
  test_fail_if(!test_compare_ii(rp1.interior.axis.index.nbpoints, ==, 4));
  test_fail_if(!test_compare_eps(rect3d1p_getc(&rp1.boundary), 0.485, 1.0e-6));
  test_fail_if(!test_compare_eps(rect3d1p_getc(&rp1.interior), 0.52, 1.0e-6));

  test_fail_if(
    !test_compare_ii(rect3d1p_boundary_next(&rp1, 1, 1, &rt), ==, 1));

  test_fail_if(!test_compare_eps(rt, 0.525, 1.0e-6));
  test_fail_if(!test_compare_dd(rp1.vcoord, ==, 0.5));
  test_fail_if(
    !test_compare_poff(rp1.boundary.values, val, (0 * 13 + 0) * 12 + 0));
  test_fail_if(
    !test_compare_poff(rp1.interior.values, val, (4 * 13 + 0) * 12 + 0));
  test_fail_if(!test_compare_poff(rp1.boundary.axis.coords, cdo.zc, 0));
  test_fail_if(!test_compare_ii(rp1.boundary.axis.coffset, ==, -1));
  test_fail_if(!test_compare_ii(rp1.boundary.axis.index.voffset, ==, -12 * 13));
  test_fail_if(!test_compare_ii(rp1.boundary.axis.index.nfpoints, ==, 0));
  test_fail_if(!test_compare_ii(rp1.boundary.axis.index.nbpoints, ==, 13));
  test_fail_if(!test_compare_poff(rp1.interior.axis.coords, cdo.zc, 4));
  test_fail_if(!test_compare_ii(rp1.interior.axis.coffset, ==, 1));
  test_fail_if(!test_compare_ii(rp1.interior.axis.index.voffset, ==, 12 * 13));
  test_fail_if(!test_compare_ii(rp1.interior.axis.index.nfpoints, ==, 9));
  test_fail_if(!test_compare_ii(rp1.interior.axis.index.nbpoints, ==, 4));
  test_fail_if(!test_compare_eps(rect3d1p_getc(&rp1.boundary), 0.475, 1.0e-6));
  test_fail_if(!test_compare_eps(rect3d1p_getc(&rp1.interior), 0.52, 1.0e-6));

  test_fail_if(
    !test_compare_ii(rect3d1p_boundary_next(&rp1, 1, 1, &rt), ==, 0));

  rect3d1p_boundary_Bv(&rp1, val, &cdo, 5, 4);
  test_fail_if(!test_compare_dd(rp1.vcoord, ==, 0.5));
  test_fail_if(
    !test_compare_poff(rp1.boundary.values, val, (3 * 13 + 4) * 12 + 5));
  test_fail_if(
    !test_compare_poff(rp1.interior.values, val, (3 * 13 + 4) * 12 + 5));
  test_fail_if(!test_compare_poff(rp1.boundary.axis.coords, cdo.zv, 3));
  test_fail_if(!test_compare_ii(rp1.boundary.axis.coffset, ==, -1));
  test_fail_if(!test_compare_ii(rp1.boundary.axis.index.voffset, ==, -12 * 13));
  test_fail_if(!test_compare_ii(rp1.boundary.axis.index.nfpoints, ==, 3));
  test_fail_if(!test_compare_ii(rp1.boundary.axis.index.nbpoints, ==, 10));
  test_fail_if(!test_compare_poff(rp1.interior.axis.coords, cdo.zv, 3));
  test_fail_if(!test_compare_ii(rp1.interior.axis.coffset, ==, 1));
  test_fail_if(!test_compare_ii(rp1.interior.axis.index.voffset, ==, 12 * 13));
  test_fail_if(!test_compare_ii(rp1.interior.axis.index.nfpoints, ==, 10));
  test_fail_if(!test_compare_ii(rp1.interior.axis.index.nbpoints, ==, 3));
  test_fail_if(!test_compare_dd(rect3d1p_getc(&rp1.boundary), ==, 0.5));
  test_fail_if(!test_compare_dd(rect3d1p_getc(&rp1.interior), ==, 0.5));

  test_fail_if(
    !test_compare_ii(rect3d1p_boundary_next(&rp1, 1, 1, &rt), ==, 1));

  test_fail_if(!test_compare_eps(rt, 0.51, 1.0e-6));
  test_fail_if(!test_compare_dd(rp1.vcoord, ==, 0.5));
  test_fail_if(
    !test_compare_poff(rp1.boundary.values, val, (2 * 13 + 4) * 12 + 5));
  test_fail_if(
    !test_compare_poff(rp1.interior.values, val, (4 * 13 + 4) * 12 + 5));
  test_fail_if(!test_compare_poff(rp1.boundary.axis.coords, cdo.zv, 2));
  test_fail_if(!test_compare_ii(rp1.boundary.axis.coffset, ==, -1));
  test_fail_if(!test_compare_ii(rp1.boundary.axis.index.voffset, ==, -12 * 13));
  test_fail_if(!test_compare_ii(rp1.boundary.axis.index.nfpoints, ==, 2));
  test_fail_if(!test_compare_ii(rp1.boundary.axis.index.nbpoints, ==, 11));
  test_fail_if(!test_compare_poff(rp1.interior.axis.coords, cdo.zv, 4));
  test_fail_if(!test_compare_ii(rp1.interior.axis.coffset, ==, 1));
  test_fail_if(!test_compare_ii(rp1.interior.axis.index.voffset, ==, 12 * 13));
  test_fail_if(!test_compare_ii(rp1.interior.axis.index.nfpoints, ==, 9));
  test_fail_if(!test_compare_ii(rp1.interior.axis.index.nbpoints, ==, 4));
  test_fail_if(!test_compare_dd(rect3d1p_getc(&rp1.boundary), ==, 0.49));
  test_fail_if(!test_compare_dd(rect3d1p_getc(&rp1.interior), ==, 0.51));

  test_fail_if(
    !test_compare_ii(rect3d1p_boundary_next(&rp1, 1, 1, &rt), ==, 1));

  test_fail_if(!test_compare_eps(rt, 0.52, 1.0e-6));
  test_fail_if(!test_compare_dd(rp1.vcoord, ==, 0.5));
  test_fail_if(
    !test_compare_poff(rp1.boundary.values, val, (1 * 13 + 4) * 12 + 5));
  test_fail_if(
    !test_compare_poff(rp1.interior.values, val, (4 * 13 + 4) * 12 + 5));
  test_fail_if(!test_compare_poff(rp1.boundary.axis.coords, cdo.zv, 1));
  test_fail_if(!test_compare_ii(rp1.boundary.axis.coffset, ==, -1));
  test_fail_if(!test_compare_ii(rp1.boundary.axis.index.voffset, ==, -12 * 13));
  test_fail_if(!test_compare_ii(rp1.boundary.axis.index.nfpoints, ==, 1));
  test_fail_if(!test_compare_ii(rp1.boundary.axis.index.nbpoints, ==, 12));
  test_fail_if(!test_compare_poff(rp1.interior.axis.coords, cdo.zv, 4));
  test_fail_if(!test_compare_ii(rp1.interior.axis.coffset, ==, 1));
  test_fail_if(!test_compare_ii(rp1.interior.axis.index.voffset, ==, 12 * 13));
  test_fail_if(!test_compare_ii(rp1.interior.axis.index.nfpoints, ==, 9));
  test_fail_if(!test_compare_ii(rp1.interior.axis.index.nbpoints, ==, 4));
  test_fail_if(!test_compare_dd(rect3d1p_getc(&rp1.boundary), ==, 0.48));
  test_fail_if(!test_compare_dd(rect3d1p_getc(&rp1.interior), ==, 0.51));

  test_fail_if(
    !test_compare_ii(rect3d1p_boundary_next(&rp1, 1, 1, &rt), ==, 1));

  test_fail_if(!test_compare_eps(rt, 0.53, 1.0e-6));
  test_fail_if(!test_compare_dd(rp1.vcoord, ==, 0.5));
  test_fail_if(
    !test_compare_poff(rp1.boundary.values, val, (0 * 13 + 4) * 12 + 5));
  test_fail_if(
    !test_compare_poff(rp1.interior.values, val, (5 * 13 + 4) * 12 + 5));
  test_fail_if(!test_compare_poff(rp1.boundary.axis.coords, cdo.zv, 0));
  test_fail_if(!test_compare_ii(rp1.boundary.axis.coffset, ==, -1));
  test_fail_if(!test_compare_ii(rp1.boundary.axis.index.voffset, ==, -12 * 13));
  test_fail_if(!test_compare_ii(rp1.boundary.axis.index.nfpoints, ==, 0));
  test_fail_if(!test_compare_ii(rp1.boundary.axis.index.nbpoints, ==, 13));
  test_fail_if(!test_compare_poff(rp1.interior.axis.coords, cdo.zv, 5));
  test_fail_if(!test_compare_ii(rp1.interior.axis.coffset, ==, 1));
  test_fail_if(!test_compare_ii(rp1.interior.axis.index.voffset, ==, 12 * 13));
  test_fail_if(!test_compare_ii(rp1.interior.axis.index.nfpoints, ==, 8));
  test_fail_if(!test_compare_ii(rp1.interior.axis.index.nbpoints, ==, 5));
  test_fail_if(!test_compare_dd(rect3d1p_getc(&rp1.boundary), ==, 0.47));
  test_fail_if(!test_compare_dd(rect3d1p_getc(&rp1.interior), ==, 0.53));

  test_fail_if(
    !test_compare_ii(rect3d1p_boundary_next(&rp1, 1, 1, &rt), ==, 0));

#define texpnd(...) __VA_ARGS__
#define pexpnd(x) texpnd x
#define tcaddr(i, j, k, nx, ny) (k * ny + j) * nx + i
#define tcaddrX(i, u, v, nx, ny) tcaddr(i, u, v, nx, ny)
#define tcaddrY(i, u, v, nx, ny) tcaddr(u, i, v, nx, ny)
#define tcaddrZ(i, u, v, nx, ny) tcaddr(u, v, i, nx, ny)
#define tecar(x, ...) x
#define ttcar(...) tecar(__VA_ARGS__)
#define tcar(x) ttcar(pexpnd(x))
#define tecdr(x, ...) (__VA_ARGS__)
#define ttcdr(...) tecdr(__VA_ARGS__)
#define tcdr(x) ttcdr(pexpnd(x))

#define testvaleps(var, i, val, eps) \
  test_fail_if(!test_compare_eps(var[i], val, eps));
#define testvalb(var, ax, u, v, nx, ny, eps, i, val) \
  testvaleps(var, tcaddr##ax(i, u, v, nx, ny), val, eps);
#define testval1(var, ax, u, v, nx, ny, eps, ia, va) \
  testvalb(var, ax, u, v, nx, ny, eps, pexpnd(ia), pexpnd(va))
#define testval2(var, ax, u, v, nx, ny, eps, ia, va)        \
  testvalb(var, ax, u, v, nx, ny, eps, tcar(ia), tcar(va)); \
  testval1(var, ax, u, v, nx, ny, eps, tcdr(ia), tcdr(va));
#define testval3(var, ax, u, v, nx, ny, eps, ia, va)        \
  testvalb(var, ax, u, v, nx, ny, eps, tcar(ia), tcar(va)); \
  testval2(var, ax, u, v, nx, ny, eps, tcdr(ia), tcdr(va));
#define testval4(var, ax, u, v, nx, ny, eps, ia, va)        \
  testvalb(var, ax, u, v, nx, ny, eps, tcar(ia), tcar(va)); \
  testval3(var, ax, u, v, nx, ny, eps, tcdr(ia), tcdr(va));
#define testval5(var, ax, u, v, nx, ny, eps, ia, va)        \
  testvalb(var, ax, u, v, nx, ny, eps, tcar(ia), tcar(va)); \
  testval4(var, ax, u, v, nx, ny, eps, tcdr(ia), tcdr(va));
#define testval6(var, ax, u, v, nx, ny, eps, ia, va)        \
  testvalb(var, ax, u, v, nx, ny, eps, tcar(ia), tcar(va)); \
  testval5(var, ax, u, v, nx, ny, eps, tcdr(ia), tcdr(va));
#define testval7(var, ax, u, v, nx, ny, eps, ia, va)        \
  testvalb(var, ax, u, v, nx, ny, eps, tcar(ia), tcar(va)); \
  testval6(var, ax, u, v, nx, ny, eps, tcdr(ia), tcdr(va));
#define testval8(var, ax, u, v, nx, ny, eps, ia, va)        \
  testvalb(var, ax, u, v, nx, ny, eps, tcar(ia), tcar(va)); \
  testval7(var, ax, u, v, nx, ny, eps, tcdr(ia), tcdr(va));
#define testval9(var, ax, u, v, nx, ny, eps, ia, va)        \
  testvalb(var, ax, u, v, nx, ny, eps, tcar(ia), tcar(va)); \
  testval8(var, ax, u, v, nx, ny, eps, tcdr(ia), tcdr(va));
#define testval10(var, ax, u, v, nx, ny, eps, ia, va)       \
  testvalb(var, ax, u, v, nx, ny, eps, tcar(ia), tcar(va)); \
  testval9(var, ax, u, v, nx, ny, eps, tcdr(ia), tcdr(va));

#define cn(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, x, ...) x
#define ta(...) cn(__VA_ARGS__, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1)
#define tx(x) ta(x)
#define tn(x) tx(pexpnd(x))
#define testvaln2(var, ax, u, v, nx, ny, eps, n, ...) \
  testval##n(var, ax, u, v, nx, ny, eps, __VA_ARGS__)
#define testvaln1(var, ax, u, v, nx, ny, eps, n, ...) \
  testvaln2(var, ax, u, v, nx, ny, eps, n, __VA_ARGS__)
#define testvaln(var, ax, u, v, nx, ny, eps, ia, va) \
  testvaln1(var, ax, u, v, nx, ny, eps, tn(ia), ia, va)

  test_fail_if(
    !test_compare_ii(rect3d1p_boundary_Wc(&rp1, val, &cdo, 5, 1), ==, 1));
  testvaln(val, X, 5, 1, 12, 13, 1.0e-6, (0, 1, 2, 3, 4, 5, 6, 7),
           (224.5, 245.0, 244.5, 252.0, 51.625, 257.625, 146.75, 161.5));

  rect3d1p_boundary_line_symmetry_scalar(&rp1, 3, rect3d1_interp_linear, NULL);

  testvaln(val, X, 5, 1, 12, 13, 1.0e-6, (0, 1, 2, 3, 4, 5, 6, 7),
           (134.025, 355.25 / 3.0, 252.0, 252.0, 51.625, 257.625, 146.75,
            161.5));

  test_fail_if(
    !test_compare_ii(rect3d1p_boundary_Ec(&rp1, val, &cdo, 0, 4), ==, 1));
  testvaln(val, X, 0, 4, 12, 13, 1.0e-6, (4, 5, 6, 7, 8, 9, 10, 11),
           (9.75, 113.25, 165.25, 239.0, 99.375, 146.25, 138.5, 54.25));

  rect3d1p_boundary_line_symmetry_scalar(&rp1, 3, rect3d1_interp_linear, NULL);

  testvaln(val, X, 0, 4, 12, 13, 1.0e-6, (4, 5, 6, 7, 8, 9, 10, 11),
           (9.75, 113.25, 165.25, 239.0, 239.0, 165.25, 113.25, 54.25));

  test_fail_if(
    !test_compare_ii(rect3d1p_boundary_Sc(&rp1, val, &cdo, 2, 8), ==, 1));
  testvaln(val, Y, 2, 8, 12, 13, 1.0e-6, (0, 1, 2, 3, 4, 5, 6, 7),
           (23.625, 12.875, 272.5, 19.25, 139.5, 72.75, 115.375, 145.875));

  rect3d1p_boundary_line_symmetry_scalar(&rp1, 3, rect3d1_interp_linear, NULL);
  testvaln(val, Y, 2, 8, 12, 13, 1.0e-6, (0, 1, 2, 3, 4, 5, 6, 7),
           (72.75, 139.5, 19.25, 19.25, 139.5, 72.75, 115.375, 145.875));

  test_fail_if(
    !test_compare_ii(rect3d1p_boundary_Nc(&rp1, val, &cdo, 3, 4), ==, 1));
  testvaln(val, Y, 3, 4, 12, 13, 1.0e-6, (5, 6, 7, 8, 9, 10, 11, 12),
           (131.0, 250.625, 34.75, 36.25, 71.125, 38.125, 69.75, 267.0));

  rect3d1p_boundary_line_symmetry_scalar(&rp1, 3, rect3d1_interp_linear, NULL);
  testvaln(val, Y, 3, 4, 12, 13, 1.0e-6, (5, 6, 7, 8, 9, 10, 11, 12),
           (131.0, 250.625, 34.75, 36.25, 36.25, 34.75, 250.625, 267.0));

  test_fail_if(
    !test_compare_ii(rect3d1p_boundary_Bc(&rp1, val, &cdo, 9, 5), ==, 1));
  testvaln(val, Z, 9, 5, 12, 13, 1.0e-6, (0, 1, 2, 3, 4, 5, 6, 7),
           (112.5, 79.625, 265.0, 225.75, 70.0, 185.0, 33.125, 214.375));

  rect3d1p_boundary_line_symmetry_scalar(&rp1, 3, rect3d1_interp_linear, NULL);
  testvaln(val, Z, 9, 5, 12, 13, 1.0e-6, (0, 1, 2, 3, 4, 5, 6, 7),
           (98.75, 365.75 / 3.0, 225.75, 225.75, 70.0, 185.0, 33.125, 214.375));

  test_fail_if(
    !test_compare_ii(rect3d1p_boundary_Tc(&rp1, val, &cdo, 3, 8), ==, 1));
  testvaln(val, Z, 3, 8, 12, 13, 1.0e-6, (0, 1, 6, 7, 8, 9, 10, 11, 12, 13),
           (242.5, 265.25, 87.0, 136.125, 74.125, 56.875, 224.75, 180.5,
            212.125, 164.375));

  rect3d1p_boundary_line_symmetry_scalar(&rp1, 3, rect3d1_interp_linear, NULL);
  testvaln(val, Z, 3, 8, 12, 13, 1.0e-6, (0, 1, 6, 7, 8, 9, 10, 11, 12, 13),
           (242.5, 265.25, 87.0, 136.125, 74.125, 56.875, 60.325, 106.65,
            231.125, 164.375));

  test_fail_if(
    !test_compare_ii(rect3d1p_boundary_Ec(&rp1, val, &cdo, 4, 8), ==, 1));

  test_fail_if(!test_compare_eps(val[(8 * 13 + 4) * 12 + 11], 50.0, 1.0e-6));
  test_fail_if(!test_compare_eps(val[(8 * 13 + 4) * 12 + 12], 103.75, 1.0e-6));

  rect3d1p_boundary_line_symmetry_scalar(&rp1, 99, rect3d1_interp_linear, NULL);

  test_fail_if(!test_compare_eps(val[(8 * 13 + 4) * 12 + 11], 189.575, 1.0e-6));
  test_fail_if(!test_compare_eps(val[(8 * 13 + 4) * 12 + 12], 103.75, 1.0e-6));

  test_fail_if(
    !test_compare_ii(rect3d1p_boundary_Wv(&rp1, val, &cdo, 9, 1), ==, 1));
  testvaln(val, X, 9, 1, 12, 13, 1.0e-6, (0, 1, 2, 3, 4, 5, 6, 7),
           (240.875, 83.25, 211.875, 230.125, 71.875, 104.5, 264.625, 44.5));

  rect3d1p_boundary_line_symmetry_stgperp(&rp1, 3, rect3d1_interp_linear, NULL);

  testvaln(val, X, 9, 1, 12, 13, 1.0e-6, (0, 1, 2, 3, 4, 5, 6, 7),
           (104.5, 88.1875, 71.875, 230.125, 71.875, 104.5, 264.625, 44.5));

  test_fail_if(
    !test_compare_ii(rect3d1p_boundary_Ev(&rp1, val, &cdo, 1, 9), ==, 1));
  testvaln(val, X, 1, 9, 12, 13, 1.0e-6, (4, 5, 6, 7, 8, 9, 10, 11),
           (214.625, 51.75, 253.375, 83.125, 203.125, 195.875, 135.375,
            68.375));

  rect3d1p_boundary_line_symmetry_stgperp(&rp1, 3, rect3d1_interp_linear, NULL);

  testvaln(val, X, 1, 9, 12, 13, 1.0e-6, (4, 5, 6, 7, 8, 9, 10, 11),
           (214.625, 51.75, 253.375, 83.125, 203.125, 83.125, 253.375, 51.75));

  test_fail_if(
    !test_compare_ii(rect3d1p_boundary_Sv(&rp1, val, &cdo, 8, 5), ==, 1));
  testvaln(val, Y, 8, 5, 12, 13, 1.0e-6, (0, 1, 2, 3, 4, 5, 6, 7),
           (122.25, 82.875, 130.5, 4.125, 207.0, 37.625, 124.5, 0.75));

  rect3d1p_boundary_line_symmetry_stgperp(&rp1, 3, rect3d1_interp_linear, NULL);

  testvaln(val, Y, 8, 5, 12, 13, 1.0e-6, (0, 1, 2, 3, 4, 5, 6, 7),
           (124.5, 37.625, 207.0, 4.125, 207.0, 37.625, 124.5, 0.75));

  test_fail_if(
    !test_compare_ii(rect3d1p_boundary_Nv(&rp1, val, &cdo, 4, 3), ==, 1));
  testvaln(val, Y, 4, 3, 12, 13, 1.0e-6, (5, 6, 7, 8, 9, 10, 11, 12),
           (193.25, 258.25, 102.75, 160.875, 139.0, 114.75, 136.375, 246.625));

  rect3d1p_boundary_line_symmetry_stgperp(&rp1, 3, rect3d1_interp_linear, NULL);

  testvaln(val, Y, 4, 3, 12, 13, 1.0e-6, (5, 6, 7, 8, 9, 10, 11, 12),
           (193.25, 258.25, 102.75, 160.875, 139.0, 160.875, 102.75, 258.25));

  test_fail_if(
    !test_compare_ii(rect3d1p_boundary_Bv(&rp1, val, &cdo, 3, 5), ==, 1));
  testvaln(val, Z, 3, 5, 12, 13, 1.0e-6, (0, 1, 2, 3, 4, 5, 6, 7),
           (175.125, 252.0, 210.5, 4.0, 131.0, 146.125, 189.0, 32.25));

  rect3d1p_boundary_line_symmetry_stgperp(&rp1, 3, rect3d1_interp_linear, NULL);

  testvaln(val, Z, 3, 5, 12, 13, 1.0e-6, (0, 1, 2, 3, 4, 5, 6, 7),
           (146.125, 138.5625, 131.0, 4.0, 131.0, 146.125, 189.0, 32.25));

  test_fail_if(
    !test_compare_ii(rect3d1p_boundary_Tv(&rp1, val, &cdo, 7, 9), ==, 1));
  testvaln(val, Z, 7, 9, 12, 13, 1.0e-6, (0, 1, 4, 5, 8, 9, 10, 11, 12, 13),
           (90.75, 44.5, 65.625, 179.625, 120.625, 138.125, 156.125, 214.125,
            11.25, 241.0));

  rect3d1p_boundary_line_symmetry_stgperp(&rp1, 3, rect3d1_interp_linear, NULL);

  testvaln(val, Z, 7, 9, 12, 13, 1.0e-6, (0, 1, 4, 5, 8, 9, 10, 11, 12, 13),
           (90.75, 44.5, 65.625, 179.625, 120.625, 138.125, 156.125, 129.375,
            122.625, 322.0));

  test_fail_if(
    !test_compare_ii(rect3d1p_boundary_Wc(&rp1, val, &cdo, 2, 3), ==, 1));
  testvaln(val, X, 2, 3, 12, 13, 1.0e-6, (0, 1, 2, 3, 4, 5, 6, 7),
           (123.75, 49.875, 170.25, 19.75, 220.375, 135.5, 13.75, 133.875));

  rect3d1p_boundary_point_symmetry_scalar(&rp1, 3, 100.0, rect3d1_interp_linear,
                                          NULL);

  testvaln(val, X, 2, 3, 12, 13, 1.0e-6, (0, 1, 2, 3, 4, 5, 6, 7),
           (13.575, 46.5, 180.25, 19.75, 220.375, 135.5, 13.75, 133.875));

  test_fail_if(
    !test_compare_ii(rect3d1p_boundary_Ec(&rp1, val, &cdo, 7, 8), ==, 1));
  testvaln(val, X, 7, 8, 12, 13, 1.0e-6, (4, 5, 6, 7, 8, 9, 10, 11),
           (189.375, 59.875, 6.5, 66.125, 85.625, 150.625, 34.5, 75.25));

  rect3d1p_boundary_point_symmetry_scalar(&rp1, 3, 100.0, rect3d1_interp_linear,
                                          NULL);

  testvaln(val, X, 7, 8, 12, 13, 1.0e-6, (4, 5, 6, 7, 8, 9, 10, 11),
           (189.375, 59.875, 6.5, 66.125, 133.875, 193.5, 140.125, 75.25));

  return r;
}
