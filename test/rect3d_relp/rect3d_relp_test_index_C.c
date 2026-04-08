
#include <jupiter/rect3d_relp.h>

#include "rect3d_relp_test.h"
#include "test-util.h"

#define test_fail_if(expr) \
  do {                     \
    if (expr)              \
      r = 1;               \
  } while (0)

int rect3d_index_C_test(void)
{
  int r;
  domain cdo;
  struct rect3d_relpointer_index idx;

  r = 0;

  if (!make_cdo_v(&cdo, 1, 1, 2, 0, 0, 0.0, 0.1, 0.0, 0.2, 0.0, 2.0, 4.0))
    return 1;

  idx = rect3d_relpointer_index_C(&cdo, 1, 0, 0, 10);

  test_fail_if(!test_compare_ii(idx.nbpoints, ==, 0));
  test_fail_if(!test_compare_ii(idx.nfpoints, ==, 10));
  test_fail_if(!test_compare_ii(idx.voffset, ==, cdo.m));

  test_fail_if(
    !test_compare_ii(rect3d_relpointer_index_movable(&idx, -1), ==, 0));
  test_fail_if(
    !test_compare_ii(rect3d_relpointer_index_movable(&idx, 1), ==, 1));

  test_fail_if(
    !test_compare_ii(rect3d_relpointer_index_absmovable(&idx, -1), ==, 0));
  test_fail_if(
    !test_compare_ii(rect3d_relpointer_index_absmovable(&idx, 1), ==, 1));

  test_fail_if(
    !test_compare_ii(rect3d_relpointer_index_is_reverse(&idx), ==, 0));

  test_fail_if(
    !test_compare_ii(rect3d_relpointer_index_move(&idx, 1), ==, cdo.m));
  test_fail_if(!test_compare_ii(idx.nbpoints, ==, 1));
  test_fail_if(!test_compare_ii(idx.nfpoints, ==, 9));
  test_fail_if(!test_compare_ii(idx.voffset, ==, cdo.m));

  test_fail_if(
    !test_compare_ii(rect3d_relpointer_index_move(&idx, 3), ==, 3 * cdo.m));
  test_fail_if(!test_compare_ii(idx.nbpoints, ==, 4));
  test_fail_if(!test_compare_ii(idx.nfpoints, ==, 6));
  test_fail_if(!test_compare_ii(idx.voffset, ==, cdo.m));

  test_fail_if(
    !test_compare_ii(rect3d_relpointer_index_move(&idx, -2), ==, -2 * cdo.m));
  test_fail_if(!test_compare_ii(idx.nbpoints, ==, 2));
  test_fail_if(!test_compare_ii(idx.nfpoints, ==, 8));
  test_fail_if(!test_compare_ii(idx.voffset, ==, cdo.m));

  test_fail_if(
    !test_compare_ii(rect3d_relpointer_index_absmove(&idx, 5), ==, 5 * cdo.m));
  test_fail_if(!test_compare_ii(idx.nbpoints, ==, 7));
  test_fail_if(!test_compare_ii(idx.nfpoints, ==, 3));
  test_fail_if(!test_compare_ii(idx.voffset, ==, cdo.m));

  test_fail_if(!test_compare_ii(rect3d_relpointer_index_absmove(&idx, -1), ==,
                                -1 * cdo.m));
  test_fail_if(!test_compare_ii(idx.nbpoints, ==, 6));
  test_fail_if(!test_compare_ii(idx.nfpoints, ==, 4));
  test_fail_if(!test_compare_ii(idx.voffset, ==, cdo.m));

  test_fail_if(!test_compare_ii(rect3d_relpointer_index_nfwdpts(&idx), ==, 4));
  test_fail_if(!test_compare_ii(rect3d_relpointer_index_nbkwdpts(&idx), ==, 6));
  test_fail_if(!test_compare_ii(rect3d_relpointer_index_npospts(&idx), ==, 4));
  test_fail_if(!test_compare_ii(rect3d_relpointer_index_nnegpts(&idx), ==, 6));

  test_fail_if(!test_compare_ii(rect3d_relpointer_index_move(&idx, 99), ==, 0));
  test_fail_if(!test_compare_ii(idx.nbpoints, ==, 6));
  test_fail_if(!test_compare_ii(idx.nfpoints, ==, 4));
  test_fail_if(!test_compare_ii(idx.voffset, ==, cdo.m));

  test_fail_if(
    !test_compare_ii(rect3d_relpointer_index_absmove(&idx, -99), ==, 0));
  test_fail_if(!test_compare_ii(idx.nbpoints, ==, 6));
  test_fail_if(!test_compare_ii(idx.nfpoints, ==, 4));
  test_fail_if(!test_compare_ii(idx.voffset, ==, cdo.m));

  test_fail_if(
    !test_compare_ii(rect3d_relpointer_index_absmove(&idx, 99), ==, 0));
  test_fail_if(!test_compare_ii(idx.nbpoints, ==, 6));
  test_fail_if(!test_compare_ii(idx.nfpoints, ==, 4));
  test_fail_if(!test_compare_ii(idx.voffset, ==, cdo.m));

  test_fail_if(
    !test_compare_ii(rect3d_relpointer_index_move(&idx, -99), ==, 0));
  test_fail_if(!test_compare_ii(idx.nbpoints, ==, 6));
  test_fail_if(!test_compare_ii(idx.nfpoints, ==, 4));
  test_fail_if(!test_compare_ii(idx.voffset, ==, cdo.m));

  idx = rect3d_relpointer_index_C(&cdo, 1, 3, 0, 10);

  test_fail_if(!test_compare_ii(idx.nbpoints, ==, 3));
  test_fail_if(!test_compare_ii(idx.nfpoints, ==, 7));
  test_fail_if(!test_compare_ii(idx.voffset, ==, cdo.m));

  idx = rect3d_relpointer_index_C(&cdo, -1, 10, 0, 10);

  test_fail_if(!test_compare_ii(idx.nbpoints, ==, 0));
  test_fail_if(!test_compare_ii(idx.nfpoints, ==, 10));
  test_fail_if(!test_compare_ii(idx.voffset, ==, -cdo.m));

  test_fail_if(
    !test_compare_ii(rect3d_relpointer_index_movable(&idx, -1), ==, 0));
  test_fail_if(
    !test_compare_ii(rect3d_relpointer_index_movable(&idx, 1), ==, 1));

  test_fail_if(
    !test_compare_ii(rect3d_relpointer_index_absmovable(&idx, -1), ==, 1));
  test_fail_if(
    !test_compare_ii(rect3d_relpointer_index_absmovable(&idx, 1), ==, 0));

  test_fail_if(
    !test_compare_ii(rect3d_relpointer_index_is_reverse(&idx), ==, 1));

  test_fail_if(
    !test_compare_ii(rect3d_relpointer_index_move(&idx, 1), ==, -1 * cdo.m));
  test_fail_if(!test_compare_ii(idx.nbpoints, ==, 1));
  test_fail_if(!test_compare_ii(idx.nfpoints, ==, 9));
  test_fail_if(!test_compare_ii(idx.voffset, ==, -cdo.m));

  test_fail_if(
    !test_compare_ii(rect3d_relpointer_index_move(&idx, 3), ==, -3 * cdo.m));
  test_fail_if(!test_compare_ii(idx.nbpoints, ==, 4));
  test_fail_if(!test_compare_ii(idx.nfpoints, ==, 6));
  test_fail_if(!test_compare_ii(idx.voffset, ==, -cdo.m));

  test_fail_if(
    !test_compare_ii(rect3d_relpointer_index_move(&idx, -2), ==, 2 * cdo.m));
  test_fail_if(!test_compare_ii(idx.nbpoints, ==, 2));
  test_fail_if(!test_compare_ii(idx.nfpoints, ==, 8));
  test_fail_if(!test_compare_ii(idx.voffset, ==, -cdo.m));

  test_fail_if(!test_compare_ii(rect3d_relpointer_index_absmove(&idx, -5), ==,
                                -5 * cdo.m));
  test_fail_if(!test_compare_ii(idx.nbpoints, ==, 7));
  test_fail_if(!test_compare_ii(idx.nfpoints, ==, 3));
  test_fail_if(!test_compare_ii(idx.voffset, ==, -cdo.m));

  test_fail_if(!test_compare_ii(rect3d_relpointer_index_absmove(&idx, 1), ==,
                                1 * cdo.m));
  test_fail_if(!test_compare_ii(idx.nbpoints, ==, 6));
  test_fail_if(!test_compare_ii(idx.nfpoints, ==, 4));
  test_fail_if(!test_compare_ii(idx.voffset, ==, -cdo.m));

  test_fail_if(!test_compare_ii(rect3d_relpointer_index_nfwdpts(&idx), ==, 4));
  test_fail_if(!test_compare_ii(rect3d_relpointer_index_nbkwdpts(&idx), ==, 6));
  test_fail_if(!test_compare_ii(rect3d_relpointer_index_npospts(&idx), ==, 6));
  test_fail_if(!test_compare_ii(rect3d_relpointer_index_nnegpts(&idx), ==, 4));

  return r;
}
