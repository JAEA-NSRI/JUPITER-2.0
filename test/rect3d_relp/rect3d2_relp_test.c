#include <jupiter/common_util.h>
#include <jupiter/random/random.h>
#include <jupiter/rect3d_relp.h>

#include "rect3d_relp_test.h"
#include "test-util.h"

#define test_fail_if(expr) \
  do {                     \
    if (expr)              \
      r = 1;               \
  } while (0)

int rect3d2_relp_test(void)
{
  domain cdo;
  int r = 0;
  struct rect3d2p rp1, rp2, rp3;
  struct rect3d1p rp11;
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

  test_fail_if(!test_compare_dd(cdo.xv[0], ==, -0.3));
  test_fail_if(!test_compare_dd(cdo.xv[12], ==, 0.65));
  test_fail_if(!test_compare_dd(cdo.yv[0], ==, 0.0));
  test_fail_if(!test_compare_dd(cdo.yv[13], ==, 13.0));
  test_fail_if(!test_compare_dd(cdo.zv[0], ==, 0.47));
  test_fail_if(!test_compare_dd(cdo.zv[14], ==, 1.08));

  rp1 = rect3d2p_XYccm(val, 3, 3, 3, &cdo, 1, 1);
  test_fail_if(!test_compare_poff(rp1.values, val, (3 * 13 + 3) * 12 + 3));
  test_fail_if(!test_compare_ii(rp1.right.coffset, ==, 1));
  test_fail_if(!test_compare_poff(rp1.right.coords, cdo.xc, 3));
  test_fail_if(!test_compare_ii(rp1.right.index.voffset, ==, 1));
  test_fail_if(!test_compare_ii(rp1.right.index.nbpoints, ==, 3));
  test_fail_if(!test_compare_ii(rp1.right.index.nfpoints, ==, 8));
  test_fail_if(!test_compare_ii(rp1.up.coffset, ==, 1));
  test_fail_if(!test_compare_poff(rp1.up.coords, cdo.yc, 3));
  test_fail_if(!test_compare_ii(rp1.up.index.voffset, ==, 12));
  test_fail_if(!test_compare_ii(rp1.up.index.nbpoints, ==, 3));
  test_fail_if(!test_compare_ii(rp1.up.index.nfpoints, ==, 9));
  test_fail_if(!test_compare_eps(rect3d2p_getcR(&rp1), 0.05, 1e-6));
  test_fail_if(!test_compare_eps(rect3d2p_getcU(&rp1), 3.5, 1e-6));
  test_fail_if(!test_compare_dd(rect3d2p_getv(&rp1), ==, 114.125));
  test_fail_if(!test_compare_dd(val[(3 * 13 + 3) * 12 + 3], ==, 114.125));

  test_fail_if(!test_compare_ii(rect3d2p_move(&rp1, 1, 0), ==, 1));
  test_fail_if(!test_compare_poff(rp1.values, val, (3 * 13 + 3) * 12 + 4));
  test_fail_if(!test_compare_ii(rp1.right.coffset, ==, 1));
  test_fail_if(!test_compare_poff(rp1.right.coords, cdo.xc, 4));
  test_fail_if(!test_compare_ii(rp1.right.index.voffset, ==, 1));
  test_fail_if(!test_compare_ii(rp1.right.index.nbpoints, ==, 4));
  test_fail_if(!test_compare_ii(rp1.right.index.nfpoints, ==, 7));
  test_fail_if(!test_compare_ii(rp1.up.coffset, ==, 1));
  test_fail_if(!test_compare_poff(rp1.up.coords, cdo.yc, 3));
  test_fail_if(!test_compare_ii(rp1.up.index.voffset, ==, 12));
  test_fail_if(!test_compare_ii(rp1.up.index.nbpoints, ==, 3));
  test_fail_if(!test_compare_ii(rp1.up.index.nfpoints, ==, 9));
  test_fail_if(!test_compare_eps(rect3d2p_getcR(&rp1), 0.2, 1e-6));
  test_fail_if(!test_compare_eps(rect3d2p_getcU(&rp1), 3.5, 1e-6));
  test_fail_if(!test_compare_dd(rect3d2p_getv(&rp1), ==, 121.125));
  test_fail_if(!test_compare_dd(val[(3 * 13 + 3) * 12 + 4], ==, 121.125));

  rect3d2p_move(&rp1, 4, 3);
  test_fail_if(!test_compare_poff(rp1.values, val, (3 * 13 + 6) * 12 + 8));
  test_fail_if(!test_compare_ii(rp1.right.coffset, ==, 1));
  test_fail_if(!test_compare_poff(rp1.right.coords, cdo.xc, 8));
  test_fail_if(!test_compare_ii(rp1.right.index.voffset, ==, 1));
  test_fail_if(!test_compare_ii(rp1.right.index.nbpoints, ==, 8));
  test_fail_if(!test_compare_ii(rp1.right.index.nfpoints, ==, 3));
  test_fail_if(!test_compare_ii(rp1.up.coffset, ==, 1));
  test_fail_if(!test_compare_poff(rp1.up.coords, cdo.yc, 6));
  test_fail_if(!test_compare_ii(rp1.up.index.voffset, ==, 12));
  test_fail_if(!test_compare_ii(rp1.up.index.nbpoints, ==, 6));
  test_fail_if(!test_compare_ii(rp1.up.index.nfpoints, ==, 6));
  test_fail_if(!test_compare_eps(rect3d2p_getcR(&rp1), 0.475, 1e-6));
  test_fail_if(!test_compare_eps(rect3d2p_getcU(&rp1), 6.5, 1e-6));
  test_fail_if(!test_compare_dd(rect3d2p_getv(&rp1), ==, 80.75));
  test_fail_if(!test_compare_dd(val[(3 * 13 + 6) * 12 + 8], ==, 80.75));
  test_fail_if(!test_compare_dd(rect3d2p_getnv(&rp1, -4, -3), ==, 121.125));

  rect3d2p_moveR(&rp1, -3);
  test_fail_if(!test_compare_poff(rp1.values, val, (3 * 13 + 6) * 12 + 5));
  test_fail_if(!test_compare_ii(rp1.right.coffset, ==, 1));
  test_fail_if(!test_compare_poff(rp1.right.coords, cdo.xc, 5));
  test_fail_if(!test_compare_ii(rp1.right.index.voffset, ==, 1));
  test_fail_if(!test_compare_ii(rp1.right.index.nbpoints, ==, 5));
  test_fail_if(!test_compare_ii(rp1.right.index.nfpoints, ==, 6));
  test_fail_if(!test_compare_ii(rp1.up.coffset, ==, 1));
  test_fail_if(!test_compare_poff(rp1.up.coords, cdo.yc, 6));
  test_fail_if(!test_compare_ii(rp1.up.index.voffset, ==, 12));
  test_fail_if(!test_compare_ii(rp1.up.index.nbpoints, ==, 6));
  test_fail_if(!test_compare_ii(rp1.up.index.nfpoints, ==, 6));
  test_fail_if(!test_compare_eps(rect3d2p_getcR(&rp1), 0.325, 1e-6));
  test_fail_if(!test_compare_eps(rect3d2p_getcU(&rp1), 6.5, 1e-6));
  test_fail_if(!test_compare_dd(rect3d2p_getv(&rp1), ==, 222.375));
  test_fail_if(!test_compare_dd(val[(3 * 13 + 6) * 12 + 5], ==, 222.375));
  test_fail_if(!test_compare_dd(rect3d2p_getnvR(&rp1, 3), ==, 80.75));

  rect3d2p_moveU(&rp1, 1);
  test_fail_if(!test_compare_poff(rp1.values, val, (3 * 13 + 7) * 12 + 5));
  test_fail_if(!test_compare_ii(rp1.right.coffset, ==, 1));
  test_fail_if(!test_compare_poff(rp1.right.coords, cdo.xc, 5));
  test_fail_if(!test_compare_ii(rp1.right.index.voffset, ==, 1));
  test_fail_if(!test_compare_ii(rp1.right.index.nbpoints, ==, 5));
  test_fail_if(!test_compare_ii(rp1.right.index.nfpoints, ==, 6));
  test_fail_if(!test_compare_ii(rp1.up.coffset, ==, 1));
  test_fail_if(!test_compare_poff(rp1.up.coords, cdo.yc, 7));
  test_fail_if(!test_compare_ii(rp1.up.index.voffset, ==, 12));
  test_fail_if(!test_compare_ii(rp1.up.index.nbpoints, ==, 7));
  test_fail_if(!test_compare_ii(rp1.up.index.nfpoints, ==, 5));
  test_fail_if(!test_compare_eps(rect3d2p_getcR(&rp1), 0.325, 1e-6));
  test_fail_if(!test_compare_eps(rect3d2p_getcU(&rp1), 7.5, 1e-6));
  test_fail_if(!test_compare_dd(rect3d2p_getv(&rp1), ==, 247.375));
  test_fail_if(!test_compare_dd(val[(3 * 13 + 7) * 12 + 5], ==, 247.375));
  test_fail_if(!test_compare_dd(rect3d2p_getnvU(&rp1, -1), ==, 222.375));

  rp1 = rect3d2p_ZXcvm(val, 6, 2, 4, &cdo, 1, -1);
  test_fail_if(!test_compare_poff(rp1.values, val, (4 * 13 + 2) * 12 + 6));
  test_fail_if(!test_compare_ii(rp1.right.coffset, ==, 1));
  test_fail_if(!test_compare_poff(rp1.right.coords, cdo.zc, 4));
  test_fail_if(!test_compare_ii(rp1.right.index.voffset, ==, 12 * 13));
  test_fail_if(!test_compare_ii(rp1.right.index.nbpoints, ==, 4));
  test_fail_if(!test_compare_ii(rp1.right.index.nfpoints, ==, 9));
  test_fail_if(!test_compare_ii(rp1.up.coffset, ==, -1));
  test_fail_if(!test_compare_poff(rp1.up.coords, cdo.xv, 6));
  test_fail_if(!test_compare_ii(rp1.up.index.voffset, ==, -1));
  test_fail_if(!test_compare_ii(rp1.up.index.nbpoints, ==, 5));
  test_fail_if(!test_compare_ii(rp1.up.index.nfpoints, ==, 6));
  test_fail_if(!test_compare_eps(rect3d2p_getcR(&rp1), 0.52, 1e-6));
  test_fail_if(!test_compare_eps(rect3d2p_getcU(&rp1), 0.35, 1e-6));
  test_fail_if(!test_compare_dd(rect3d2p_getv(&rp1), ==, 272.125));
  test_fail_if(!test_compare_dd(val[(4 * 13 + 2) * 12 + 6], ==, 272.125));

  // ...

  rp11 = rect3d2p_to1R(&rp1);
  test_fail_if(!test_compare_poff(rp11.values, val, (4 * 13 + 2) * 12 + 6));
  test_fail_if(!test_compare_ii(rp11.axis.coffset, ==, 1));
  test_fail_if(!test_compare_poff(rp11.axis.coords, cdo.zc, 4));
  test_fail_if(!test_compare_ii(rp11.axis.index.voffset, ==, 12 * 13));
  test_fail_if(!test_compare_ii(rp11.axis.index.nbpoints, ==, 4));
  test_fail_if(!test_compare_ii(rp11.axis.index.nfpoints, ==, 9));
  test_fail_if(!test_compare_eps(rect3d1p_getc(&rp11), 0.52, 1e-6));
  test_fail_if(!test_compare_dd(rect3d1p_getv(&rp11), ==, 272.125));
  test_fail_if(!test_compare_dd(val[(4 * 13 + 2) * 12 + 6], ==, 272.125));

  rp11 = rect3d2p_to1U(&rp1);
  test_fail_if(!test_compare_poff(rp11.values, val, (4 * 13 + 2) * 12 + 6));
  test_fail_if(!test_compare_ii(rp11.axis.coffset, ==, -1));
  test_fail_if(!test_compare_poff(rp11.axis.coords, cdo.xv, 6));
  test_fail_if(!test_compare_ii(rp11.axis.index.voffset, ==, -1));
  test_fail_if(!test_compare_ii(rp11.axis.index.nbpoints, ==, 5));
  test_fail_if(!test_compare_ii(rp11.axis.index.nfpoints, ==, 6));
  test_fail_if(!test_compare_eps(rect3d1p_getc(&rp11), 0.35, 1e-6));
  test_fail_if(!test_compare_dd(rect3d1p_getv(&rp11), ==, 272.125));
  test_fail_if(!test_compare_dd(val[(4 * 13 + 2) * 12 + 6], ==, 272.125));

  rp1 = rect3d2p_YZvcm(val, 8, 2, 3, &cdo, 1, -1);
  test_fail_if(!test_compare_eps(rect3d2p_getcR(&rp1), 2.0, 1e-6));
  test_fail_if(!test_compare_eps(rect3d2p_getcU(&rp1), 0.505, 1e-6));
  test_fail_if(!test_compare_dd(rect3d2p_getv(&rp1), ==, 54.5));
  test_fail_if(!test_compare_dd(val[(3 * 13 + 2) * 12 + 8], ==, 54.5));

  rp2 = rp1;
  rect3d2p_absmove(&rp2, 0, 1);
  test_fail_if(!test_compare_eps(rect3d2p_getcR(&rp2), 2.0, 1e-6));
  test_fail_if(!test_compare_eps(rect3d2p_getcU(&rp2), 0.52, 1e-6));
  test_fail_if(!test_compare_dd(rect3d2p_getv(&rp2), ==, 186.75));
  test_fail_if(!test_compare_dd(val[(4 * 13 + 2) * 12 + 8], ==, 186.75));

  rp2 = rp1;
  rect3d2p_absmove(&rp2, 1, 0);
  test_fail_if(!test_compare_eps(rect3d2p_getcR(&rp2), 3.0, 1e-6));
  test_fail_if(!test_compare_eps(rect3d2p_getcU(&rp2), 0.505, 1e-6));
  test_fail_if(!test_compare_dd(rect3d2p_getv(&rp2), ==, 228.875));
  test_fail_if(!test_compare_dd(val[(3 * 13 + 3) * 12 + 8], ==, 228.875));

  rp2 = rp1;
  rect3d2p_absmove(&rp2, 1, 1);
  test_fail_if(!test_compare_eps(rect3d2p_getcR(&rp2), 3.0, 1e-6));
  test_fail_if(!test_compare_eps(rect3d2p_getcU(&rp2), 0.52, 1e-6));
  test_fail_if(!test_compare_dd(rect3d2p_getv(&rp2), ==, 27.875));
  test_fail_if(!test_compare_dd(val[(4 * 13 + 3) * 12 + 8], ==, 27.875));

  test_fail_if(
    !test_compare_eps(rect3d2_interp_linear(&rp1, 2.25, 0.5175, NULL), 138.875,
                      1.0e-6));
  test_fail_if(
    !test_compare_eps(rect3d2_interp_linear(&rp1, 2.75, 0.5125, NULL), 126.4375,
                      1.0e-6));
  test_fail_if(
    !test_compare_eps(rect3d2_interp_linear(&rp1, 2.25, 0.5225, NULL), 155.1875,
                      1.0e-6));

  rp2 = rp1;
  rect3d2p_absmove(&rp2, 0, -1);
  test_fail_if(!test_compare_eps(rect3d2p_getcR(&rp2), 2.0, 1e-6));
  test_fail_if(!test_compare_eps(rect3d2p_getcU(&rp2), 0.495, 1e-6));
  test_fail_if(!test_compare_dd(rect3d2p_getv(&rp2), ==, 11.75));
  test_fail_if(!test_compare_dd(val[(2 * 13 + 2) * 12 + 8], ==, 11.75));

  rp2 = rp1;
  rect3d2p_absmove(&rp2, 1, -1);
  test_fail_if(!test_compare_eps(rect3d2p_getcR(&rp2), 3.0, 1e-6));
  test_fail_if(!test_compare_eps(rect3d2p_getcU(&rp2), 0.495, 1e-6));
  test_fail_if(!test_compare_dd(rect3d2p_getv(&rp2), ==, 143.0));
  test_fail_if(!test_compare_dd(val[(2 * 13 + 3) * 12 + 8], ==, 143.0));

  rp2 = rp1;
  test_fail_if(!test_compare_eps(rect3d2p_getcR(&rp2), 2.0, 1e-6));
  test_fail_if(!test_compare_eps(rect3d2p_getcU(&rp2), 0.505, 1e-6));
  test_fail_if(!test_compare_dd(rect3d2p_getv(&rp2), ==, 54.5));
  test_fail_if(!test_compare_dd(val[(3 * 13 + 2) * 12 + 8], ==, 54.5));

  rp2 = rp1;
  rect3d2p_absmove(&rp2, 1, 0);
  test_fail_if(!test_compare_eps(rect3d2p_getcR(&rp2), 3.0, 1e-6));
  test_fail_if(!test_compare_eps(rect3d2p_getcU(&rp2), 0.505, 1e-6));
  test_fail_if(!test_compare_dd(rect3d2p_getv(&rp2), ==, 228.875));
  test_fail_if(!test_compare_dd(val[(3 * 13 + 3) * 12 + 8], ==, 228.875));

  test_fail_if(!test_compare_eps(rect3d2_interp_linear(&rp1, 2.25, 0.5, NULL),
                                 71.328125, 1.0e-6));
  test_fail_if(!test_compare_eps(rect3d2_interp_linear(&rp1, 2.75, 0.475, NULL),
                                 -40.0, 1.0e-6));

  rp2 = rp1;
  rect3d2p_absmove(&rp2, -1, 0);
  test_fail_if(!test_compare_eps(rect3d2p_getcR(&rp2), 1.0, 1e-6));
  test_fail_if(!test_compare_eps(rect3d2p_getcU(&rp2), 0.505, 1e-6));
  test_fail_if(!test_compare_dd(rect3d2p_getv(&rp2), ==, 55.875));
  test_fail_if(!test_compare_dd(val[(3 * 13 + 1) * 12 + 8], ==, 55.875));

  rp2 = rp1;
  rect3d2p_absmove(&rp2, -1, 1);
  test_fail_if(!test_compare_eps(rect3d2p_getcR(&rp2), 1.0, 1e-6));
  test_fail_if(!test_compare_eps(rect3d2p_getcU(&rp2), 0.52, 1e-6));
  test_fail_if(!test_compare_dd(rect3d2p_getv(&rp2), ==, 32.75));
  test_fail_if(!test_compare_dd(val[(4 * 13 + 1) * 12 + 8], ==, 32.75));

  rp2 = rp1;
  rect3d2p_absmove(&rp2, 0, 0);
  test_fail_if(!test_compare_eps(rect3d2p_getcR(&rp2), 2.0, 1e-6));
  test_fail_if(!test_compare_eps(rect3d2p_getcU(&rp2), 0.505, 1e-6));
  test_fail_if(!test_compare_dd(rect3d2p_getv(&rp2), ==, 54.5));
  test_fail_if(!test_compare_dd(val[(3 * 13 + 2) * 12 + 8], ==, 54.5));

  rp2 = rp1;
  rect3d2p_absmove(&rp2, 0, 1);
  test_fail_if(!test_compare_eps(rect3d2p_getcR(&rp2), 2.0, 1e-6));
  test_fail_if(!test_compare_eps(rect3d2p_getcU(&rp2), 0.52, 1e-6));
  test_fail_if(!test_compare_dd(rect3d2p_getv(&rp2), ==, 186.75));
  test_fail_if(!test_compare_dd(val[(4 * 13 + 2) * 12 + 8], ==, 186.75));

  test_fail_if(!test_compare_eps(rect3d2_interp_linear(&rp1, 1.5, 0.5075, NULL),
                                 64.28125, 1.0e-6));
  test_fail_if(!test_compare_eps(rect3d2_interp_linear(&rp1, 0.0, 0.5125, NULL),
                                 -32.0, 1.0e-6));

  return r;
}
