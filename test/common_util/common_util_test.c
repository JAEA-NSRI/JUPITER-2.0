#include "jupiter/os/os.h"
#include "test-util.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#ifdef _OPENMP
#include <omp.h>
#endif

#include <jupiter/common_util.h>

static int f22(ptrdiff_t jj, void *arg)
{
  if (jj == 22)
    return 1;
  return 0;
}

static int f22_3(ptrdiff_t jj, void *arg)
{
  if (jj == 22)
    return 1;
  if (jj == 44)
    return 1;
  if (jj == 319)
    return 1;
  return 0;
}

int main(int argc, char **argv)
{
  int r = 0;

  if (!test_compare_ii(calc_address(0, 0, 0, 10, 10, 10), ==, 0))
    r = 1;
  if (!test_compare_ii(calc_address(1, 0, 0, 10, 10, 10), ==, 1))
    r = 1;
  if (!test_compare_ii(calc_address(9, 0, 0, 10, 10, 10), ==, 9))
    r = 1;
  if (!test_compare_ii(calc_address(0, 1, 0, 10, 10, 10), ==, 10))
    r = 1;
  if (!test_compare_ii(calc_address(1, 1, 0, 10, 10, 10), ==, 11))
    r = 1;
  if (!test_compare_ii(calc_address(0, 2, 0, 10, 10, 10), ==, 20))
    r = 1;
  if (!test_compare_ii(calc_address(0, 9, 0, 10, 10, 10), ==, 90))
    r = 1;
  if (!test_compare_ii(calc_address(0, 0, 1, 10, 10, 10), ==, 100))
    r = 1;
  if (!test_compare_ii(calc_address(1, 0, 1, 10, 10, 10), ==, 101))
    r = 1;
  if (!test_compare_ii(calc_address(0, 1, 1, 10, 10, 10), ==, 110))
    r = 1;
  if (!test_compare_ii(calc_address(1, 1, 1, 10, 10, 10), ==, 111))
    r = 1;
  if (!test_compare_ii(calc_address(0, 0, 2, 10, 10, 10), ==, 200))
    r = 1;
  if (!test_compare_ii(calc_address(0, 0, 9, 10, 10, 10), ==, 900))
    r = 1;
  if (!test_compare_ii(calc_address(9, 9, 9, 10, 10, 10), ==, 999))
    r = 1;

  {
    int i, j, k;
    if (!test_compare_ii(calc_struct_index(0, 1, 1, 1, &i, &j, &k), ==, 0))
      r = 1;
    if (!test_compare_ii(calc_struct_index(-1, 1, 1, 1, &i, &j, &k), ==, 1))
      r = 1;
    if (!test_compare_ii(calc_struct_index(1, 1, 1, 1, &i, &j, &k), ==, 1))
      r = 1;

    if (!test_compare_ii(calc_struct_index(0, 10, 10, 10, &i, &j, &k), ==, 0))
      r = 1;
    if (!test_compare_ii(i, ==, 0))
      r = 1;
    if (!test_compare_ii(j, ==, 0))
      r = 1;
    if (!test_compare_ii(k, ==, 0))
      r = 1;

    if (!test_compare_ii(calc_struct_index(1, 10, 10, 10, &i, &j, &k), ==, 0))
      r = 1;
    if (!test_compare_ii(i, ==, 1))
      r = 1;
    if (!test_compare_ii(j, ==, 0))
      r = 1;
    if (!test_compare_ii(k, ==, 0))
      r = 1;

    if (!test_compare_ii(calc_struct_index(2, 10, 10, 10, &i, &j, &k), ==, 0))
      r = 1;
    if (!test_compare_ii(i, ==, 2))
      r = 1;
    if (!test_compare_ii(j, ==, 0))
      r = 1;
    if (!test_compare_ii(k, ==, 0))
      r = 1;

    if (!test_compare_ii(calc_struct_index(9, 10, 10, 10, &i, &j, &k), ==, 0))
      r = 1;
    if (!test_compare_ii(i, ==, 9))
      r = 1;
    if (!test_compare_ii(j, ==, 0))
      r = 1;
    if (!test_compare_ii(k, ==, 0))
      r = 1;

    if (!test_compare_ii(calc_struct_index(10, 10, 10, 10, &i, &j, &k), ==, 0))
      r = 1;
    if (!test_compare_ii(i, ==, 0))
      r = 1;
    if (!test_compare_ii(j, ==, 1))
      r = 1;
    if (!test_compare_ii(k, ==, 0))
      r = 1;

    if (!test_compare_ii(calc_struct_index(11, 10, 10, 10, &i, &j, &k), ==, 0))
      r = 1;
    if (!test_compare_ii(i, ==, 1))
      r = 1;
    if (!test_compare_ii(j, ==, 1))
      r = 1;
    if (!test_compare_ii(k, ==, 0))
      r = 1;

    if (!test_compare_ii(calc_struct_index(20, 10, 10, 10, &i, &j, &k), ==, 0))
      r = 1;
    if (!test_compare_ii(i, ==, 0))
      r = 1;
    if (!test_compare_ii(j, ==, 2))
      r = 1;
    if (!test_compare_ii(k, ==, 0))
      r = 1;

    if (!test_compare_ii(calc_struct_index(90, 10, 10, 10, &i, &j, &k), ==, 0))
      r = 1;
    if (!test_compare_ii(i, ==, 0))
      r = 1;
    if (!test_compare_ii(j, ==, 9))
      r = 1;
    if (!test_compare_ii(k, ==, 0))
      r = 1;

    if (!test_compare_ii(calc_struct_index(99, 10, 10, 10, &i, &j, &k), ==, 0))
      r = 1;
    if (!test_compare_ii(i, ==, 9))
      r = 1;
    if (!test_compare_ii(j, ==, 9))
      r = 1;
    if (!test_compare_ii(k, ==, 0))
      r = 1;

    if (!test_compare_ii(calc_struct_index(100, 10, 10, 10, &i, &j, &k), ==, 0))
      r = 1;
    if (!test_compare_ii(i, ==, 0))
      r = 1;
    if (!test_compare_ii(j, ==, 0))
      r = 1;
    if (!test_compare_ii(k, ==, 1))
      r = 1;

    if (!test_compare_ii(calc_struct_index(101, 10, 10, 10, &i, &j, &k), ==, 0))
      r = 1;
    if (!test_compare_ii(i, ==, 1))
      r = 1;
    if (!test_compare_ii(j, ==, 0))
      r = 1;
    if (!test_compare_ii(k, ==, 1))
      r = 1;

    if (!test_compare_ii(calc_struct_index(110, 10, 10, 10, &i, &j, &k), ==, 0))
      r = 1;
    if (!test_compare_ii(i, ==, 0))
      r = 1;
    if (!test_compare_ii(j, ==, 1))
      r = 1;
    if (!test_compare_ii(k, ==, 1))
      r = 1;

    if (!test_compare_ii(calc_struct_index(111, 10, 10, 10, &i, &j, &k), ==, 0))
      r = 1;
    if (!test_compare_ii(i, ==, 1))
      r = 1;
    if (!test_compare_ii(j, ==, 1))
      r = 1;
    if (!test_compare_ii(k, ==, 1))
      r = 1;

    if (!test_compare_ii(calc_struct_index(200, 10, 10, 10, &i, &j, &k), ==, 0))
      r = 1;
    if (!test_compare_ii(i, ==, 0))
      r = 1;
    if (!test_compare_ii(j, ==, 0))
      r = 1;
    if (!test_compare_ii(k, ==, 2))
      r = 1;

    if (!test_compare_ii(calc_struct_index(900, 10, 10, 10, &i, &j, &k), ==, 0))
      r = 1;
    if (!test_compare_ii(i, ==, 0))
      r = 1;
    if (!test_compare_ii(j, ==, 0))
      r = 1;
    if (!test_compare_ii(k, ==, 9))
      r = 1;

    if (!test_compare_ii(calc_struct_index(999, 10, 10, 10, &i, &j, &k), ==, 0))
      r = 1;
    if (!test_compare_ii(i, ==, 9))
      r = 1;
    if (!test_compare_ii(j, ==, 9))
      r = 1;
    if (!test_compare_ii(k, ==, 9))
      r = 1;

    if (!test_compare_ii(calc_struct_index(1000, 10, 10, 10, &i, &j, &k), ==,
                         1))
      r = 1;
    if (!test_compare_ii(i, ==, -1))
      r = 1;
    if (!test_compare_ii(j, ==, -1))
      r = 1;
    if (!test_compare_ii(k, ==, -1))
      r = 1;
  }

  {
    ptrdiff_t is, ie;

    distribute_thread_1di(0, 10000, 1, 0, &is, &ie);
    if (!test_compare_ii(is, ==, 0))
      r = 1;
    if (!test_compare_ii(ie, ==, 10000))
      r = 1;

    distribute_thread_1di(0, 10000, 4, 0, &is, &ie);
    if (!test_compare_ii(is, ==, 0))
      r = 1;
    if (!test_compare_ii(ie, ==, 2500))
      r = 1;

    distribute_thread_1di(0, 10000, 4, 1, &is, &ie);
    if (!test_compare_ii(is, ==, 2500))
      r = 1;
    if (!test_compare_ii(ie, ==, 5000))
      r = 1;

    distribute_thread_1di(0, 10000, 4, 2, &is, &ie);
    if (!test_compare_ii(is, ==, 5000))
      r = 1;
    if (!test_compare_ii(ie, ==, 7500))
      r = 1;

    distribute_thread_1di(0, 10000, 4, 3, &is, &ie);
    if (!test_compare_ii(is, ==, 7500))
      r = 1;
    if (!test_compare_ii(ie, ==, 10000))
      r = 1;

    distribute_thread_1di(0, 10000, 4, 4, &is, &ie);
    if (!test_compare_ii(is, ==, 0))
      r = 1;
    if (!test_compare_ii(ie, ==, 0))
      r = 1;

    distribute_thread_1di(0, 100, 7, 0, &is, &ie);
    if (!test_compare_ii(is, ==, 0))
      r = 1;
    if (!test_compare_ii(ie, ==, 14))
      r = 1;

    distribute_thread_1di(0, 100, 7, 1, &is, &ie);
    if (!test_compare_ii(is, ==, 14))
      r = 1;
    if (!test_compare_ii(ie, ==, 28))
      r = 1;

    distribute_thread_1di(0, 100, 7, 2, &is, &ie);
    if (!test_compare_ii(is, ==, 28))
      r = 1;
    if (!test_compare_ii(ie, ==, 42))
      r = 1;

    distribute_thread_1di(0, 100, 7, 3, &is, &ie);
    if (!test_compare_ii(is, ==, 42))
      r = 1;
    if (!test_compare_ii(ie, ==, 56))
      r = 1;

    distribute_thread_1di(0, 100, 7, 4, &is, &ie);
    if (!test_compare_ii(is, ==, 56))
      r = 1;
    if (!test_compare_ii(ie, ==, 70))
      r = 1;

    distribute_thread_1di(0, 100, 7, 5, &is, &ie);
    if (!test_compare_ii(is, ==, 70))
      r = 1;
    if (!test_compare_ii(ie, ==, 85))
      r = 1;

    distribute_thread_1di(0, 100, 7, 6, &is, &ie);
    if (!test_compare_ii(is, ==, 85))
      r = 1;
    if (!test_compare_ii(ie, ==, 100))
      r = 1;

    distribute_thread_1di(0, -1, 1, 0, &is, &ie);
    if (!test_compare_ii(is, ==, 0))
      r = 1;
    if (!test_compare_ii(ie, ==, 0))
      r = 1;

    distribute_thread_1di(0, -1, 4, 0, &is, &ie);
    if (!test_compare_ii(is, ==, 0))
      r = 1;
    if (!test_compare_ii(ie, ==, 0))
      r = 1;

    distribute_thread_1di(0, -1, 4, 1, &is, &ie);
    if (!test_compare_ii(is, ==, 0))
      r = 1;
    if (!test_compare_ii(ie, ==, 0))
      r = 1;

    distribute_thread_1di(0, -1, 4, 2, &is, &ie);
    if (!test_compare_ii(is, ==, 0))
      r = 1;
    if (!test_compare_ii(ie, ==, 0))
      r = 1;

    distribute_thread_1di(0, -1, 4, 3, &is, &ie);
    if (!test_compare_ii(is, ==, 0))
      r = 1;
    if (!test_compare_ii(ie, ==, 0))
      r = 1;
  }

  {
    int nb;
    ptrdiff_t nbs;

    distribute_thread_1db(100, 10, &nb, &nbs);
    if (!test_compare_ii(nb, ==, 10))
      r = 1;
    if (!test_compare_ii(nbs, ==, 10))
      r = 1;

    distribute_thread_1db(111, 11, &nb, &nbs);
    if (!test_compare_ii(nb, ==, 11))
      r = 1;
    if (!test_compare_ii(nbs, ==, 11))
      r = 1;

    distribute_thread_1db(100, 9, &nb, &nbs);
    if (!test_compare_ii(nb, ==, 12))
      r = 1;
    if (!test_compare_ii(nbs, ==, 9))
      r = 1;

    distribute_thread_1db(1000, 0, &nb, &nbs);
    if (!test_compare_ii(nb, ==, 8))
      r = 1;
    if (!test_compare_ii(nbs, ==, 128))
      r = 1;

    distribute_thread_1db(0, 9, &nb, &nbs);
    if (!test_compare_ii(nb, ==, 0))
      r = 1;
    if (!test_compare_ii(nbs, ==, 9))
      r = 1;

    distribute_thread_1db(1, 9, &nb, &nbs);
    if (!test_compare_ii(nb, ==, 1))
      r = 1;
    if (!test_compare_ii(nbs, ==, 9))
      r = 1;
  }

  {
    int shared_nb;

#ifdef _OPENMP
#pragma omp parallel num_threads(1)
#endif
    {
      int nb;
      nb = distribute_thread_max_nb(1, &shared_nb);

#ifdef _OPENMP
#pragma omp critical
#endif
      {
        if (!test_compare_ii(nb, ==, 1))
          r = 1;
      }

      nb = distribute_thread_max_nb(100, &shared_nb);

#ifdef _OPENMP
#pragma omp critical
#endif
      {
        if (!test_compare_ii(nb, ==, 100))
          r = 1;
      }
    }

#ifdef _OPENMP
#pragma omp parallel num_threads(4)
    {
      int nb;
      int ith = omp_get_thread_num();

      nb = distribute_thread_max_nb(ith, &shared_nb);
#pragma omp critical
      {
        if (!test_compare_ii(nb, ==, 3))
          r = 1;
      }

      nb = distribute_thread_max_nb(4 - ith, &shared_nb);
#pragma omp critical
      {
        if (!test_compare_ii(nb, ==, 4))
          r = 1;
      }

      switch (ith) {
      case 0:
        jupiter_sleep(100);
        nb = distribute_thread_max_nb(99, &shared_nb);
        break;
      case 1:
        jupiter_sleep(500);
        nb = distribute_thread_max_nb(11, &shared_nb);
        break;
      case 2:
        jupiter_sleep(1000);
        nb = distribute_thread_max_nb(100, &shared_nb);
        break;
      case 3:
      default:
        jupiter_sleep(10);
        nb = distribute_thread_max_nb(0, &shared_nb);
        break;
      }
#pragma omp critical
      {
        if (!test_compare_ii(nb, ==, 100))
          r = 1;
      }
    }
#endif
  }

#ifdef _OPENMP
#pragma omp parallel num_threads(1)
#endif
  {
    ptrdiff_t is, ie, nbs;
    int nth, ith, nb;

    is = -1;
    ie = -1;
    nth = -1;
    ith = -1;
    nbs = 0;

    distribute_thread_1d(0, 10000, &nth, &ith, &is, &ie, &nb, &nbs, NULL);
    if (!test_compare_ii(is, ==, 0))
      r = 1;
    if (!test_compare_ii(ie, ==, 10000))
      r = 1;
    if (!test_compare_ii(nth, ==, 1))
      r = 1;
    if (!test_compare_ii(ith, ==, 0))
      r = 1;
    if (!test_compare_ii(nb, ==, 1))
      r = 1;
    if (!test_compare_ii(nbs, ==, 10000))
      r = 1;

    is = -1;
    ie = -1;
    nth = -1;
    ith = -1;
    nbs = 9999;

    distribute_thread_1d(0, 1000, &nth, &ith, &is, &ie, &nb, &nbs, NULL);
    if (!test_compare_ii(is, ==, 0))
      r = 1;
    if (!test_compare_ii(ie, ==, 1000))
      r = 1;
    if (!test_compare_ii(nth, ==, 1))
      r = 1;
    if (!test_compare_ii(ith, ==, 0))
      r = 1;
    if (!test_compare_ii(nb, ==, 1))
      r = 1;
    if (!test_compare_ii(nbs, ==, 1000))
      r = 1;

    is = -1;
    ie = -1;

    distribute_thread_1d(0, 1000, NULL, NULL, &is, &ie, NULL, NULL, NULL);
    if (!test_compare_ii(is, ==, 0))
      r = 1;
    if (!test_compare_ii(ie, ==, 1000))
      r = 1;
  }

#ifdef _OPENMP
  {
    int shared_nb;

#pragma omp parallel num_threads(4)
    {
      ptrdiff_t is, ie, nbs;
      int nth, ith, nb;

      is = -1;
      ie = -1;
      ith = -1;
      nth = -1;
      nbs = 0;

      distribute_thread_1d(0, 10000, &nth, &ith, &is, &ie, &nb, &nbs, NULL);
#pragma omp critical
      {
        fprintf(stderr, "..... Thread %d\n", omp_get_thread_num());
        if (!test_compare_ii(ith, >=, 0))
          r = 1;
        if (!test_compare_ii(ith, <, 4))
          r = 1;
        if (!test_compare_ii(nth, ==, 4))
          r = 1;
        switch (ith) {
        case 0:
          if (!test_compare_ii(is, ==, 0))
            r = 1;
          if (!test_compare_ii(ie, ==, 2500))
            r = 1;
          if (!test_compare_ii(nb, ==, 20))
            r = 1;
          if (!test_compare_ii(nbs, ==, 128))
            r = 1;
          break;
        case 1:
          if (!test_compare_ii(is, ==, 2500))
            r = 1;
          if (!test_compare_ii(ie, ==, 5000))
            r = 1;
          if (!test_compare_ii(nb, ==, 20))
            r = 1;
          if (!test_compare_ii(nbs, ==, 128))
            r = 1;
          break;
        case 2:
          if (!test_compare_ii(is, ==, 5000))
            r = 1;
          if (!test_compare_ii(ie, ==, 7500))
            r = 1;
          if (!test_compare_ii(nb, ==, 20))
            r = 1;
          if (!test_compare_ii(nbs, ==, 128))
            r = 1;
          break;
        case 3:
          if (!test_compare_ii(is, ==, 7500))
            r = 1;
          if (!test_compare_ii(ie, ==, 10000))
            r = 1;
          if (!test_compare_ii(nb, ==, 20))
            r = 1;
          if (!test_compare_ii(nbs, ==, 128))
            r = 1;
          break;
        default:
          r = 1;
        }
      }

#pragma omp barrier

      is = -1;
      ie = -1;
      ith = -1;
      nth = -1;
      nbs = 0;
      if (omp_get_thread_num() == 0)
        nbs = 12;

      distribute_thread_1d(0, 713, &nth, &ith, &is, &ie, &nb, &nbs, NULL);
#pragma omp critical
      {
        fprintf(stderr, "..... Thread %d\n", omp_get_thread_num());
        if (!test_compare_ii(ith, >=, 0))
          r = 1;
        if (!test_compare_ii(ith, <, 4))
          r = 1;
        if (!test_compare_ii(nth, ==, 4))
          r = 1;
        switch (ith) {
        case 0:
          if (!test_compare_ii(is, ==, 0))
            r = 1;
          if (!test_compare_ii(ie, ==, 178))
            r = 1;
          if (!test_compare_ii(nb, ==, 15))
            r = 1;
          if (!test_compare_ii(nbs, ==, 12))
            r = 1;
          break;
        case 1:
          if (!test_compare_ii(is, ==, 178))
            r = 1;
          if (!test_compare_ii(ie, ==, 356))
            r = 1;
          if (!test_compare_ii(nb, ==, 2))
            r = 1;
          if (!test_compare_ii(nbs, ==, 128))
            r = 1;
          break;
        case 2:
          if (!test_compare_ii(is, ==, 356))
            r = 1;
          if (!test_compare_ii(ie, ==, 534))
            r = 1;
          if (!test_compare_ii(nb, ==, 2))
            r = 1;
          if (!test_compare_ii(nbs, ==, 128))
            r = 1;
          break;
        case 3:
          if (!test_compare_ii(is, ==, 534))
            r = 1;
          if (!test_compare_ii(ie, ==, 713))
            r = 1;
          if (!test_compare_ii(nb, ==, 2))
            r = 1;
          if (!test_compare_ii(nbs, ==, 128))
            r = 1;
          break;
        default:
          r = 1;
        }
      }
    }
  }
#endif

#ifdef _OPENMP
#pragma omp parallel num_threads(4) if (0)
  {
    ptrdiff_t is, ie, nbs;
    int nth, ith, nb;

    is = -1;
    ie = -1;
    nth = -1;
    ith = -1;
    nbs = 0;

    distribute_thread_1d(0, 10000, &nth, &ith, &is, &ie, &nb, &nbs, NULL);
    if (!test_compare_ii(is, ==, 0))
      r = 1;
    if (!test_compare_ii(ie, ==, 10000))
      r = 1;
    if (!test_compare_ii(nth, ==, 1))
      r = 1;
    if (!test_compare_ii(ith, ==, 0))
      r = 1;
    if (!test_compare_ii(nb, ==, 1))
      r = 1;
    if (!test_compare_ii(nbs, ==, 10000))
      r = 1;

    is = -1;
    ie = -1;
    nth = -1;
    ith = -1;
    nbs = 9999;

    distribute_thread_1d(0, 1000, &nth, &ith, &is, &ie, &nb, &nbs, NULL);
    if (!test_compare_ii(is, ==, 0))
      r = 1;
    if (!test_compare_ii(ie, ==, 1000))
      r = 1;
    if (!test_compare_ii(nth, ==, 1))
      r = 1;
    if (!test_compare_ii(ith, ==, 0))
      r = 1;
    if (!test_compare_ii(nb, ==, 1))
      r = 1;
    if (!test_compare_ii(nbs, ==, 1000))
      r = 1;
  }
#endif

  if (!test_compare_ii(struct_domain_find_if(10, 10, 10, 0, 0, 0, 0, 0, 0, f22,
                                             NULL),
                       ==, 22))
    r = 1;

  if (!test_compare_ii(struct_domain_find_if(1, 1, 1, 0, 0, 0, 0, 0, 0, f22,
                                             NULL),
                       ==, -1))
    r = 1;

  if (!test_compare_ii(struct_domain_find_if(0, 0, 0, 0, 0, 0, 0, 0, 0, f22,
                                             NULL),
                       ==, -1))
    r = 1;

  if (!test_compare_ii(struct_domain_find_if(0, 0, 0, 1, 2, 3, 4, 5, 6, f22,
                                             NULL),
                       ==, -1))
    r = 1;

  /* jj == 22 is in stencil */
  if (!test_compare_ii(struct_domain_find_if(10, 10, 10, 2, 2, 2, 2, 2, 2, f22,
                                             NULL),
                       ==, -1))
    r = 1;

  if (!test_compare_ii(struct_domain_find_n_if(10, 10, 10, 0, 0, 0, 0, 0, 0, 2,
                                               f22_3, NULL),
                       ==, 2))
    r = 1;

  if (!test_compare_ii(struct_domain_find_n_if(10, 10, 10, 0, 0, 0, 0, 0, 0, 99,
                                               f22_3, NULL),
                       ==, 3))
    r = 1;

  if (!test_compare_ii(struct_domain_find_n_if(10, 10, 1, 0, 0, 0, 0, 0, 0, 99,
                                               f22_3, NULL),
                       ==, 2))
    r = 1;

  if (!test_compare_ii(struct_domain_find_n_if(10, 10, 10, 0, 0, 0, 0, 0, 0, 0,
                                               f22_3, NULL),
                       ==, 0))
    r = 1;

  /* all indices are in stencil */
  if (!test_compare_ii(struct_domain_find_n_if(10, 10, 10, 0, 0, 4, 0, 0, 0, 99,
                                               f22_3, NULL),
                       ==, 0))
    r = 1;

  return r;
}
