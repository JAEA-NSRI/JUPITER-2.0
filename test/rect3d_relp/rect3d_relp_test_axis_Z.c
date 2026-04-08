
#include <jupiter/func.h>
#include <jupiter/rect3d_relp.h>
#include <jupiter/struct.h>

#include "rect3d_relp_test.h"
#include "test-util.h"

#define test_fail_if(expr) \
  do {                     \
    if (expr)              \
      r = 1;               \
  } while (0)

int rect3d_axis_Z_test(void)
{
  int r;
  domain cdo;
  mpi_param mpi;
  struct rect3d_relpointer_axis axis;

  set_self_comm_mpi(&mpi);

  r = 0;

  if (!make_cdo_c(&mpi, &cdo, 12, 13, 14, 3, 4, 0.5, 6.0, 70.0))
    return 1;

  axis = rect3d_relpointer_axis_Zcm(&cdo, 1, 0);
  test_fail_if(!test_compare_ii(axis.coffset, ==, 1));
  test_fail_if(!test_compare_pp(axis.coords, ==, cdo.zc));
  test_fail_if(!test_compare_ii(axis.index.voffset, ==, cdo.mxy));
  test_fail_if(!test_compare_ii(axis.index.nbpoints, ==, 0));
  test_fail_if(!test_compare_ii(axis.index.nfpoints, ==, 13));

  axis = rect3d_relpointer_axis_Zcm(&cdo, -1, 0);
  test_fail_if(!test_compare_ii(axis.coffset, ==, -1));
  test_fail_if(!test_compare_pp(axis.coords, ==, cdo.zc));
  test_fail_if(!test_compare_ii(axis.index.voffset, ==, -cdo.mxy));
  test_fail_if(!test_compare_ii(axis.index.nbpoints, ==, 13));
  test_fail_if(!test_compare_ii(axis.index.nfpoints, ==, 0));

  axis = rect3d_relpointer_axis_Zvm(&cdo, 1, 0);
  test_fail_if(!test_compare_ii(axis.coffset, ==, 1));
  test_fail_if(!test_compare_pp(axis.coords, ==, cdo.zv));
  test_fail_if(!test_compare_ii(axis.index.voffset, ==, cdo.mxy));
  test_fail_if(!test_compare_ii(axis.index.nbpoints, ==, 0));
  test_fail_if(!test_compare_ii(axis.index.nfpoints, ==, 13));

  axis = rect3d_relpointer_axis_Zc(&cdo, 1, cdo.stm, 0, 0);
  test_fail_if(!test_compare_ii(axis.coffset, ==, 1));
  test_fail_if(!test_compare_poff(axis.coords, cdo.zc, cdo.stm));
  test_fail_if(!test_compare_ii(axis.index.voffset, ==, cdo.mxy));
  test_fail_if(!test_compare_ii(axis.index.nbpoints, ==, 0));
  test_fail_if(!test_compare_ii(axis.index.nfpoints, ==, 6));

  axis = rect3d_relpointer_axis_Zv(&cdo, 1, cdo.stm, 0, 0);
  test_fail_if(!test_compare_ii(axis.coffset, ==, 1));
  test_fail_if(!test_compare_poff(axis.coords, cdo.zv, cdo.stm));
  test_fail_if(!test_compare_ii(axis.index.voffset, ==, cdo.mxy));
  test_fail_if(!test_compare_ii(axis.index.nbpoints, ==, 0));
  test_fail_if(!test_compare_ii(axis.index.nfpoints, ==, 7));

  test_fail_if(
    !test_compare_ii(rect3d_relpointer_axis_movable(&axis, -1), ==, 0));
  test_fail_if(
    !test_compare_ii(rect3d_relpointer_axis_movable(&axis, 1), ==, 1));

  test_fail_if(
    !test_compare_ii(rect3d_relpointer_axis_absmovable(&axis, -1), ==, 0));
  test_fail_if(
    !test_compare_ii(rect3d_relpointer_axis_absmovable(&axis, 1), ==, 1));

  test_fail_if(!test_compare_ii(rect3d_relpointer_axis_coffset(&axis), ==, 1));
  test_fail_if(
    !test_compare_ii(rect3d_relpointer_axis_voffset(&axis), ==, cdo.mxy));
  test_fail_if(
    !test_compare_ii(rect3d_relpointer_axis_abscoffset(&axis), ==, 1));
  test_fail_if(
    !test_compare_ii(rect3d_relpointer_axis_absvoffset(&axis), ==, cdo.mxy));

  test_fail_if(!test_compare_ii(rect3d_relpointer_axis_nfwdpts(&axis), ==, 7));
  test_fail_if(!test_compare_ii(rect3d_relpointer_axis_nbkwdpts(&axis), ==, 0));
  test_fail_if(!test_compare_ii(rect3d_relpointer_axis_npospts(&axis), ==, 7));
  test_fail_if(!test_compare_ii(rect3d_relpointer_axis_nnegpts(&axis), ==, 0));

  test_fail_if(
    !test_compare_eps(rect3d_relpointer_axis_coords(&axis), 0.0, 1.0e-6));

  test_fail_if(
    !test_compare_ii(rect3d_relpointer_axis_move(&axis, 1), ==, cdo.mxy));
  test_fail_if(!test_compare_ii(axis.coffset, ==, 1));
  test_fail_if(!test_compare_poff(axis.coords, cdo.zv, cdo.stm + 1));
  test_fail_if(!test_compare_ii(axis.index.voffset, ==, cdo.mxy));
  test_fail_if(!test_compare_ii(axis.index.nbpoints, ==, 1));
  test_fail_if(!test_compare_ii(axis.index.nfpoints, ==, 6));
  test_fail_if(
    !test_compare_eps(rect3d_relpointer_axis_coords(&axis), 10.0, 1.0e-6));

  test_fail_if(
    !test_compare_ii(rect3d_relpointer_axis_move(&axis, 3), ==, 3 * cdo.mxy));
  test_fail_if(!test_compare_ii(axis.coffset, ==, 1));
  test_fail_if(!test_compare_poff(axis.coords, cdo.zv, cdo.stm + 4));
  test_fail_if(!test_compare_ii(axis.index.voffset, ==, cdo.mxy));
  test_fail_if(!test_compare_ii(axis.index.nbpoints, ==, 4));
  test_fail_if(!test_compare_ii(axis.index.nfpoints, ==, 3));
  test_fail_if(
    !test_compare_eps(rect3d_relpointer_axis_coords(&axis), 40.0, 1.0e-6));

  test_fail_if(
    !test_compare_ii(rect3d_relpointer_axis_move(&axis, -2), ==, -2 * cdo.mxy));
  test_fail_if(!test_compare_ii(axis.coffset, ==, 1));
  test_fail_if(!test_compare_poff(axis.coords, cdo.zv, cdo.stm + 2));
  test_fail_if(!test_compare_ii(axis.index.voffset, ==, cdo.mxy));
  test_fail_if(!test_compare_ii(axis.index.nbpoints, ==, 2));
  test_fail_if(!test_compare_ii(axis.index.nfpoints, ==, 5));
  test_fail_if(
    !test_compare_eps(rect3d_relpointer_axis_coords(&axis), 20.0, 1.0e-6));

  test_fail_if(!test_compare_ii(rect3d_relpointer_axis_absmove(&axis, 3), ==,
                                3 * cdo.mxy));
  test_fail_if(!test_compare_ii(axis.coffset, ==, 1));
  test_fail_if(!test_compare_poff(axis.coords, cdo.zv, cdo.stm + 5));
  test_fail_if(!test_compare_ii(axis.index.voffset, ==, cdo.mxy));
  test_fail_if(!test_compare_ii(axis.index.nbpoints, ==, 5));
  test_fail_if(!test_compare_ii(axis.index.nfpoints, ==, 2));
  test_fail_if(
    !test_compare_eps(rect3d_relpointer_axis_coords(&axis), 50.0, 1.0e-6));

  test_fail_if(!test_compare_ii(rect3d_relpointer_axis_absmove(&axis, -2), ==,
                                -2 * cdo.mxy));
  test_fail_if(!test_compare_ii(axis.coffset, ==, 1));
  test_fail_if(!test_compare_poff(axis.coords, cdo.zv, cdo.stm + 3));
  test_fail_if(!test_compare_ii(axis.index.voffset, ==, cdo.mxy));
  test_fail_if(!test_compare_ii(axis.index.nbpoints, ==, 3));
  test_fail_if(!test_compare_ii(axis.index.nfpoints, ==, 4));
  test_fail_if(
    !test_compare_eps(rect3d_relpointer_axis_coords(&axis), 30.0, 1.0e-6));

  //--- rev
  axis = rect3d_relpointer_axis_Zv(&cdo, -1, cdo.stm + 7, 0, 0);
  test_fail_if(!test_compare_ii(axis.coffset, ==, -1));
  test_fail_if(!test_compare_poff(axis.coords, cdo.zv, cdo.stm + 7));
  test_fail_if(!test_compare_ii(axis.index.voffset, ==, -cdo.mxy));
  test_fail_if(!test_compare_ii(axis.index.nbpoints, ==, 0));
  test_fail_if(!test_compare_ii(axis.index.nfpoints, ==, 7));

  test_fail_if(
    !test_compare_ii(rect3d_relpointer_axis_movable(&axis, -1), ==, 0));
  test_fail_if(
    !test_compare_ii(rect3d_relpointer_axis_movable(&axis, 1), ==, 1));

  test_fail_if(
    !test_compare_ii(rect3d_relpointer_axis_absmovable(&axis, -1), ==, 1));
  test_fail_if(
    !test_compare_ii(rect3d_relpointer_axis_absmovable(&axis, 1), ==, 0));

  test_fail_if(!test_compare_ii(rect3d_relpointer_axis_coffset(&axis), ==, -1));
  test_fail_if(
    !test_compare_ii(rect3d_relpointer_axis_voffset(&axis), ==, -cdo.mxy));
  test_fail_if(
    !test_compare_ii(rect3d_relpointer_axis_abscoffset(&axis), ==, 1));
  test_fail_if(
    !test_compare_ii(rect3d_relpointer_axis_absvoffset(&axis), ==, cdo.mxy));

  test_fail_if(!test_compare_ii(rect3d_relpointer_axis_nfwdpts(&axis), ==, 7));
  test_fail_if(!test_compare_ii(rect3d_relpointer_axis_nbkwdpts(&axis), ==, 0));
  test_fail_if(!test_compare_ii(rect3d_relpointer_axis_npospts(&axis), ==, 0));
  test_fail_if(!test_compare_ii(rect3d_relpointer_axis_nnegpts(&axis), ==, 7));

  test_fail_if(
    !test_compare_eps(rect3d_relpointer_axis_coords(&axis), 70.0, 1.0e-6));

  test_fail_if(
    !test_compare_ii(rect3d_relpointer_axis_move(&axis, 1), ==, -1 * cdo.mxy));
  test_fail_if(!test_compare_ii(axis.coffset, ==, -1));
  test_fail_if(!test_compare_poff(axis.coords, cdo.zv, cdo.stm + 6));
  test_fail_if(!test_compare_ii(axis.index.voffset, ==, -cdo.mxy));
  test_fail_if(!test_compare_ii(axis.index.nbpoints, ==, 1));
  test_fail_if(!test_compare_ii(axis.index.nfpoints, ==, 6));
  test_fail_if(
    !test_compare_eps(rect3d_relpointer_axis_coords(&axis), 60.0, 1.0e-6));

  test_fail_if(
    !test_compare_ii(rect3d_relpointer_axis_move(&axis, 3), ==, -3 * cdo.mxy));
  test_fail_if(!test_compare_ii(axis.coffset, ==, -1));
  test_fail_if(!test_compare_poff(axis.coords, cdo.zv, cdo.stm + 3));
  test_fail_if(!test_compare_ii(axis.index.voffset, ==, -cdo.mxy));
  test_fail_if(!test_compare_ii(axis.index.nbpoints, ==, 4));
  test_fail_if(!test_compare_ii(axis.index.nfpoints, ==, 3));
  test_fail_if(
    !test_compare_eps(rect3d_relpointer_axis_coords(&axis), 30.0, 1.0e-6));

  test_fail_if(
    !test_compare_ii(rect3d_relpointer_axis_move(&axis, -2), ==, 2 * cdo.mxy));
  test_fail_if(!test_compare_ii(axis.coffset, ==, -1));
  test_fail_if(!test_compare_poff(axis.coords, cdo.zv, cdo.stm + 5));
  test_fail_if(!test_compare_ii(axis.index.voffset, ==, -cdo.mxy));
  test_fail_if(!test_compare_ii(axis.index.nbpoints, ==, 2));
  test_fail_if(!test_compare_ii(axis.index.nfpoints, ==, 5));
  test_fail_if(
    !test_compare_eps(rect3d_relpointer_axis_coords(&axis), 50.0, 1.0e-6));

  test_fail_if(!test_compare_ii(rect3d_relpointer_axis_absmove(&axis, -3), ==,
                                -3 * cdo.mxy));
  test_fail_if(!test_compare_ii(axis.coffset, ==, -1));
  test_fail_if(!test_compare_poff(axis.coords, cdo.zv, cdo.stm + 2));
  test_fail_if(!test_compare_ii(axis.index.voffset, ==, -cdo.mxy));
  test_fail_if(!test_compare_ii(axis.index.nbpoints, ==, 5));
  test_fail_if(!test_compare_ii(axis.index.nfpoints, ==, 2));
  test_fail_if(
    !test_compare_eps(rect3d_relpointer_axis_coords(&axis), 20.0, 1.0e-6));

  test_fail_if(!test_compare_ii(rect3d_relpointer_axis_absmove(&axis, 2), ==,
                                2 * cdo.mxy));
  test_fail_if(!test_compare_ii(axis.coffset, ==, -1));
  test_fail_if(!test_compare_poff(axis.coords, cdo.zv, cdo.stm + 4));
  test_fail_if(!test_compare_ii(axis.index.voffset, ==, -cdo.mxy));
  test_fail_if(!test_compare_ii(axis.index.nbpoints, ==, 3));
  test_fail_if(!test_compare_ii(axis.index.nfpoints, ==, 4));
  test_fail_if(
    !test_compare_eps(rect3d_relpointer_axis_coords(&axis), 40.0, 1.0e-6));

  test_fail_if(
    !test_compare_ii(rect3d_relpointer_axis_move_nearest(&axis, 50.0), ==,
                     1 * cdo.mxy));
  test_fail_if(
    !test_compare_eps(rect3d_relpointer_axis_coords(&axis), 50.0, 1.0e-6));
  test_fail_if(!test_compare_ii(axis.coffset, ==, -1));
  test_fail_if(!test_compare_poff(axis.coords, cdo.zv, cdo.stm + 5));
  test_fail_if(!test_compare_ii(axis.index.voffset, ==, -cdo.mxy));
  test_fail_if(!test_compare_ii(axis.index.nbpoints, ==, 2));
  test_fail_if(!test_compare_ii(axis.index.nfpoints, ==, 5));

  test_fail_if(
    !test_compare_ii(rect3d_relpointer_axis_move_nearest(&axis, 42.0), ==,
                     -1 * cdo.mxy));
  test_fail_if(
    !test_compare_eps(rect3d_relpointer_axis_coords(&axis), 40.0, 1.0e-6));
  test_fail_if(!test_compare_ii(axis.coffset, ==, -1));
  test_fail_if(!test_compare_poff(axis.coords, cdo.zv, cdo.stm + 4));
  test_fail_if(!test_compare_ii(axis.index.voffset, ==, -cdo.mxy));
  test_fail_if(!test_compare_ii(axis.index.nbpoints, ==, 3));
  test_fail_if(!test_compare_ii(axis.index.nfpoints, ==, 4));

  test_fail_if(
    !test_compare_ii(rect3d_relpointer_axis_move_nearest(&axis, 57.0), ==,
                     2 * cdo.mxy));
  test_fail_if(
    !test_compare_eps(rect3d_relpointer_axis_coords(&axis), 60.0, 1.0e-6));
  test_fail_if(!test_compare_ii(axis.coffset, ==, -1));
  test_fail_if(!test_compare_poff(axis.coords, cdo.zv, cdo.stm + 6));
  test_fail_if(!test_compare_ii(axis.index.voffset, ==, -cdo.mxy));
  test_fail_if(!test_compare_ii(axis.index.nbpoints, ==, 1));
  test_fail_if(!test_compare_ii(axis.index.nfpoints, ==, 6));

  test_fail_if(!test_compare_ii(rect3d_relpointer_axis_move_nearest(&axis, 8.0),
                                ==, -5 * cdo.mxy));
  test_fail_if(
    !test_compare_eps(rect3d_relpointer_axis_coords(&axis), 10.0, 1.0e-6));
  test_fail_if(!test_compare_ii(axis.coffset, ==, -1));
  test_fail_if(!test_compare_poff(axis.coords, cdo.zv, cdo.stm + 1));
  test_fail_if(!test_compare_ii(axis.index.voffset, ==, -cdo.mxy));
  test_fail_if(!test_compare_ii(axis.index.nbpoints, ==, 6));
  test_fail_if(!test_compare_ii(axis.index.nfpoints, ==, 1));

  test_fail_if(
    !test_compare_ii(rect3d_relpointer_axis_move_nearest(&axis, 32.0), ==,
                     2 * cdo.mxy));
  test_fail_if(
    !test_compare_eps(rect3d_relpointer_axis_coords(&axis), 30.0, 1.0e-6));
  test_fail_if(!test_compare_ii(axis.coffset, ==, -1));
  test_fail_if(!test_compare_poff(axis.coords, cdo.zv, cdo.stm + 3));
  test_fail_if(!test_compare_ii(axis.index.voffset, ==, -cdo.mxy));
  test_fail_if(!test_compare_ii(axis.index.nbpoints, ==, 4));
  test_fail_if(!test_compare_ii(axis.index.nfpoints, ==, 3));

  test_fail_if(
    !test_compare_ii(rect3d_relpointer_axis_move_nearest(&axis, cdo.zv[4]), ==,
                     -2 * cdo.mxy)); /* 10.0 */
  test_fail_if(
    !test_compare_dd(rect3d_relpointer_axis_coords(&axis), ==, cdo.zv[4]));

  test_fail_if(
    !test_compare_ii(rect3d_relpointer_axis_move_nearest(&axis, cdo.zv[6]), ==,
                     2 * cdo.mxy)); /* 3.0 */
  test_fail_if(
    !test_compare_dd(rect3d_relpointer_axis_coords(&axis), ==, cdo.zv[6]));

  test_fail_if(
    !test_compare_ii(rect3d_relpointer_axis_move_nearest(&axis, cdo.zv[6]), ==,
                     0));
  test_fail_if(
    !test_compare_dd(rect3d_relpointer_axis_coords(&axis), ==, cdo.zv[6]));

  test_fail_if(
    !test_compare_ii(rect3d_relpointer_axis_move_nearest(&axis, 1000.0), >=,
                     0));
  test_fail_if(!test_compare_ii(rect3d_relpointer_axis_npospts(&axis), ==, 0));

  test_fail_if(
    !test_compare_ii(rect3d_relpointer_axis_move_nearest(&axis, -1000.0), <=,
                     0));
  test_fail_if(!test_compare_ii(rect3d_relpointer_axis_nnegpts(&axis), ==, 0));

  axis = rect3d_relpointer_axis_Zvm(&cdo, 2, 4);
  test_fail_if(!test_compare_ii(axis.coffset, ==, 2));
  test_fail_if(!test_compare_poff(axis.coords, cdo.zv, 4));
  test_fail_if(!test_compare_ii(axis.index.voffset, ==, 2 * cdo.mxy));
  test_fail_if(!test_compare_ii(axis.index.nfpoints, ==, 4));
  test_fail_if(!test_compare_ii(axis.index.nbpoints, ==, 2));

  test_fail_if(
    !test_compare_ii(rect3d_relpointer_axis_move(&axis, 1), ==, 2 * cdo.mxy));
  test_fail_if(!test_compare_ii(axis.coffset, ==, 2));
  test_fail_if(!test_compare_poff(axis.coords, cdo.zv, 6));
  test_fail_if(!test_compare_ii(axis.index.voffset, ==, 2 * cdo.mxy));
  test_fail_if(!test_compare_ii(axis.index.nfpoints, ==, 3));
  test_fail_if(!test_compare_ii(axis.index.nbpoints, ==, 3));

  test_fail_if(
    !test_compare_ii(rect3d_relpointer_axis_move(&axis, -2), ==, -4 * cdo.mxy));
  test_fail_if(!test_compare_ii(axis.coffset, ==, 2));
  test_fail_if(!test_compare_poff(axis.coords, cdo.zv, 2));
  test_fail_if(!test_compare_ii(axis.index.voffset, ==, 2 * cdo.mxy));
  test_fail_if(!test_compare_ii(axis.index.nfpoints, ==, 5));
  test_fail_if(!test_compare_ii(axis.index.nbpoints, ==, 1));

  test_fail_if(
    !test_compare_eps(rect3d_relpointer_axis_coords(&axis), -10.0, 1e-6));

  test_fail_if(
    !test_compare_ii(rect3d_relpointer_axis_move_nearest(&axis, 48.0), ==,
                     6 * cdo.mxy));
  test_fail_if(
    !test_compare_eps(rect3d_relpointer_axis_coords(&axis), 50.0, 1e-6));

  test_fail_if(!test_compare_eps(cdo.zv[2], -10.0, 1.0e-6));
  test_fail_if(!test_compare_eps(cdo.zv[4], 10.0, 1.0e-6));
  if (cdo.zv[2] + cdo.zv[4] < 0.0) { /* near to 10.0 */
    test_fail_if(
      !test_compare_ii(rect3d_relpointer_axis_move_nearest(&axis, 0.0), ==,
                       -4 * cdo.mxy));
    test_fail_if(
      !test_compare_eps(rect3d_relpointer_axis_coords(&axis), 10.0, 1e-6));
  } else {
    test_fail_if(
      !test_compare_ii(rect3d_relpointer_axis_move_nearest(&axis, 0.0), ==,
                       -6 * cdo.mxy));
    test_fail_if(
      !test_compare_eps(rect3d_relpointer_axis_coords(&axis), -10.0, 1e-6));
  }

  rect3d_relpointer_axis_move(&axis, -rect3d_relpointer_axis_nbkwdpts(&axis));

  if (cdo.zv[2] + cdo.zv[4] < 0.0) { /* near to 10.0 */
    test_fail_if(
      !test_compare_ii(rect3d_relpointer_axis_move_nearest(&axis, 0.0), ==,
                       4 * cdo.mxy));
    test_fail_if(
      !test_compare_eps(rect3d_relpointer_axis_coords(&axis), 10.0, 1e-6));
  } else {
    test_fail_if(
      !test_compare_ii(rect3d_relpointer_axis_move_nearest(&axis, 0.0), ==,
                       2 * cdo.mxy));
    test_fail_if(
      !test_compare_eps(rect3d_relpointer_axis_coords(&axis), -10.0, 1e-6));
  }

  {
    struct rect3d_relpointer_axis ax1;
    struct rect3d_relpointer_axis ax2;

    ax1 = rect3d_relpointer_axis_Zcm(&cdo, 1, 0);
    ax2 = rect3d_relpointer_axis_Zcm(&cdo, -1, 6);
    test_fail_if(
      !test_compare_ii(rect3d_relpointer_axis_in_same_axis(&ax1, &ax2, NULL,
                                                           NULL, 1),
                       ==, 1));

    ax1 = rect3d_relpointer_axis_Zcm(&cdo, 1, 6);
    ax2 = rect3d_relpointer_axis_Zcm(&cdo, -1, 0);
    test_fail_if(
      !test_compare_ii(rect3d_relpointer_axis_in_same_axis(&ax1, &ax2, NULL,
                                                           NULL, 1),
                       ==, -1));

    ax1 = rect3d_relpointer_axis_Zcm(&cdo, -1, 6);
    ax2 = rect3d_relpointer_axis_Zcm(&cdo, 1, 0);
    test_fail_if(
      !test_compare_ii(rect3d_relpointer_axis_in_same_axis(&ax1, &ax2, NULL,
                                                           NULL, 1),
                       ==, -1));

    ax1 = rect3d_relpointer_axis_Zcm(&cdo, -1, 0);
    ax2 = rect3d_relpointer_axis_Zcm(&cdo, 1, 6);
    test_fail_if(
      !test_compare_ii(rect3d_relpointer_axis_in_same_axis(&ax1, &ax2, NULL,
                                                           NULL, 1),
                       ==, 1));

    ax1 = rect3d_relpointer_axis_Ycm(&cdo, 1, 0);
    ax2 = rect3d_relpointer_axis_Zcm(&cdo, 1, 0);
    test_fail_if(
      !test_compare_ii(rect3d_relpointer_axis_in_same_axis(&ax1, &ax2, NULL,
                                                           NULL, 1),
                       ==, 0));

    test_fail_if(
      !test_compare_ii(rect3d_relpointer_axis_in_same_axis(&ax1, &ax1, NULL,
                                                           NULL, 1),
                       ==, 1));
  }

  axis = rect3d_relpointer_axis_Zv(&cdo, -1, cdo.stm + 5, 0, 0);
  test_fail_if(!test_compare_ii(axis.coffset, ==, -1));
  test_fail_if(!test_compare_poff(axis.coords, cdo.zv, cdo.stm + 5));
  test_fail_if(!test_compare_ii(axis.index.voffset, ==, -12 * 13));
  test_fail_if(!test_compare_ii(axis.index.nbpoints, ==, 2));
  test_fail_if(!test_compare_ii(axis.index.nfpoints, ==, 5));

  rect3d_relpointer_axis_flip(&axis);
  test_fail_if(!test_compare_ii(axis.coffset, ==, 1));
  test_fail_if(!test_compare_poff(axis.coords, cdo.zv, cdo.stm + 5));
  test_fail_if(!test_compare_ii(axis.index.voffset, ==, 12 * 13));
  test_fail_if(!test_compare_ii(axis.index.nbpoints, ==, 5));
  test_fail_if(!test_compare_ii(axis.index.nfpoints, ==, 2));

  {
    ptrdiff_t off;

    axis = rect3d_relpointer_axis_Zvm(&cdo, 1, cdo.stm + 3);
    test_fail_if(!test_compare_ii(axis.coffset, ==, 1));
    test_fail_if(!test_compare_poff(axis.coords, cdo.zv, cdo.stm + 3));
    test_fail_if(!test_compare_ii(axis.index.voffset, ==, 12 * 13));
    test_fail_if(!test_compare_ii(axis.index.nbpoints, ==, 6));
    test_fail_if(!test_compare_ii(axis.index.nfpoints, ==, 7));

    off = 0;
    test_fail_if(
      !test_compare_ii(rect3d_relpointer_axis__ndgetnvf(
                         &off, &axis, 0, rect3d_relpointer_axis_offset),
                       ==, 1));
    test_fail_if(!test_compare_ii(off, ==, 0));
    test_fail_if(!test_compare_ii(axis.coffset, ==, 1));
    test_fail_if(!test_compare_poff(axis.coords, cdo.zv, cdo.stm + 3));
    test_fail_if(!test_compare_ii(axis.index.voffset, ==, 12 * 13));
    test_fail_if(!test_compare_ii(axis.index.nbpoints, ==, 6));
    test_fail_if(!test_compare_ii(axis.index.nfpoints, ==, 7));

    test_fail_if(
      !test_compare_ii(rect3d_relpointer_axis__ndgetnvf(
                         &off, &axis, 1, rect3d_relpointer_axis_offset),
                       ==, 1));
    test_fail_if(!test_compare_ii(off, ==, 12 * 13));
    test_fail_if(!test_compare_ii(axis.coffset, ==, 1));
    test_fail_if(!test_compare_poff(axis.coords, cdo.zv, cdo.stm + 3));
    test_fail_if(!test_compare_ii(axis.index.voffset, ==, 12 * 13));
    test_fail_if(!test_compare_ii(axis.index.nbpoints, ==, 6));
    test_fail_if(!test_compare_ii(axis.index.nfpoints, ==, 7));

    test_fail_if(
      !test_compare_ii(rect3d_relpointer_axis__ndgetnvf(
                         &off, &axis, 2, rect3d_relpointer_axis_offset),
                       ==, 1));
    test_fail_if(!test_compare_ii(off, ==, 3 * 12 * 13));
    test_fail_if(!test_compare_ii(axis.coffset, ==, 1));
    test_fail_if(!test_compare_poff(axis.coords, cdo.zv, cdo.stm + 3));
    test_fail_if(!test_compare_ii(axis.index.voffset, ==, 12 * 13));
    test_fail_if(!test_compare_ii(axis.index.nbpoints, ==, 6));
    test_fail_if(!test_compare_ii(axis.index.nfpoints, ==, 7));

    test_fail_if(
      !test_compare_ii(rect3d_relpointer_axis__ndgetnvf(
                         &off, &axis, 9, rect3d_relpointer_axis_offset),
                       ==, 0));
    test_fail_if(!test_compare_ii(off, ==, 3 * 12 * 13));
    test_fail_if(!test_compare_ii(axis.coffset, ==, 1));
    test_fail_if(!test_compare_poff(axis.coords, cdo.zv, cdo.stm + 3));
    test_fail_if(!test_compare_ii(axis.index.voffset, ==, 12 * 13));
    test_fail_if(!test_compare_ii(axis.index.nbpoints, ==, 6));
    test_fail_if(!test_compare_ii(axis.index.nfpoints, ==, 7));

    off = 0;
    test_fail_if(
      !test_compare_ii(rect3d_relpointer_axis__ndgetnvf(
                         &off, &axis, 0, rect3d_relpointer_axis_absoffset),
                       ==, 1));
    test_fail_if(!test_compare_ii(off, ==, 0));
    test_fail_if(!test_compare_ii(axis.coffset, ==, 1));
    test_fail_if(!test_compare_poff(axis.coords, cdo.zv, cdo.stm + 3));
    test_fail_if(!test_compare_ii(axis.index.voffset, ==, 12 * 13));
    test_fail_if(!test_compare_ii(axis.index.nbpoints, ==, 6));
    test_fail_if(!test_compare_ii(axis.index.nfpoints, ==, 7));

    test_fail_if(
      !test_compare_ii(rect3d_relpointer_axis__ndgetnvf(
                         &off, &axis, 1, rect3d_relpointer_axis_absoffset),
                       ==, 1));
    test_fail_if(!test_compare_ii(off, ==, 12 * 13));
    test_fail_if(!test_compare_ii(axis.coffset, ==, 1));
    test_fail_if(!test_compare_poff(axis.coords, cdo.zv, cdo.stm + 3));
    test_fail_if(!test_compare_ii(axis.index.voffset, ==, 12 * 13));
    test_fail_if(!test_compare_ii(axis.index.nbpoints, ==, 6));
    test_fail_if(!test_compare_ii(axis.index.nfpoints, ==, 7));

    test_fail_if(
      !test_compare_ii(rect3d_relpointer_axis__ndgetnvf(
                         &off, &axis, 2, rect3d_relpointer_axis_absoffset),
                       ==, 1));
    test_fail_if(!test_compare_ii(off, ==, 3 * 12 * 13));
    test_fail_if(!test_compare_ii(axis.coffset, ==, 1));
    test_fail_if(!test_compare_poff(axis.coords, cdo.zv, cdo.stm + 3));
    test_fail_if(!test_compare_ii(axis.index.voffset, ==, 12 * 13));
    test_fail_if(!test_compare_ii(axis.index.nbpoints, ==, 6));
    test_fail_if(!test_compare_ii(axis.index.nfpoints, ==, 7));

    test_fail_if(
      !test_compare_ii(rect3d_relpointer_axis__ndgetnvf(
                         &off, &axis, 9, rect3d_relpointer_axis_absoffset),
                       ==, 0));
    test_fail_if(!test_compare_ii(off, ==, 3 * 12 * 13));
    test_fail_if(!test_compare_ii(axis.coffset, ==, 1));
    test_fail_if(!test_compare_poff(axis.coords, cdo.zv, cdo.stm + 3));
    test_fail_if(!test_compare_ii(axis.index.voffset, ==, 12 * 13));
    test_fail_if(!test_compare_ii(axis.index.nbpoints, ==, 6));
    test_fail_if(!test_compare_ii(axis.index.nfpoints, ==, 7));

    //---
    axis = rect3d_relpointer_axis_Zvm(&cdo, -1, cdo.stm + 3);
    test_fail_if(!test_compare_ii(axis.coffset, ==, -1));
    test_fail_if(!test_compare_poff(axis.coords, cdo.zv, cdo.stm + 3));
    test_fail_if(!test_compare_ii(axis.index.voffset, ==, -12 * 13));
    test_fail_if(!test_compare_ii(axis.index.nbpoints, ==, 7));
    test_fail_if(!test_compare_ii(axis.index.nfpoints, ==, 6));

    off = 0;
    test_fail_if(
      !test_compare_ii(rect3d_relpointer_axis__ndgetnvf(
                         &off, &axis, 0, rect3d_relpointer_axis_offset),
                       ==, 1));
    test_fail_if(!test_compare_ii(off, ==, 0));
    test_fail_if(!test_compare_ii(axis.coffset, ==, -1));
    test_fail_if(!test_compare_poff(axis.coords, cdo.zv, cdo.stm + 3));
    test_fail_if(!test_compare_ii(axis.index.voffset, ==, -12 * 13));
    test_fail_if(!test_compare_ii(axis.index.nbpoints, ==, 7));
    test_fail_if(!test_compare_ii(axis.index.nfpoints, ==, 6));

    test_fail_if(
      !test_compare_ii(rect3d_relpointer_axis__ndgetnvf(
                         &off, &axis, 1, rect3d_relpointer_axis_offset),
                       ==, 1));
    test_fail_if(!test_compare_ii(off, ==, -12 * 13));
    test_fail_if(!test_compare_ii(axis.coffset, ==, -1));
    test_fail_if(!test_compare_poff(axis.coords, cdo.zv, cdo.stm + 3));
    test_fail_if(!test_compare_ii(axis.index.voffset, ==, -12 * 13));
    test_fail_if(!test_compare_ii(axis.index.nbpoints, ==, 7));
    test_fail_if(!test_compare_ii(axis.index.nfpoints, ==, 6));

    test_fail_if(
      !test_compare_ii(rect3d_relpointer_axis__ndgetnvf(
                         &off, &axis, 2, rect3d_relpointer_axis_offset),
                       ==, 1));
    test_fail_if(!test_compare_ii(off, ==, -3 * 12 * 13));
    test_fail_if(!test_compare_ii(axis.coffset, ==, -1));
    test_fail_if(!test_compare_poff(axis.coords, cdo.zv, cdo.stm + 3));
    test_fail_if(!test_compare_ii(axis.index.voffset, ==, -12 * 13));
    test_fail_if(!test_compare_ii(axis.index.nbpoints, ==, 7));
    test_fail_if(!test_compare_ii(axis.index.nfpoints, ==, 6));

    test_fail_if(
      !test_compare_ii(rect3d_relpointer_axis__ndgetnvf(
                         &off, &axis, 9, rect3d_relpointer_axis_offset),
                       ==, 0));
    test_fail_if(!test_compare_ii(off, ==, -3 * 12 * 13));
    test_fail_if(!test_compare_ii(axis.coffset, ==, -1));
    test_fail_if(!test_compare_poff(axis.coords, cdo.zv, cdo.stm + 3));
    test_fail_if(!test_compare_ii(axis.index.voffset, ==, -12 * 13));
    test_fail_if(!test_compare_ii(axis.index.nbpoints, ==, 7));
    test_fail_if(!test_compare_ii(axis.index.nfpoints, ==, 6));

    off = 0;
    test_fail_if(
      !test_compare_ii(rect3d_relpointer_axis__ndgetnvf(
                         &off, &axis, 0, rect3d_relpointer_axis_absoffset),
                       ==, 1));
    test_fail_if(!test_compare_ii(off, ==, 0));
    test_fail_if(!test_compare_ii(axis.coffset, ==, -1));
    test_fail_if(!test_compare_poff(axis.coords, cdo.zv, cdo.stm + 3));
    test_fail_if(!test_compare_ii(axis.index.voffset, ==, -12 * 13));
    test_fail_if(!test_compare_ii(axis.index.nbpoints, ==, 7));
    test_fail_if(!test_compare_ii(axis.index.nfpoints, ==, 6));

    test_fail_if(
      !test_compare_ii(rect3d_relpointer_axis__ndgetnvf(
                         &off, &axis, 1, rect3d_relpointer_axis_absoffset),
                       ==, 1));
    test_fail_if(!test_compare_ii(off, ==, 12 * 13));
    test_fail_if(!test_compare_ii(axis.coffset, ==, -1));
    test_fail_if(!test_compare_poff(axis.coords, cdo.zv, cdo.stm + 3));
    test_fail_if(!test_compare_ii(axis.index.voffset, ==, -12 * 13));
    test_fail_if(!test_compare_ii(axis.index.nbpoints, ==, 7));
    test_fail_if(!test_compare_ii(axis.index.nfpoints, ==, 6));

    test_fail_if(
      !test_compare_ii(rect3d_relpointer_axis__ndgetnvf(
                         &off, &axis, 2, rect3d_relpointer_axis_absoffset),
                       ==, 1));
    test_fail_if(!test_compare_ii(off, ==, 3 * 12 * 13));
    test_fail_if(!test_compare_ii(axis.coffset, ==, -1));
    test_fail_if(!test_compare_poff(axis.coords, cdo.zv, cdo.stm + 3));
    test_fail_if(!test_compare_ii(axis.index.voffset, ==, -12 * 13));
    test_fail_if(!test_compare_ii(axis.index.nbpoints, ==, 7));
    test_fail_if(!test_compare_ii(axis.index.nfpoints, ==, 6));

    test_fail_if(
      !test_compare_ii(rect3d_relpointer_axis__ndgetnvf(
                         &off, &axis, 9, rect3d_relpointer_axis_absoffset),
                       ==, 0));
    test_fail_if(!test_compare_ii(off, ==, 3 * 12 * 13));
    test_fail_if(!test_compare_ii(axis.coffset, ==, -1));
    test_fail_if(!test_compare_poff(axis.coords, cdo.zv, cdo.stm + 3));
    test_fail_if(!test_compare_ii(axis.index.voffset, ==, -12 * 13));
    test_fail_if(!test_compare_ii(axis.index.nbpoints, ==, 7));
    test_fail_if(!test_compare_ii(axis.index.nfpoints, ==, 6));
  }

  return r;
}
