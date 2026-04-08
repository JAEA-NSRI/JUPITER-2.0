
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

int rect3d_axis_X_test(void)
{
  int r;
  domain cdo;
  mpi_param mpi;
  struct rect3d_relpointer_axis axis;

  set_self_comm_mpi(&mpi);

  r = 0;

  if (!make_cdo_c(&mpi, &cdo, 12, 13, 14, 3, 4, 0.5, 6.0, 70.0))
    return 1;

  axis = rect3d_relpointer_axis_Xcm(&cdo, 1, 0);
  test_fail_if(!test_compare_ii(axis.coffset, ==, 1));
  test_fail_if(!test_compare_pp(axis.coords, ==, cdo.xc));
  test_fail_if(!test_compare_ii(axis.index.voffset, ==, 1));
  test_fail_if(!test_compare_ii(axis.index.nbpoints, ==, 0));
  test_fail_if(!test_compare_ii(axis.index.nfpoints, ==, 11));

  axis = rect3d_relpointer_axis_Xcm(&cdo, -1, 0);
  test_fail_if(!test_compare_ii(axis.coffset, ==, -1));
  test_fail_if(!test_compare_pp(axis.coords, ==, cdo.xc));
  test_fail_if(!test_compare_ii(axis.index.voffset, ==, -1));
  test_fail_if(!test_compare_ii(axis.index.nbpoints, ==, 11));
  test_fail_if(!test_compare_ii(axis.index.nfpoints, ==, 0));

  axis = rect3d_relpointer_axis_Xvm(&cdo, 1, 0);
  test_fail_if(!test_compare_ii(axis.coffset, ==, 1));
  test_fail_if(!test_compare_pp(axis.coords, ==, cdo.xv));
  test_fail_if(!test_compare_ii(axis.index.voffset, ==, 1));
  test_fail_if(!test_compare_ii(axis.index.nbpoints, ==, 0));
  test_fail_if(!test_compare_ii(axis.index.nfpoints, ==, 11));

  axis = rect3d_relpointer_axis_Xc(&cdo, 1, cdo.stm, 0, 0);
  test_fail_if(!test_compare_ii(axis.coffset, ==, 1));
  test_fail_if(!test_compare_poff(axis.coords, cdo.xc, cdo.stm));
  test_fail_if(!test_compare_ii(axis.index.voffset, ==, 1));
  test_fail_if(!test_compare_ii(axis.index.nbpoints, ==, 0));
  test_fail_if(!test_compare_ii(axis.index.nfpoints, ==, 4));

  axis = rect3d_relpointer_axis_Xv(&cdo, 1, cdo.stm, 0, 0);
  test_fail_if(!test_compare_ii(axis.coffset, ==, 1));
  test_fail_if(!test_compare_poff(axis.coords, cdo.xv, cdo.stm));
  test_fail_if(!test_compare_ii(axis.index.voffset, ==, 1));
  test_fail_if(!test_compare_ii(axis.index.nbpoints, ==, 0));
  test_fail_if(!test_compare_ii(axis.index.nfpoints, ==, 5));

  test_fail_if(
    !test_compare_ii(rect3d_relpointer_axis_movable(&axis, -1), ==, 0));
  test_fail_if(
    !test_compare_ii(rect3d_relpointer_axis_movable(&axis, 1), ==, 1));

  test_fail_if(
    !test_compare_ii(rect3d_relpointer_axis_absmovable(&axis, -1), ==, 0));
  test_fail_if(
    !test_compare_ii(rect3d_relpointer_axis_absmovable(&axis, 1), ==, 1));

  test_fail_if(!test_compare_ii(rect3d_relpointer_axis_coffset(&axis), ==, 1));
  test_fail_if(!test_compare_ii(rect3d_relpointer_axis_voffset(&axis), ==, 1));
  test_fail_if(
    !test_compare_ii(rect3d_relpointer_axis_abscoffset(&axis), ==, 1));
  test_fail_if(
    !test_compare_ii(rect3d_relpointer_axis_absvoffset(&axis), ==, 1));

  test_fail_if(!test_compare_ii(rect3d_relpointer_axis_nfwdpts(&axis), ==, 5));
  test_fail_if(!test_compare_ii(rect3d_relpointer_axis_nbkwdpts(&axis), ==, 0));
  test_fail_if(!test_compare_ii(rect3d_relpointer_axis_npospts(&axis), ==, 5));
  test_fail_if(!test_compare_ii(rect3d_relpointer_axis_nnegpts(&axis), ==, 0));

  test_fail_if(
    !test_compare_eps(rect3d_relpointer_axis_coords(&axis), 0.0, 1.0e-6));

  test_fail_if(!test_compare_ii(rect3d_relpointer_axis_move(&axis, 1), ==, 1));
  test_fail_if(!test_compare_ii(axis.coffset, ==, 1));
  test_fail_if(!test_compare_poff(axis.coords, cdo.xv, cdo.stm + 1));
  test_fail_if(!test_compare_ii(axis.index.voffset, ==, 1));
  test_fail_if(!test_compare_ii(axis.index.nbpoints, ==, 1));
  test_fail_if(!test_compare_ii(axis.index.nfpoints, ==, 4));
  test_fail_if(
    !test_compare_eps(rect3d_relpointer_axis_coords(&axis), 0.1, 1.0e-6));

  test_fail_if(!test_compare_ii(rect3d_relpointer_axis_move(&axis, 3), ==, 3));
  test_fail_if(!test_compare_ii(axis.coffset, ==, 1));
  test_fail_if(!test_compare_poff(axis.coords, cdo.xv, cdo.stm + 4));
  test_fail_if(!test_compare_ii(axis.index.voffset, ==, 1));
  test_fail_if(!test_compare_ii(axis.index.nbpoints, ==, 4));
  test_fail_if(!test_compare_ii(axis.index.nfpoints, ==, 1));
  test_fail_if(
    !test_compare_eps(rect3d_relpointer_axis_coords(&axis), 0.4, 1.0e-6));

  test_fail_if(
    !test_compare_ii(rect3d_relpointer_axis_move(&axis, -2), ==, -2));
  test_fail_if(!test_compare_ii(axis.coffset, ==, 1));
  test_fail_if(!test_compare_poff(axis.coords, cdo.xv, cdo.stm + 2));
  test_fail_if(!test_compare_ii(axis.index.voffset, ==, 1));
  test_fail_if(!test_compare_ii(axis.index.nbpoints, ==, 2));
  test_fail_if(!test_compare_ii(axis.index.nfpoints, ==, 3));
  test_fail_if(
    !test_compare_eps(rect3d_relpointer_axis_coords(&axis), 0.2, 1.0e-6));

  test_fail_if(
    !test_compare_ii(rect3d_relpointer_axis_absmove(&axis, 3), ==, 3));
  test_fail_if(!test_compare_ii(axis.coffset, ==, 1));
  test_fail_if(!test_compare_poff(axis.coords, cdo.xv, cdo.stm + 5));
  test_fail_if(!test_compare_ii(axis.index.voffset, ==, 1));
  test_fail_if(!test_compare_ii(axis.index.nbpoints, ==, 5));
  test_fail_if(!test_compare_ii(axis.index.nfpoints, ==, 0));
  test_fail_if(
    !test_compare_eps(rect3d_relpointer_axis_coords(&axis), 0.5, 1.0e-6));

  test_fail_if(
    !test_compare_ii(rect3d_relpointer_axis_absmove(&axis, -2), ==, -2));
  test_fail_if(!test_compare_ii(axis.coffset, ==, 1));
  test_fail_if(!test_compare_poff(axis.coords, cdo.xv, cdo.stm + 3));
  test_fail_if(!test_compare_ii(axis.index.voffset, ==, 1));
  test_fail_if(!test_compare_ii(axis.index.nbpoints, ==, 3));
  test_fail_if(!test_compare_ii(axis.index.nfpoints, ==, 2));
  test_fail_if(
    !test_compare_eps(rect3d_relpointer_axis_coords(&axis), 0.3, 1.0e-6));

  //--- rev
  axis = rect3d_relpointer_axis_Xv(&cdo, -1, cdo.stm + 5, 0, 0);
  test_fail_if(!test_compare_ii(axis.coffset, ==, -1));
  test_fail_if(!test_compare_poff(axis.coords, cdo.xv, cdo.stm + 5));
  test_fail_if(!test_compare_ii(axis.index.voffset, ==, -1));
  test_fail_if(!test_compare_ii(axis.index.nbpoints, ==, 0));
  test_fail_if(!test_compare_ii(axis.index.nfpoints, ==, 5));

  test_fail_if(
    !test_compare_ii(rect3d_relpointer_axis_movable(&axis, -1), ==, 0));
  test_fail_if(
    !test_compare_ii(rect3d_relpointer_axis_movable(&axis, 1), ==, 1));

  test_fail_if(
    !test_compare_ii(rect3d_relpointer_axis_absmovable(&axis, -1), ==, 1));
  test_fail_if(
    !test_compare_ii(rect3d_relpointer_axis_absmovable(&axis, 1), ==, 0));

  test_fail_if(!test_compare_ii(rect3d_relpointer_axis_coffset(&axis), ==, -1));
  test_fail_if(!test_compare_ii(rect3d_relpointer_axis_voffset(&axis), ==, -1));
  test_fail_if(
    !test_compare_ii(rect3d_relpointer_axis_abscoffset(&axis), ==, 1));
  test_fail_if(
    !test_compare_ii(rect3d_relpointer_axis_absvoffset(&axis), ==, 1));

  test_fail_if(!test_compare_ii(rect3d_relpointer_axis_nfwdpts(&axis), ==, 5));
  test_fail_if(!test_compare_ii(rect3d_relpointer_axis_nbkwdpts(&axis), ==, 0));
  test_fail_if(!test_compare_ii(rect3d_relpointer_axis_npospts(&axis), ==, 0));
  test_fail_if(!test_compare_ii(rect3d_relpointer_axis_nnegpts(&axis), ==, 5));

  test_fail_if(
    !test_compare_eps(rect3d_relpointer_axis_coords(&axis), 0.5, 1.0e-6));

  test_fail_if(!test_compare_ii(rect3d_relpointer_axis_move(&axis, 1), ==, -1));
  test_fail_if(!test_compare_ii(axis.coffset, ==, -1));
  test_fail_if(!test_compare_poff(axis.coords, cdo.xv, cdo.stm + 4));
  test_fail_if(!test_compare_ii(axis.index.voffset, ==, -1));
  test_fail_if(!test_compare_ii(axis.index.nbpoints, ==, 1));
  test_fail_if(!test_compare_ii(axis.index.nfpoints, ==, 4));
  test_fail_if(
    !test_compare_eps(rect3d_relpointer_axis_coords(&axis), 0.4, 1.0e-6));

  test_fail_if(!test_compare_ii(rect3d_relpointer_axis_move(&axis, 3), ==, -3));
  test_fail_if(!test_compare_ii(axis.coffset, ==, -1));
  test_fail_if(!test_compare_poff(axis.coords, cdo.xv, cdo.stm + 1));
  test_fail_if(!test_compare_ii(axis.index.voffset, ==, -1));
  test_fail_if(!test_compare_ii(axis.index.nbpoints, ==, 4));
  test_fail_if(!test_compare_ii(axis.index.nfpoints, ==, 1));
  test_fail_if(
    !test_compare_eps(rect3d_relpointer_axis_coords(&axis), 0.1, 1.0e-6));

  test_fail_if(!test_compare_ii(rect3d_relpointer_axis_move(&axis, -2), ==, 2));
  test_fail_if(!test_compare_ii(axis.coffset, ==, -1));
  test_fail_if(!test_compare_poff(axis.coords, cdo.xv, cdo.stm + 3));
  test_fail_if(!test_compare_ii(axis.index.voffset, ==, -1));
  test_fail_if(!test_compare_ii(axis.index.nbpoints, ==, 2));
  test_fail_if(!test_compare_ii(axis.index.nfpoints, ==, 3));
  test_fail_if(
    !test_compare_eps(rect3d_relpointer_axis_coords(&axis), 0.3, 1.0e-6));

  test_fail_if(
    !test_compare_ii(rect3d_relpointer_axis_absmove(&axis, -3), ==, -3));
  test_fail_if(!test_compare_ii(axis.coffset, ==, -1));
  test_fail_if(!test_compare_poff(axis.coords, cdo.xv, cdo.stm));
  test_fail_if(!test_compare_ii(axis.index.voffset, ==, -1));
  test_fail_if(!test_compare_ii(axis.index.nbpoints, ==, 5));
  test_fail_if(!test_compare_ii(axis.index.nfpoints, ==, 0));
  test_fail_if(
    !test_compare_eps(rect3d_relpointer_axis_coords(&axis), 0.0, 1.0e-6));

  test_fail_if(
    !test_compare_ii(rect3d_relpointer_axis_absmove(&axis, 2), ==, 2));
  test_fail_if(!test_compare_ii(axis.coffset, ==, -1));
  test_fail_if(!test_compare_poff(axis.coords, cdo.xv, cdo.stm + 2));
  test_fail_if(!test_compare_ii(axis.index.voffset, ==, -1));
  test_fail_if(!test_compare_ii(axis.index.nbpoints, ==, 3));
  test_fail_if(!test_compare_ii(axis.index.nfpoints, ==, 2));
  test_fail_if(
    !test_compare_eps(rect3d_relpointer_axis_coords(&axis), 0.2, 1.0e-6));

  test_fail_if(
    !test_compare_ii(rect3d_relpointer_axis_move_nearest(&axis, 0.3), ==, 1));
  test_fail_if(
    !test_compare_eps(rect3d_relpointer_axis_coords(&axis), 0.3, 1.0e-6));
  test_fail_if(!test_compare_ii(axis.coffset, ==, -1));
  test_fail_if(!test_compare_poff(axis.coords, cdo.xv, cdo.stm + 3));
  test_fail_if(!test_compare_ii(axis.index.voffset, ==, -1));
  test_fail_if(!test_compare_ii(axis.index.nbpoints, ==, 2));
  test_fail_if(!test_compare_ii(axis.index.nfpoints, ==, 3));

  test_fail_if(
    !test_compare_ii(rect3d_relpointer_axis_move_nearest(&axis, 0.22), ==, -1));
  test_fail_if(
    !test_compare_eps(rect3d_relpointer_axis_coords(&axis), 0.2, 1.0e-6));
  test_fail_if(!test_compare_ii(axis.coffset, ==, -1));
  test_fail_if(!test_compare_poff(axis.coords, cdo.xv, cdo.stm + 2));
  test_fail_if(!test_compare_ii(axis.index.voffset, ==, -1));
  test_fail_if(!test_compare_ii(axis.index.nbpoints, ==, 3));
  test_fail_if(!test_compare_ii(axis.index.nfpoints, ==, 2));

  test_fail_if(
    !test_compare_ii(rect3d_relpointer_axis_move_nearest(&axis, 0.37), ==, 2));
  test_fail_if(
    !test_compare_eps(rect3d_relpointer_axis_coords(&axis), 0.4, 1.0e-6));
  test_fail_if(!test_compare_ii(axis.coffset, ==, -1));
  test_fail_if(!test_compare_poff(axis.coords, cdo.xv, cdo.stm + 4));
  test_fail_if(!test_compare_ii(axis.index.voffset, ==, -1));
  test_fail_if(!test_compare_ii(axis.index.nbpoints, ==, 1));
  test_fail_if(!test_compare_ii(axis.index.nfpoints, ==, 4));

  test_fail_if(
    !test_compare_ii(rect3d_relpointer_axis_move_nearest(&axis, 0.08), ==, -3));
  test_fail_if(
    !test_compare_eps(rect3d_relpointer_axis_coords(&axis), 0.1, 1.0e-6));
  test_fail_if(!test_compare_ii(axis.coffset, ==, -1));
  test_fail_if(!test_compare_poff(axis.coords, cdo.xv, cdo.stm + 1));
  test_fail_if(!test_compare_ii(axis.index.voffset, ==, -1));
  test_fail_if(!test_compare_ii(axis.index.nbpoints, ==, 4));
  test_fail_if(!test_compare_ii(axis.index.nfpoints, ==, 1));

  test_fail_if(
    !test_compare_ii(rect3d_relpointer_axis_move_nearest(&axis, 0.32), ==, 2));
  test_fail_if(
    !test_compare_eps(rect3d_relpointer_axis_coords(&axis), 0.3, 1.0e-6));
  test_fail_if(!test_compare_ii(axis.coffset, ==, -1));
  test_fail_if(!test_compare_poff(axis.coords, cdo.xv, cdo.stm + 3));
  test_fail_if(!test_compare_ii(axis.index.voffset, ==, -1));
  test_fail_if(!test_compare_ii(axis.index.nbpoints, ==, 2));
  test_fail_if(!test_compare_ii(axis.index.nfpoints, ==, 3));

  test_fail_if(
    !test_compare_ii(rect3d_relpointer_axis_move_nearest(&axis, cdo.xv[4]), ==,
                     -2)); /* 0.1 */
  test_fail_if(
    !test_compare_dd(rect3d_relpointer_axis_coords(&axis), ==, cdo.xv[4]));

  test_fail_if(
    !test_compare_ii(rect3d_relpointer_axis_move_nearest(&axis, cdo.xv[6]), ==,
                     2)); /* 0.3 */
  test_fail_if(
    !test_compare_dd(rect3d_relpointer_axis_coords(&axis), ==, cdo.xv[6]));

  test_fail_if(
    !test_compare_ii(rect3d_relpointer_axis_move_nearest(&axis, cdo.xv[6]), ==,
                     0));
  test_fail_if(
    !test_compare_dd(rect3d_relpointer_axis_coords(&axis), ==, cdo.xv[6]));

  test_fail_if(
    !test_compare_ii(rect3d_relpointer_axis_move_nearest(&axis, 1000.0), >=,
                     0));
  test_fail_if(!test_compare_ii(rect3d_relpointer_axis_npospts(&axis), ==, 0));

  test_fail_if(
    !test_compare_ii(rect3d_relpointer_axis_move_nearest(&axis, -1000.0), <=,
                     0));
  test_fail_if(!test_compare_ii(rect3d_relpointer_axis_nnegpts(&axis), ==, 0));

  axis = rect3d_relpointer_axis_Xvm(&cdo, 2, 4);
  test_fail_if(!test_compare_ii(axis.coffset, ==, 2));
  test_fail_if(!test_compare_poff(axis.coords, cdo.xv, 4));
  test_fail_if(!test_compare_ii(axis.index.voffset, ==, 2));
  test_fail_if(!test_compare_ii(axis.index.nfpoints, ==, 3));
  test_fail_if(!test_compare_ii(axis.index.nbpoints, ==, 2));

  test_fail_if(!test_compare_ii(rect3d_relpointer_axis_move(&axis, 1), ==, 2));
  test_fail_if(!test_compare_ii(axis.coffset, ==, 2));
  test_fail_if(!test_compare_poff(axis.coords, cdo.xv, 6));
  test_fail_if(!test_compare_ii(axis.index.voffset, ==, 2));
  test_fail_if(!test_compare_ii(axis.index.nfpoints, ==, 2));
  test_fail_if(!test_compare_ii(axis.index.nbpoints, ==, 3));

  test_fail_if(
    !test_compare_ii(rect3d_relpointer_axis_move(&axis, -2), ==, -4));
  test_fail_if(!test_compare_ii(axis.coffset, ==, 2));
  test_fail_if(!test_compare_poff(axis.coords, cdo.xv, 2));
  test_fail_if(!test_compare_ii(axis.index.voffset, ==, 2));
  test_fail_if(!test_compare_ii(axis.index.nfpoints, ==, 4));
  test_fail_if(!test_compare_ii(axis.index.nbpoints, ==, 1));

  test_fail_if(
    !test_compare_eps(rect3d_relpointer_axis_coords(&axis), -0.1, 1e-6));

  test_fail_if(
    !test_compare_ii(rect3d_relpointer_axis_move_nearest(&axis, 0.48), ==, 6));
  test_fail_if(
    !test_compare_eps(rect3d_relpointer_axis_coords(&axis), 0.5, 1e-6));

  test_fail_if(!test_compare_eps(cdo.xv[2], -0.1, 1.0e-6));
  test_fail_if(!test_compare_eps(cdo.xv[4], 0.1, 1.0e-6));
  if (cdo.xv[2] + cdo.xv[4] < 0.0) { /* near to 0.1 */
    test_fail_if(
      !test_compare_ii(rect3d_relpointer_axis_move_nearest(&axis, 0.0), ==,
                       -4));
    test_fail_if(
      !test_compare_eps(rect3d_relpointer_axis_coords(&axis), 0.1, 1e-6));
  } else {
    test_fail_if(
      !test_compare_ii(rect3d_relpointer_axis_move_nearest(&axis, 0.0), ==,
                       -6));
    test_fail_if(
      !test_compare_eps(rect3d_relpointer_axis_coords(&axis), -0.1, 1e-6));
  }

  rect3d_relpointer_axis_move(&axis, -rect3d_relpointer_axis_nbkwdpts(&axis));

  if (cdo.xv[2] + cdo.xv[4] < 0.0) { /* near to 0.1 */
    test_fail_if(
      !test_compare_ii(rect3d_relpointer_axis_move_nearest(&axis, 0.0), ==, 4));
    test_fail_if(
      !test_compare_eps(rect3d_relpointer_axis_coords(&axis), 0.1, 1e-6));
  } else {
    test_fail_if(
      !test_compare_ii(rect3d_relpointer_axis_move_nearest(&axis, 0.0), ==, 2));
    test_fail_if(
      !test_compare_eps(rect3d_relpointer_axis_coords(&axis), -0.1, 1e-6));
  }

  axis = rect3d_relpointer_axis_Xv(&cdo, -1, cdo.stm + 5, 0, 0);
  test_fail_if(!test_compare_ii(axis.coffset, ==, -1));
  test_fail_if(!test_compare_poff(axis.coords, cdo.xv, cdo.stm + 5));
  test_fail_if(!test_compare_ii(axis.index.voffset, ==, -1));
  test_fail_if(!test_compare_ii(axis.index.nbpoints, ==, 0));
  test_fail_if(!test_compare_ii(axis.index.nfpoints, ==, 5));

  rect3d_relpointer_axis_flip(&axis);
  test_fail_if(!test_compare_ii(axis.coffset, ==, 1));
  test_fail_if(!test_compare_poff(axis.coords, cdo.xv, cdo.stm + 5));
  test_fail_if(!test_compare_ii(axis.index.voffset, ==, 1));
  test_fail_if(!test_compare_ii(axis.index.nbpoints, ==, 5));
  test_fail_if(!test_compare_ii(axis.index.nfpoints, ==, 0));

  {
    ptrdiff_t off;

    axis = rect3d_relpointer_axis_Xvm(&cdo, 1, cdo.stm + 2);
    test_fail_if(!test_compare_ii(axis.coffset, ==, 1));
    test_fail_if(!test_compare_poff(axis.coords, cdo.xv, cdo.stm + 2));
    test_fail_if(!test_compare_ii(axis.index.voffset, ==, 1));
    test_fail_if(!test_compare_ii(axis.index.nbpoints, ==, 5));
    test_fail_if(!test_compare_ii(axis.index.nfpoints, ==, 6));

    off = 0;
    test_fail_if(
      !test_compare_ii(rect3d_relpointer_axis__ndgetnvf(
                         &off, &axis, 0, rect3d_relpointer_axis_offset),
                       ==, 1));
    test_fail_if(!test_compare_ii(off, ==, 0));
    test_fail_if(!test_compare_ii(axis.coffset, ==, 1));
    test_fail_if(!test_compare_poff(axis.coords, cdo.xv, cdo.stm + 2));
    test_fail_if(!test_compare_ii(axis.index.voffset, ==, 1));
    test_fail_if(!test_compare_ii(axis.index.nbpoints, ==, 5));
    test_fail_if(!test_compare_ii(axis.index.nfpoints, ==, 6));

    test_fail_if(
      !test_compare_ii(rect3d_relpointer_axis__ndgetnvf(
                         &off, &axis, 1, rect3d_relpointer_axis_offset),
                       ==, 1));
    test_fail_if(!test_compare_ii(off, ==, 1));
    test_fail_if(!test_compare_ii(axis.coffset, ==, 1));
    test_fail_if(!test_compare_poff(axis.coords, cdo.xv, cdo.stm + 2));
    test_fail_if(!test_compare_ii(axis.index.voffset, ==, 1));
    test_fail_if(!test_compare_ii(axis.index.nbpoints, ==, 5));
    test_fail_if(!test_compare_ii(axis.index.nfpoints, ==, 6));

    test_fail_if(
      !test_compare_ii(rect3d_relpointer_axis__ndgetnvf(
                         &off, &axis, 2, rect3d_relpointer_axis_offset),
                       ==, 1));
    test_fail_if(!test_compare_ii(off, ==, 3));
    test_fail_if(!test_compare_ii(axis.coffset, ==, 1));
    test_fail_if(!test_compare_poff(axis.coords, cdo.xv, cdo.stm + 2));
    test_fail_if(!test_compare_ii(axis.index.voffset, ==, 1));
    test_fail_if(!test_compare_ii(axis.index.nbpoints, ==, 5));
    test_fail_if(!test_compare_ii(axis.index.nfpoints, ==, 6));

    test_fail_if(
      !test_compare_ii(rect3d_relpointer_axis__ndgetnvf(
                         &off, &axis, 9, rect3d_relpointer_axis_offset),
                       ==, 0));
    test_fail_if(!test_compare_ii(off, ==, 3));
    test_fail_if(!test_compare_ii(axis.coffset, ==, 1));
    test_fail_if(!test_compare_poff(axis.coords, cdo.xv, cdo.stm + 2));
    test_fail_if(!test_compare_ii(axis.index.voffset, ==, 1));
    test_fail_if(!test_compare_ii(axis.index.nbpoints, ==, 5));
    test_fail_if(!test_compare_ii(axis.index.nfpoints, ==, 6));

    off = 0;
    test_fail_if(
      !test_compare_ii(rect3d_relpointer_axis__ndgetnvf(
                         &off, &axis, 0, rect3d_relpointer_axis_absoffset),
                       ==, 1));
    test_fail_if(!test_compare_ii(off, ==, 0));
    test_fail_if(!test_compare_ii(axis.coffset, ==, 1));
    test_fail_if(!test_compare_poff(axis.coords, cdo.xv, cdo.stm + 2));
    test_fail_if(!test_compare_ii(axis.index.voffset, ==, 1));
    test_fail_if(!test_compare_ii(axis.index.nbpoints, ==, 5));
    test_fail_if(!test_compare_ii(axis.index.nfpoints, ==, 6));

    test_fail_if(
      !test_compare_ii(rect3d_relpointer_axis__ndgetnvf(
                         &off, &axis, 1, rect3d_relpointer_axis_absoffset),
                       ==, 1));
    test_fail_if(!test_compare_ii(off, ==, 1));
    test_fail_if(!test_compare_ii(axis.coffset, ==, 1));
    test_fail_if(!test_compare_poff(axis.coords, cdo.xv, cdo.stm + 2));
    test_fail_if(!test_compare_ii(axis.index.voffset, ==, 1));
    test_fail_if(!test_compare_ii(axis.index.nbpoints, ==, 5));
    test_fail_if(!test_compare_ii(axis.index.nfpoints, ==, 6));

    test_fail_if(
      !test_compare_ii(rect3d_relpointer_axis__ndgetnvf(
                         &off, &axis, 2, rect3d_relpointer_axis_absoffset),
                       ==, 1));
    test_fail_if(!test_compare_ii(off, ==, 3));
    test_fail_if(!test_compare_ii(axis.coffset, ==, 1));
    test_fail_if(!test_compare_poff(axis.coords, cdo.xv, cdo.stm + 2));
    test_fail_if(!test_compare_ii(axis.index.voffset, ==, 1));
    test_fail_if(!test_compare_ii(axis.index.nbpoints, ==, 5));
    test_fail_if(!test_compare_ii(axis.index.nfpoints, ==, 6));

    test_fail_if(
      !test_compare_ii(rect3d_relpointer_axis__ndgetnvf(
                         &off, &axis, 9, rect3d_relpointer_axis_absoffset),
                       ==, 0));
    test_fail_if(!test_compare_ii(off, ==, 3));
    test_fail_if(!test_compare_ii(axis.coffset, ==, 1));
    test_fail_if(!test_compare_poff(axis.coords, cdo.xv, cdo.stm + 2));
    test_fail_if(!test_compare_ii(axis.index.voffset, ==, 1));
    test_fail_if(!test_compare_ii(axis.index.nbpoints, ==, 5));
    test_fail_if(!test_compare_ii(axis.index.nfpoints, ==, 6));

    //---
    axis = rect3d_relpointer_axis_Xvm(&cdo, -1, cdo.stm + 3);
    test_fail_if(!test_compare_ii(axis.coffset, ==, -1));
    test_fail_if(!test_compare_poff(axis.coords, cdo.xv, cdo.stm + 3));
    test_fail_if(!test_compare_ii(axis.index.voffset, ==, -1));
    test_fail_if(!test_compare_ii(axis.index.nbpoints, ==, 5));
    test_fail_if(!test_compare_ii(axis.index.nfpoints, ==, 6));

    off = 0;
    test_fail_if(
      !test_compare_ii(rect3d_relpointer_axis__ndgetnvf(
                         &off, &axis, 0, rect3d_relpointer_axis_offset),
                       ==, 1));
    test_fail_if(!test_compare_ii(off, ==, 0));
    test_fail_if(!test_compare_ii(axis.coffset, ==, -1));
    test_fail_if(!test_compare_poff(axis.coords, cdo.xv, cdo.stm + 3));
    test_fail_if(!test_compare_ii(axis.index.voffset, ==, -1));
    test_fail_if(!test_compare_ii(axis.index.nbpoints, ==, 5));
    test_fail_if(!test_compare_ii(axis.index.nfpoints, ==, 6));

    test_fail_if(
      !test_compare_ii(rect3d_relpointer_axis__ndgetnvf(
                         &off, &axis, 1, rect3d_relpointer_axis_offset),
                       ==, 1));
    test_fail_if(!test_compare_ii(off, ==, -1));
    test_fail_if(!test_compare_ii(axis.coffset, ==, -1));
    test_fail_if(!test_compare_poff(axis.coords, cdo.xv, cdo.stm + 3));
    test_fail_if(!test_compare_ii(axis.index.voffset, ==, -1));
    test_fail_if(!test_compare_ii(axis.index.nbpoints, ==, 5));
    test_fail_if(!test_compare_ii(axis.index.nfpoints, ==, 6));

    test_fail_if(
      !test_compare_ii(rect3d_relpointer_axis__ndgetnvf(
                         &off, &axis, 2, rect3d_relpointer_axis_offset),
                       ==, 1));
    test_fail_if(!test_compare_ii(off, ==, -3));
    test_fail_if(!test_compare_ii(axis.coffset, ==, -1));
    test_fail_if(!test_compare_poff(axis.coords, cdo.xv, cdo.stm + 3));
    test_fail_if(!test_compare_ii(axis.index.voffset, ==, -1));
    test_fail_if(!test_compare_ii(axis.index.nbpoints, ==, 5));
    test_fail_if(!test_compare_ii(axis.index.nfpoints, ==, 6));

    test_fail_if(
      !test_compare_ii(rect3d_relpointer_axis__ndgetnvf(
                         &off, &axis, 9, rect3d_relpointer_axis_offset),
                       ==, 0));
    test_fail_if(!test_compare_ii(off, ==, -3));
    test_fail_if(!test_compare_ii(axis.coffset, ==, -1));
    test_fail_if(!test_compare_poff(axis.coords, cdo.xv, cdo.stm + 3));
    test_fail_if(!test_compare_ii(axis.index.voffset, ==, -1));
    test_fail_if(!test_compare_ii(axis.index.nbpoints, ==, 5));
    test_fail_if(!test_compare_ii(axis.index.nfpoints, ==, 6));

    off = 0;
    test_fail_if(
      !test_compare_ii(rect3d_relpointer_axis__ndgetnvf(
                         &off, &axis, 0, rect3d_relpointer_axis_absoffset),
                       ==, 1));
    test_fail_if(!test_compare_ii(off, ==, 0));
    test_fail_if(!test_compare_ii(axis.coffset, ==, -1));
    test_fail_if(!test_compare_poff(axis.coords, cdo.xv, cdo.stm + 3));
    test_fail_if(!test_compare_ii(axis.index.voffset, ==, -1));
    test_fail_if(!test_compare_ii(axis.index.nbpoints, ==, 5));
    test_fail_if(!test_compare_ii(axis.index.nfpoints, ==, 6));

    test_fail_if(
      !test_compare_ii(rect3d_relpointer_axis__ndgetnvf(
                         &off, &axis, 1, rect3d_relpointer_axis_absoffset),
                       ==, 1));
    test_fail_if(!test_compare_ii(off, ==, 1));
    test_fail_if(!test_compare_ii(axis.coffset, ==, -1));
    test_fail_if(!test_compare_poff(axis.coords, cdo.xv, cdo.stm + 3));
    test_fail_if(!test_compare_ii(axis.index.voffset, ==, -1));
    test_fail_if(!test_compare_ii(axis.index.nbpoints, ==, 5));
    test_fail_if(!test_compare_ii(axis.index.nfpoints, ==, 6));

    test_fail_if(
      !test_compare_ii(rect3d_relpointer_axis__ndgetnvf(
                         &off, &axis, 2, rect3d_relpointer_axis_absoffset),
                       ==, 1));
    test_fail_if(!test_compare_ii(off, ==, 3));
    test_fail_if(!test_compare_ii(axis.coffset, ==, -1));
    test_fail_if(!test_compare_poff(axis.coords, cdo.xv, cdo.stm + 3));
    test_fail_if(!test_compare_ii(axis.index.voffset, ==, -1));
    test_fail_if(!test_compare_ii(axis.index.nbpoints, ==, 5));
    test_fail_if(!test_compare_ii(axis.index.nfpoints, ==, 6));

    test_fail_if(
      !test_compare_ii(rect3d_relpointer_axis__ndgetnvf(
                         &off, &axis, 9, rect3d_relpointer_axis_absoffset),
                       ==, 0));
    test_fail_if(!test_compare_ii(off, ==, 3));
    test_fail_if(!test_compare_ii(axis.coffset, ==, -1));
    test_fail_if(!test_compare_poff(axis.coords, cdo.xv, cdo.stm + 3));
    test_fail_if(!test_compare_ii(axis.index.voffset, ==, -1));
    test_fail_if(!test_compare_ii(axis.index.nbpoints, ==, 5));
    test_fail_if(!test_compare_ii(axis.index.nfpoints, ==, 6));
  }

  return r;
}
