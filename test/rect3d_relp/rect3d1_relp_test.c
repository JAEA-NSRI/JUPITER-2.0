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

int rect3d1_relp_test(void)
{
  domain cdo;
  int r = 0;
  struct rect3d1p rp1, rp2, rp3;
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

  rp1 = rect3d1p_Xc(val, cdo.stm, cdo.stm, cdo.stm, &cdo, 1, 0, 0);
  test_fail_if(!test_compare_poff(rp1.values, val, (3 * 13 + 3) * 12 + 3));
  /* calc_address(cdo.stm, cdo.stm, cdo.stm, 12, 13, 14) */
  test_fail_if(!test_compare_ii(rp1.axis.coffset, ==, 1));
  test_fail_if(!test_compare_poff(rp1.axis.coords, cdo.xc, 3));
  test_fail_if(!test_compare_ii(rp1.axis.index.voffset, ==, 1));
  test_fail_if(!test_compare_ii(rp1.axis.index.nbpoints, ==, 0));
  test_fail_if(!test_compare_ii(rp1.axis.index.nfpoints, ==, 4));
  test_fail_if(!test_compare_eps(rect3d1p_getc(&rp1), 0.05, 1e-6));
  test_fail_if(!test_compare_dd(rect3d1p_getv(&rp1), ==, 114.125));
  test_fail_if(!test_compare_dd(val[(3 * 13 + 3) * 12 + 3], ==, 114.125));

  test_fail_if(!test_compare_ii(rect3d1p_move(&rp1, 1), ==, 1));
  test_fail_if(!test_compare_poff(rp1.values, val, (3 * 13 + 3) * 12 + 4));
  /* calc_address(cdo.stm + 1, cdo.stm, cdo.stm, 12, 13, 14) */
  test_fail_if(!test_compare_ii(rp1.axis.coffset, ==, 1));
  test_fail_if(!test_compare_poff(rp1.axis.coords, cdo.xc, 4));
  test_fail_if(!test_compare_ii(rp1.axis.index.voffset, ==, 1));
  test_fail_if(!test_compare_ii(rp1.axis.index.nbpoints, ==, 1));
  test_fail_if(!test_compare_ii(rp1.axis.index.nfpoints, ==, 3));
  test_fail_if(!test_compare_eps(rect3d1p_getc(&rp1), 0.2, 1e-6));
  test_fail_if(!test_compare_dd(rect3d1p_getv(&rp1), ==, 121.125));
  test_fail_if(!test_compare_dd(val[(3 * 13 + 3) * 12 + 4], ==, 121.125));

  rect3d1p_move(&rp1, 3);
  test_fail_if(!test_compare_poff(rp1.values, val, (3 * 13 + 3) * 12 + 7));
  /* calc_address(cdo.stm + 4, cdo.stm, cdo.stm, 12, 13, 14) */
  test_fail_if(!test_compare_ii(rp1.axis.coffset, ==, 1));
  test_fail_if(!test_compare_poff(rp1.axis.coords, cdo.xc, 7));
  test_fail_if(!test_compare_ii(rp1.axis.index.voffset, ==, 1));
  test_fail_if(!test_compare_ii(rp1.axis.index.nbpoints, ==, 4));
  test_fail_if(!test_compare_ii(rp1.axis.index.nfpoints, ==, 0));
  test_fail_if(!test_compare_eps(rect3d1p_getc(&rp1), 0.425, 1e-6));
  test_fail_if(!test_compare_dd(rect3d1p_getv(&rp1), ==, 98.625));
  test_fail_if(!test_compare_dd(val[(3 * 13 + 3) * 12 + 7], ==, 98.625));
  test_fail_if(!test_compare_dd(rect3d1p_getnv(&rp1, -3), ==, 121.125));

  rp1 = rect3d1p_Xv(val, cdo.stm, cdo.stm, cdo.stm, &cdo, 1, 0, 0);
  test_fail_if(!test_compare_poff(rp1.values, val, (3 * 13 + 3) * 12 + 3));
  /* calc_address(cdo.stm, cdo.stm, cdo.stm, 12, 13, 14) */
  test_fail_if(!test_compare_ii(rp1.axis.coffset, ==, 1));
  test_fail_if(!test_compare_poff(rp1.axis.coords, cdo.xv, 3));
  test_fail_if(!test_compare_ii(rp1.axis.index.voffset, ==, 1));
  test_fail_if(!test_compare_ii(rp1.axis.index.nbpoints, ==, 0));
  test_fail_if(!test_compare_ii(rp1.axis.index.nfpoints, ==, 5));
  test_fail_if(!test_compare_eps(rect3d1p_getc(&rp1), 0.0, 1e-6));
  test_fail_if(!test_compare_dd(rect3d1p_getv(&rp1), ==, 114.125));
  test_fail_if(!test_compare_dd(val[(3 * 13 + 3) * 12 + 3], ==, 114.125));

  rp1 = rect3d1p_Xcm(val, 3, 2, 5, &cdo, 1);
  test_fail_if(!test_compare_poff(rp1.values, val, (5 * 13 + 2) * 12 + 3));
  /* calc_address(5, 2, 3, 12, 13, 14) */
  test_fail_if(!test_compare_ii(rp1.axis.coffset, ==, 1));
  test_fail_if(!test_compare_poff(rp1.axis.coords, cdo.xc, 3));
  test_fail_if(!test_compare_ii(rp1.axis.index.voffset, ==, 1));
  test_fail_if(!test_compare_ii(rp1.axis.index.nbpoints, ==, 3));
  test_fail_if(!test_compare_ii(rp1.axis.index.nfpoints, ==, 8));
  test_fail_if(!test_compare_eps(rect3d1p_getc(&rp1), 0.05, 1e-6));
  test_fail_if(!test_compare_dd(rect3d1p_getv(&rp1), ==, 128.25));
  test_fail_if(!test_compare_dd(val[(5 * 13 + 2) * 12 + 3], ==, 128.25));

  rp1 = rect3d1p_Xvm(val, 5, 1, 4, &cdo, -1);
  test_fail_if(!test_compare_poff(rp1.values, val, (4 * 13 + 1) * 12 + 5));
  /* calc_address(5, 1, 4, 12, 13, 14) */
  test_fail_if(!test_compare_ii(rp1.axis.coffset, ==, -1));
  test_fail_if(!test_compare_poff(rp1.axis.coords, cdo.xv, 5));
  test_fail_if(!test_compare_ii(rp1.axis.index.voffset, ==, -1));
  test_fail_if(!test_compare_ii(rp1.axis.index.nbpoints, ==, 6));
  test_fail_if(!test_compare_ii(rp1.axis.index.nfpoints, ==, 5));
  test_fail_if(!test_compare_eps(rect3d1p_getc(&rp1), 0.3, 1e-6));
  test_fail_if(!test_compare_dd(rect3d1p_getv(&rp1), ==, 173.5));
  test_fail_if(!test_compare_dd(val[(4 * 13 + 1) * 12 + 5], ==, 173.5));

  rp2 = rect3d1p_Xvm(val, 1, 1, 4, &cdo, 1);
  test_fail_if(!test_compare_ii(rect3d1p_in_same_axis(&rp2, &rp1), ==, 1));

  rp2 = rect3d1p_Xvm(val, 7, 1, 4, &cdo, 1);
  test_fail_if(!test_compare_ii(rect3d1p_in_same_axis(&rp2, &rp1), ==, -1));

  rp2 = rect3d1p_Xvm(val, 1, 1, 4, &cdo, -1);
  test_fail_if(!test_compare_ii(rect3d1p_in_same_axis(&rp2, &rp1), ==, 1));

  rp2 = rect3d1p_Xvm(val, 7, 1, 4, &cdo, -1);
  test_fail_if(!test_compare_ii(rect3d1p_in_same_axis(&rp2, &rp1), ==, -1));

  rp2 = rect3d1p_Xvm(val, 7, 2, 4, &cdo, -1);
  test_fail_if(!test_compare_ii(rect3d1p_in_same_axis(&rp2, &rp1), ==, 0));

  rp2 = rect3d1p_Xcm(val, 5, 1, 4, &cdo, -1);
  test_fail_if(!test_compare_ii(rect3d1p_in_same_axis(&rp2, &rp1), ==, 0));

  rp1 = rect3d1p_Xcm(val, 8, 1, 4, &cdo, -1);
  test_fail_if(!test_compare_eps(rect3d1p_getc(&rp1), 0.475, 1e-6));
  test_fail_if(!test_compare_dd(rect3d1p_getv(&rp1), ==, 32.75));
  test_fail_if(!test_compare_dd(val[(4 * 13 + 1) * 12 + 8], ==, 32.75));

  rp2 = rp1;
  rect3d1p_move(&rp2, 1);
  test_fail_if(!test_compare_eps(rect3d1p_getc(&rp2), 0.425, 1e-6));
  test_fail_if(!test_compare_dd(rect3d1p_getv(&rp2), ==, 195.25));
  test_fail_if(!test_compare_dd(val[(4 * 13 + 1) * 12 + 7], ==, 195.25));

  test_fail_if(
    !test_compare_eps(rect3d1_interp_linear(&rp2, 0.45, NULL), 114.0, 1.0e-6));
  test_fail_if(!test_compare_eps(rect3d1_interp_linear(&rp2, 0.4375, NULL),
                                 154.625, 1.0e-6));
  test_fail_if(!test_compare_eps(rect3d1_interp_linear(&rp2, 0.4625, NULL),
                                 73.375, 1.0e-6));
  test_fail_if(!test_compare_eps(rect3d1_interp_linear(&rp2, 0.425, NULL),
                                 195.25, 1.0e-6));
  test_fail_if(
    !test_compare_eps(rect3d1_interp_linear(&rp2, 0.475, NULL), 32.75, 1.0e-6));
  test_fail_if(
    !test_compare_eps(rect3d1_interp_linear(&rp2, 0.5, NULL), -48.5, 1.0e-6));

  rp1 = rp2;
  rect3d1p_move(&rp1, 1);
  test_fail_if(!test_compare_eps(rect3d1p_getc(&rp1), 0.375, 1e-6));
  test_fail_if(!test_compare_dd(rect3d1p_getv(&rp1), ==, 232.0));
  test_fail_if(!test_compare_dd(val[(4 * 13 + 1) * 12 + 6], ==, 232.0));

  test_fail_if(
    !test_compare_eps(rect3d1_interp_linear(&rp2, 0.4, NULL), 213.625, 1.0e-6));

  rect3d1p_move(&rp2, rect3d_relpointer_axis_nfwdpts(&rp2.axis));
  test_fail_if(!test_compare_eps(rect3d1p_getc(&rp2), -0.25, 1e-6));
  test_fail_if(!test_compare_dd(rect3d1p_getv(&rp2), ==, 228.5));
  test_fail_if(!test_compare_dd(val[(4 * 13 + 1) * 12 + 0], ==, 228.5));

  rp1 = rp2;
  rect3d1p_move(&rp1, -1);
  test_fail_if(!test_compare_eps(rect3d1p_getc(&rp1), -0.15, 1e-6));
  test_fail_if(!test_compare_dd(rect3d1p_getv(&rp1), ==, 1.0));
  test_fail_if(!test_compare_dd(val[(4 * 13 + 1) * 12 + 1], ==, 1.0));

  test_fail_if(
    !test_compare_eps(rect3d1_interp_linear(&rp2, -0.3, NULL), 342.25, 1.0e-6));

  rect3d1p_move(&rp2, -rect3d_relpointer_axis_nbkwdpts(&rp2.axis));
  test_fail_if(!test_compare_eps(rect3d1p_getc(&rp2), 0.625, 1e-6));
  test_fail_if(!test_compare_dd(rect3d1p_getv(&rp2), ==, 9.625));
  test_fail_if(!test_compare_dd(val[(4 * 13 + 1) * 12 + 11], ==, 9.625));

  rp1 = rp2;
  rect3d1p_move(&rp1, 1);
  test_fail_if(!test_compare_eps(rect3d1p_getc(&rp1), 0.575, 1e-6));
  test_fail_if(!test_compare_dd(rect3d1p_getv(&rp1), ==, 166.375));
  test_fail_if(!test_compare_dd(val[(4 * 13 + 1) * 12 + 10], ==, 166.375));

  test_fail_if(
    !test_compare_eps(rect3d1_interp_linear(&rp2, 0.8, NULL), -539.0, 1.0e-6));
  test_fail_if(
    !test_compare_eps(rect3d1_interp_linear(&rp2, 0.6, NULL), 88.0, 1.0e-6));

  return r;
}
