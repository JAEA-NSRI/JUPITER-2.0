
#include <stdlib.h>

#include "table-func.h"
#include "linear.h"

/* See linear-save.c for using OpenMP */

struct table_search_LINEAR_data
{
  /*
   * These parameters are not given on calling find,
   * So we need to keep.
   */
  table_geometry geom;
  table_size nx;
  table_size ny;
  const double *x;
  const double *y;
};
typedef struct table_search_LINEAR_data table_search_LINEAR_data;

/*
 * Definition of functions should be static.
 * (table library accesses them via a structure defined on the end)
 */
static void *table_search_LINEAR_alloc(void)
{
  return calloc(sizeof(table_search_LINEAR_data), 1);
}

static void table_search_LINEAR_dealloc(void *d)
{
  free(d);
}

static table_error table_search_LINEAR_init(void *d, table_geometry g,
                                            table_size nx, table_size ny,
                                            const double *x, const double *y)
{
  table_search_LINEAR_data *dd;

  /*
   * FYI: Cast is not required in C. Just be explanatory.
   */
  dd = (table_search_LINEAR_data *)d;

  /*
   * Pointer is guaranteed to be available.
   */
  TABLE_ASSERT(dd);

  /*
   * If you are not going to modify the array, you need not copy them
   * (but required if you are, because the original array is required
   * when changing algorithm dynamically).
   */
  dd->geom = g;
  dd->nx = nx;
  dd->ny = ny;
  dd->x = x;
  dd->y = y;

  /* If successfully initialized, return TABLE_SUCCESS */
  return TABLE_SUCCESS;
}

static table_error
table_search_LINEAR_core(double find_for, const double *xa, table_size n,
                         table_index *np)
{
  table_index i;
  TABLE_ASSERT(np);

  if (find_for < xa[0] || n <= 1) {
    i = 0;
  } else {
    for (i = 0; i < n; ++i) {
      if (find_for < xa[i]) {
        --i;
        break;
      }
    }
    if (i == n && find_for == xa[i - 1]) {
      i -= 2;  /* To return index for lower boundary */
    }
  }
  *np = i;
  if (n <= 1 || (i >= 0 && i < n - 1)) {
    return TABLE_SUCCESS;
  } else {
    return TABLE_ERR_TABLE_RANGE;
  }
}

static table_error
table_search_LINEAR_find(void *d, double x, double y, table_node *node)
{
  table_index ix;
  table_index iy;
  table_error ex;
  table_error ey;

  table_search_LINEAR_data *dd;
  dd = (table_search_LINEAR_data *)d;

  TABLE_ASSERT(dd);
  TABLE_ASSERT(node);

  ex = table_search_LINEAR_core(x, dd->x, dd->nx, &ix);
  ey = table_search_LINEAR_core(y, dd->y, dd->ny, &iy);

  /* Should be set even if out of range */
  node->x_index = ix;
  node->y_index = iy;

  if (ex == TABLE_SUCCESS && ey == TABLE_SUCCESS) {
    return TABLE_SUCCESS;
  } else {
    return TABLE_ERR_TABLE_RANGE;
  }
}

/*
 * Assign each functions to the each member of table_search_functions
 */
const table_search_functions table_search_func_LINEAR = {
  .allocate   = table_search_LINEAR_alloc,
  .deallocate = table_search_LINEAR_dealloc,
  .init       = table_search_LINEAR_init,
  .find       = table_search_LINEAR_find,
};
