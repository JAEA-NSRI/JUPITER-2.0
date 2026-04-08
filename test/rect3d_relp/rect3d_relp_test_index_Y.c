
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

int rect3d_index_Y_test(void)
{
  int r;
  domain cdo;
  mpi_param mpi;
  struct rect3d_relpointer_index idx;

  set_self_comm_mpi(&mpi);

  r = 0;

  if (!make_cdo_c(&mpi, &cdo, 12, 13, 14, 3, 4, 0.5, 0.5, 0.5))
    return 1;

  idx = rect3d_relpointer_index_Y(&cdo, 1, 0, 0, 10);

  test_fail_if(!test_compare_ii(idx.nbpoints, ==, 0));
  test_fail_if(!test_compare_ii(idx.nfpoints, ==, 10));
  test_fail_if(!test_compare_ii(idx.voffset, ==, cdo.mx));

  idx = rect3d_relpointer_index_Y(&cdo, 1, 3, 0, 10);

  test_fail_if(!test_compare_ii(idx.nbpoints, ==, 3));
  test_fail_if(!test_compare_ii(idx.nfpoints, ==, 7));
  test_fail_if(!test_compare_ii(idx.voffset, ==, cdo.mx));

  idx = rect3d_relpointer_index_Y(&cdo, -1, 10, 0, 25);

  test_fail_if(!test_compare_ii(idx.nbpoints, ==, 15));
  test_fail_if(!test_compare_ii(idx.nfpoints, ==, 10));
  test_fail_if(!test_compare_ii(idx.voffset, ==, -cdo.mx));

  idx = rect3d_relpointer_index_Ym(&cdo, 1, 0);

  test_fail_if(!test_compare_ii(idx.nbpoints, ==, 0));
  test_fail_if(!test_compare_ii(idx.nfpoints, ==, 12));
  test_fail_if(!test_compare_ii(idx.voffset, ==, cdo.mx));

  idx = rect3d_relpointer_index_Ym(&cdo, 1, 11);

  test_fail_if(!test_compare_ii(idx.nbpoints, ==, 11));
  test_fail_if(!test_compare_ii(idx.nfpoints, ==, 1));
  test_fail_if(!test_compare_ii(idx.voffset, ==, cdo.mx));

  idx = rect3d_relpointer_index_Ym(&cdo, -1, 0);

  test_fail_if(!test_compare_ii(idx.nbpoints, ==, 12));
  test_fail_if(!test_compare_ii(idx.nfpoints, ==, 0));
  test_fail_if(!test_compare_ii(idx.voffset, ==, -cdo.mx));

  idx = rect3d_relpointer_index_Ym(&cdo, -1, 11);

  test_fail_if(!test_compare_ii(idx.nbpoints, ==, 1));
  test_fail_if(!test_compare_ii(idx.nfpoints, ==, 11));
  test_fail_if(!test_compare_ii(idx.voffset, ==, -cdo.mx));

  idx = rect3d_relpointer_index_Yc(&cdo, 1, 3, 0, 0);

  test_fail_if(!test_compare_ii(idx.nbpoints, ==, 0));
  test_fail_if(!test_compare_ii(idx.nfpoints, ==, 5));
  test_fail_if(!test_compare_ii(idx.voffset, ==, cdo.mx));

  idx = rect3d_relpointer_index_Yc(&cdo, 1, 3, 1, 2);

  test_fail_if(!test_compare_ii(idx.nbpoints, ==, 1));
  test_fail_if(!test_compare_ii(idx.nfpoints, ==, 7));
  test_fail_if(!test_compare_ii(idx.voffset, ==, cdo.mx));

  idx = rect3d_relpointer_index_Yc(&cdo, 1, 5, -1, -1);

  test_fail_if(!test_compare_ii(idx.nbpoints, ==, 1));
  test_fail_if(!test_compare_ii(idx.nfpoints, ==, 2));
  test_fail_if(!test_compare_ii(idx.voffset, ==, cdo.mx));

  idx = rect3d_relpointer_index_Yv(&cdo, 1, 3, 0, 0);

  test_fail_if(!test_compare_ii(idx.nbpoints, ==, 0));
  test_fail_if(!test_compare_ii(idx.nfpoints, ==, 6));
  test_fail_if(!test_compare_ii(idx.voffset, ==, cdo.mx));

  idx = rect3d_relpointer_index_Yv(&cdo, 1, 3, 1, 2);

  test_fail_if(!test_compare_ii(idx.nbpoints, ==, 1));
  test_fail_if(!test_compare_ii(idx.nfpoints, ==, 8));
  test_fail_if(!test_compare_ii(idx.voffset, ==, cdo.mx));

  idx = rect3d_relpointer_index_Yv(&cdo, 1, 5, -1, -1);

  test_fail_if(!test_compare_ii(idx.nbpoints, ==, 1));
  test_fail_if(!test_compare_ii(idx.nfpoints, ==, 3));
  test_fail_if(!test_compare_ii(idx.voffset, ==, cdo.mx));

  return r;
}
