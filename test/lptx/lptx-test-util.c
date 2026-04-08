
#include "jupiter/lptx/defs.h"
#include "jupiter/lptx/util.h"
#include "jupiter/lptx/vector.h"
#include "lptx-test.h"
#include "test-util.h"

#ifdef _OPENMP
#include <omp.h>
#endif

int test_lptx_util(void)
{
  LPTX_vector v;
  LPTX_idtype is, ie;
  int r = 0;

  /* Test for test utility */
  v = LPTX_vector_c(0.0, 0.0, 0.0);
  if (!test_compare_ii(test_compare_lptxvecl(v, ==, (0., 0., 0.)), ==, 1))
    r = 1;
  if (!test_compare_ii(test_compare_lptxvecl(v, ==, (1., 0., 0.)), ==, 0))
    r = 1;
  if (!test_compare_ii(test_compare_lptxvecl(v, ==, (0., 1., 0.)), ==, 0))
    r = 1;
  if (!test_compare_ii(test_compare_lptxvecl(v, ==, (0., 0., 1.)), ==, 0))
    r = 1;

  if (!test_compare_ii(test_compare_lptxvecl(v, !=, (0., 0., 0.)), ==, 0))
    r = 1;
  if (!test_compare_ii(test_compare_lptxvecl(v, !=, (1., 0., 0.)), ==, 0))
    r = 1;
  if (!test_compare_ii(test_compare_lptxvecl(v, !=, (0., 1., 0.)), ==, 0))
    r = 1;
  if (!test_compare_ii(test_compare_lptxvecl(v, !=, (0., 0., 1.)), ==, 0))
    r = 1;
  if (!test_compare_ii(test_compare_lptxvecl(v, !=, (1., 1., 0.)), ==, 0))
    r = 1;
  if (!test_compare_ii(test_compare_lptxvecl(v, !=, (0., 1., 1.)), ==, 0))
    r = 1;
  if (!test_compare_ii(test_compare_lptxvecl(v, !=, (1., 0., 1.)), ==, 0))
    r = 1;
  if (!test_compare_ii(test_compare_lptxvecl(v, !=, (1., 1., 1.)), ==, 1))
    r = 1;

  v = LPTX_vector_c(0.0, 0.0, 0.0);
  if (!test_compare_ii(test_compare_lptxvecl(v, <=, (0., 0., 0.)), ==, 1))
    r = 1;
  if (!test_compare_ii(test_compare_lptxvecl(v, <=, (-1., 0., 0.)), ==, 0))
    r = 1;
  if (!test_compare_ii(test_compare_lptxvecl(v, <=, (0., -1., 0.)), ==, 0))
    r = 1;
  if (!test_compare_ii(test_compare_lptxvecl(v, <=, (0., 0., -1.)), ==, 0))
    r = 1;

  v = LPTX_vector_c(0.0, 0.0, 0.0);
  if (!test_compare_ii(test_compare_lptxvecl(v, >=, (0., 0., 1.)), ==, 0))
    r = 1;
  if (!test_compare_ii(test_compare_lptxvecl(v, >=, (1., 0., 0.)), ==, 0))
    r = 1;
  if (!test_compare_ii(test_compare_lptxvecl(v, >=, (0., 1., 0.)), ==, 0))
    r = 1;
  if (!test_compare_ii(test_compare_lptxvecl(v, >=, (0., 0., 0.)), ==, 1))
    r = 1;
  /* */

  LPTX_omp_distribute_calc(&is, &ie, 1, 0, 0, 100);
  if (!test_compare_ii(is, ==, 0))
    r = 1;
  if (!test_compare_ii(ie, ==, 100))
    r = 1;

  LPTX_omp_distribute_calc(&is, &ie, 100, 0, 0, 6);
  if (!test_compare_ii(is, ==, 0))
    r = 1;
  if (!test_compare_ii(ie, ==, 1))
    r = 1;

  LPTX_omp_distribute_calc(&is, &ie, 100, 5, 0, 6);
  if (!test_compare_ii(is, ==, 5))
    r = 1;
  if (!test_compare_ii(ie, ==, 6))
    r = 1;

  LPTX_omp_distribute_calc(&is, &ie, 100, 99, 0, 6);
  if (!test_compare_ii(is, ==, 6))
    r = 1;
  if (!test_compare_ii(ie, ==, 6))
    r = 1;

  LPTX_omp_distribute_calc(&is, &ie, 7, 0, 0, 100);
  if (!test_compare_ii(is, ==, 0))
    r = 1;
  if (!test_compare_ii(ie, ==, 15))
    r = 1;

  LPTX_omp_distribute_calc(&is, &ie, 7, 1, 0, 100);
  if (!test_compare_ii(is, ==, 15))
    r = 1;
  if (!test_compare_ii(ie, ==, 30))
    r = 1;

  LPTX_omp_distribute_calc(&is, &ie, 7, 2, 0, 100);
  if (!test_compare_ii(is, ==, 30))
    r = 1;
  if (!test_compare_ii(ie, ==, 44))
    r = 1;

  LPTX_omp_distribute_calc(&is, &ie, 7, 3, 0, 100);
  if (!test_compare_ii(is, ==, 44))
    r = 1;
  if (!test_compare_ii(ie, ==, 58))
    r = 1;

  LPTX_omp_distribute_calc(&is, &ie, 7, 4, 0, 100);
  if (!test_compare_ii(is, ==, 58))
    r = 1;
  if (!test_compare_ii(ie, ==, 72))
    r = 1;

  LPTX_omp_distribute_calc(&is, &ie, 7, 5, 0, 100);
  if (!test_compare_ii(is, ==, 72))
    r = 1;
  if (!test_compare_ii(ie, ==, 86))
    r = 1;

  LPTX_omp_distribute_calc(&is, &ie, 7, 6, 0, 100);
  if (!test_compare_ii(is, ==, 86))
    r = 1;
  if (!test_compare_ii(ie, ==, 100))
    r = 1;

  LPTX_omp_distribute_calc(&is, &ie, 2, 0, 10, 20);
  if (!test_compare_ii(is, ==, 10))
    r = 1;
  if (!test_compare_ii(ie, ==, 15))
    r = 1;

  LPTX_omp_distribute_calc(&is, &ie, 2, 1, 10, 20);
  if (!test_compare_ii(is, ==, 15))
    r = 1;
  if (!test_compare_ii(ie, ==, 20))
    r = 1;

  LPTX_omp_distribute_calc(&is, &ie, 2, 0, 10, 11);
  if (!test_compare_ii(is, ==, 10))
    r = 1;
  if (!test_compare_ii(ie, ==, 11))
    r = 1;

  LPTX_omp_distribute_calc(&is, &ie, 2, 1, 10, 11);
  if (!test_compare_ii(is, ==, 11))
    r = 1;
  if (!test_compare_ii(ie, ==, 11))
    r = 1;

#ifdef _OPENMP
#pragma omp parallel num_threads(2)
#endif
  {
    int lr = 0;
    int nt, it;
    LPTX_idtype is, ie;
    LPTX_omp_distribute(&is, &ie, &nt, &it, 0, 100);

#ifdef _OPENMP
    if (!test_compare_ii(nt, ==, 2))
      lr = 1;
    if (!test_compare_ii(it, >=, 0))
      lr = 1;
    if (!test_compare_ii(it, <, 2))
      lr = 1;
#else
    if (!test_compare_ii(nt, ==, 1))
      lr = 1;
    if (!test_compare_ii(it, ==, 0))
      lr = 1;
#endif

#ifdef _OPENMP
    switch (it) {
    case 0:
      if (!test_compare_ii(is, ==, 0))
        lr = 1;
      if (!test_compare_ii(ie, ==, 50))
        lr = 1;
      break;
    case 1:
      if (!test_compare_ii(is, ==, 50))
        lr = 1;
      if (!test_compare_ii(ie, ==, 100))
        lr = 1;
      break;
    }
#else
    if (!test_compare_ii(is, ==, 0))
      lr = 1;
    if (!test_compare_ii(ie, ==, 100))
      lr = 1;
#endif

    if (lr) {
#ifdef _OPENMP
#pragma omp atomic write
#endif
      r = lr;
    }
  }

  return r;
}
