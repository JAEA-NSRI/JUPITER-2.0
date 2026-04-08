
#include <stdlib.h>

/*
 * If you want to use OpenMP, put all omp related facilities or calls
 * wrap with this macro.
 */
#ifdef TABLE_USE_OPENMP
#include <omp.h>
#endif

#include "table-func.h"
#include "linear-save.h"

/*
 * Values stored in the strcture allocated by alloc function and
 * arrays given on init function are **shared** among OpenMP threads,
 * other variables will be all thread **private**
 */

struct table_search_LINEAR_SAVE_data
{
  table_geometry geom;
  table_size nx;
  table_size ny;
  const double *x;
  const double *y;
#ifdef TABLE_LINEAR_SAVE_TABLE_PRIVATE
  table_node *lastp;
  size_t szlastp;
#endif
};
typedef struct table_search_LINEAR_SAVE_data table_search_LINEAR_SAVE_data;

static void *table_search_LINEAR_SAVE_alloc(void)
{
  return calloc(sizeof(table_search_LINEAR_SAVE_data), 1);
}

static void table_search_LINEAR_SAVE_dealloc(void *d)
{
  table_search_LINEAR_SAVE_data *dd;
  dd = (table_search_LINEAR_SAVE_data *)d;

  if (dd) {
#ifdef TABLE_LINEAR_SAVE_TABLE_PRIVATE
    free(dd->lastp);
#endif
  }

  free(d);
}

static table_error
table_search_LINEAR_SAVE_init(void *d, table_geometry g,
                              table_size nx, table_size ny,
                              const double *x, const double *y)
{
  int tmax;
  table_search_LINEAR_SAVE_data *dd;
  dd = (table_search_LINEAR_SAVE_data *)d;

  TABLE_ASSERT(dd);

  dd->geom = g;
  dd->nx = nx;
  dd->ny = ny;
  dd->x = x;
  dd->y = y;

#ifdef TABLE_LINEAR_SAVE_TABLE_PRIVATE
  tmax = 1;
#ifdef TABLE_USE_OPENMP
  /* omp_get_num_threads() should be 1 here. See below. */
  tmax = omp_get_max_threads();
#endif

  dd->lastp = (table_node *)calloc(sizeof(table_node), tmax);
  if (dd->lastp) {
    dd->szlastp = tmax;
  } else {
    /* If not allocated here, disable allocation while finding */
    dd->szlastp = (size_t)-1;
  }
#endif

  return TABLE_SUCCESS;
}

static table_error
table_search_LINEAR_SAVE_core(double find_for, const double *xa, table_size n,
                              table_index *np)
{
  table_index i;

  TABLE_ASSERT(np);
  TABLE_ASSERT(xa);

  i = *np;
  if (n > 1) {
    if (i < 0)      i = 0;
    if (i >= n - 1) i = n - 2;
    if (find_for < xa[i]) {
      for (; i >= 0; --i) {
        if (find_for >= xa[i]) {
          break;
        }
      }
    } else if (i < n - 1 && find_for >= xa[i]) {
      for (; i < n; ++i) {
        if (find_for < xa[i]) {
          --i;
          break;
        }
      }
      if (i == n && find_for == xa[i - 1]) {
        i -= 2;
      }
    }
  } else {
    i = 0;
  }
  *np = i;
  if (n <= 1 || (i >= 0 && i < n - 1)) {
    return TABLE_SUCCESS;
  } else {
    return TABLE_ERR_TABLE_RANGE;
  }
}

static table_error table_search_LINEAR_SAVE_find(void *d, double x, double y,
                                                 table_node *node)
{
  int nthreads, ntmax;
  int tid;

  table_index ix;
  table_index iy;
  table_error ex;
  table_error ey;

  table_search_LINEAR_SAVE_data *dd;

  /*
   * If the compiler supports OpenMP 3.1 (a well-known compiler which
   * supports OpenMP but not 3.1 is MSVC), you can apply
   * `threadprivate` property to static variable.
   *
   * Using `threadprivate` is only local to the thread. i.e., it will
   * be shared among the tables (if the OpenMP internal does not join
   * the thread among parallel region. Or, using two or more tables
   * within one parallel region).
   *
   * To implement with older version of OpenMP, or, make sure to
   * private to a table, get number of threads and running thread
   * index, and then we need to dynamically allocate thread-local
   * variable.
   */
#ifndef TABLE_LINEAR_SAVE_TABLE_PRIVATE
  static table_node saved_node = { .x_index = 0, .y_index = 0 };
#ifdef TABLE_USE_OPENMP
#pragma omp threadprivate(saved_node)
#endif
#endif

  dd = (table_search_LINEAR_SAVE_data *)d;

  TABLE_ASSERT(dd);
  TABLE_ASSERT(node);

#ifdef TABLE_LINEAR_SAVE_TABLE_PRIVATE
#ifdef TABLE_USE_OPENMP
  nthreads = omp_get_num_threads();
  ntmax = omp_get_max_threads();
  if (nthreads < ntmax) {
    nthreads = ntmax;
  }
  tid = omp_get_thread_num();
#else
  ntmax = 1;
  nthreads = 1;
  tid = 0;
#endif
#endif

  ix = 0;
  iy = 0;
#ifdef TABLE_LINEAR_SAVE_TABLE_PRIVATE
  if (dd->szlastp != (size_t) -1) {
#ifdef TABLE_USE_OPENMP
    /*
     * Only one thread access to the lastp array.
     *
     * (access_lastp) is name for region,
     * and only one of these regions will run at same time.
     *
     * Another name of critical region may run at same time.
     */
#pragma omp critical (access_lastp)
#endif
    {
      /*
       * You should not return allocation error so easily.
       * Here, we can fallback to LINEAR algorithm.
       */
      if (!dd->lastp) {
        dd->lastp = (table_node *)calloc(sizeof(table_node), nthreads);
        if (dd->lastp) {
          dd->szlastp = nthreads;
        } else {
          dd->szlastp = 0;
        }
      } else if (dd->szlastp < nthreads) {
        table_node *t;
        size_t is;

        /* Using realloc to keep previous content */
        t = (table_node *)realloc(dd->lastp, sizeof(table_node) * nthreads);
        is = dd->szlastp;
        if (t) {
          /* Initialize extended region */
          for (; is < nthreads; ++is) {
            dd->lastp[is].x_index = 0;
            dd->lastp[is].y_index = 0;
          }
          dd->szlastp = is;
          dd->lastp = t;
        }
      }

      /*
       * If realloc is used and allocation is failed,
       * szlastp is might be less than thread number.
       */
      if (dd->lastp && tid < dd->szlastp) {
        ix = dd->lastp[tid].x_index;
        iy = dd->lastp[tid].y_index;
      }
    }
  }
#else
  ix = saved_node.x_index;
  iy = saved_node.y_index;
#endif

  ex = table_search_LINEAR_SAVE_core(x, dd->x, dd->nx, &ix);
  ey = table_search_LINEAR_SAVE_core(y, dd->y, dd->ny, &iy);

  node->x_index = ix;
  node->y_index = iy;

#ifdef TABLE_LINEAR_SAVE_TABLE_PRIVATE
  if (dd->lastp && tid < dd->szlastp) {
#ifdef TABLE_USE_OPENMP
#pragma omp critical (access_lastp)
#endif
    {
      /*
       * szlastp might be change between two checks.
       */
      if (dd->lastp && tid < dd->szlastp) {
        dd->lastp[tid].x_index = ix;
        dd->lastp[tid].y_index = iy;
      }
    }
  }
#else
  saved_node.x_index = ix;
  saved_node.y_index = iy;
#endif

  if (ex == TABLE_SUCCESS && ey == TABLE_SUCCESS) {
    return TABLE_SUCCESS;
  } else {
    return TABLE_ERR_TABLE_RANGE;
  }
}

const table_search_functions table_search_func_LINEAR_SAVE = {
  .allocate   = table_search_LINEAR_SAVE_alloc,
  .deallocate = table_search_LINEAR_SAVE_dealloc,
  .init       = table_search_LINEAR_SAVE_init,
  .find       = table_search_LINEAR_SAVE_find,
};
