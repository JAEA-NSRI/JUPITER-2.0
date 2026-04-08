
#include <stdlib.h>
#include <string.h>
#ifdef TABLE_PRINT_TREE
#include <stdio.h>
#endif

#include "table-func.h"
#include "bin-tree.h"
#include "tree.h"

struct table_tree_node
{
  table_tree tree;
  double min;
  double max;
  table_index i;
};
#define table_node_for_tree(ptr) \
  table_tree_container_of(ptr, struct table_tree_node, tree)

struct table_search_BIN_TREE_MINMAX_data
{
  table_geometry geom;
  table_size nx;
  table_size ny;
  struct table_tree_node *x;
  struct table_tree_node *y;
};
typedef struct table_search_BIN_TREE_MINMAX_data table_search_BIN_TREE_MINMAX_data;

/*
 * Definition of functions should be static.
 * (table library accesses them via a structure defined on the end)
 */
static void *table_search_BIN_TREE_MINMAX_alloc(void)
{
  return calloc(sizeof(table_search_BIN_TREE_MINMAX_data), 1);
}

static void table_search_BIN_TREE_MINMAX_dealloc(void *d)
{
  if (d) {
    table_search_BIN_TREE_MINMAX_data *p;
    p = (table_search_BIN_TREE_MINMAX_data *)d;
    free(p->x);
    free(p->y);
  }
  free(d);
}

/* max is inclusive */
static void
table_search_BIN_TREE_MINMAX_split(const double *inp,
                                   struct table_tree_node *parent,
                                   struct table_tree_node **free_loc,
                                   table_index min, table_index max)
{
  table_index mid;
  struct table_tree_node *ln;
  struct table_tree_node *rn;

  if (min == max) return;

  mid = min + (max - min) / 2;

  ln = NULL;
  rn = NULL;

  parent->min = inp[mid];
  parent->max = inp[mid + 1];
  parent->i = mid;

  if (mid - min > 0) {
    ln = *free_loc;
    table_tree_init(&ln->tree);
    table_tree_chain_left(&parent->tree, &ln->tree);
    ++(*free_loc);
  }
  if (max - mid > 1) {
    rn = *free_loc;
    table_tree_init(&rn->tree);
    table_tree_chain_right(&parent->tree, &rn->tree);
    ++(*free_loc);
  }
  if (ln) {
    table_search_BIN_TREE_MINMAX_split(inp, ln, free_loc, min, mid);
  }
  if (rn) {
    table_search_BIN_TREE_MINMAX_split(inp, rn, free_loc, mid + 1, max);
  }

#ifdef TABLE_PRINT_TREE
  fprintf(stderr, "%5zu %5zu %5zu %.6f %.6f %p %p %p\n", min, max, mid,
          parent->min, parent->max, parent, ln, rn);
#endif
}

static table_error
table_search_BIN_TREE_MINMAX_init(void *d, table_geometry g,
                                  table_size nx, table_size ny,
                                  const double *x, const double *y)
{
  table_search_BIN_TREE_MINMAX_data *dd;
  struct table_tree_node *t;

  dd = (table_search_BIN_TREE_MINMAX_data *)d;

  TABLE_ASSERT(dd);

  dd->geom = g;
  dd->nx = nx;
  dd->ny = ny;

  dd->x = (struct table_tree_node*)malloc(sizeof(struct table_tree_node) * nx);
  dd->y = (struct table_tree_node*)malloc(sizeof(struct table_tree_node) * ny);
  if (!dd->x || !dd->y) {
    free(dd->x);
    free(dd->y);
    dd->x = NULL;
    dd->y = NULL;
    /* Return TABLE_ERR_NOMEM for allocation error */
    return TABLE_ERR_NOMEM;
  }

  table_tree_init(&dd->x[0].tree);
  t = &dd->x[1];
  table_search_BIN_TREE_MINMAX_split(x, &dd->x[0], &t, 0, nx - 1);

  table_tree_init(&dd->y[0].tree);
  t = &dd->y[1];
  table_search_BIN_TREE_MINMAX_split(y, &dd->y[0], &t, 0, ny - 1);

  return TABLE_SUCCESS;
}

static table_error
table_search_BIN_TREE_MINMAX_core(double find_for, table_size n,
                                  struct table_tree_node *node,
                                  table_index *np)
{
  table_index i;
  struct table_tree *ns, *nss;

  TABLE_ASSERT(np);

  if (n <= 1) {
    *np = 0;
    return TABLE_SUCCESS;
  }

  ns = &node->tree;
  nss = ns;

  while (ns) {
    nss = ns;
    node = table_node_for_tree(ns);
#ifdef TABLE_PRINT_TREE
    fprintf(stderr, "look: %.6f node: %.6f %.6f %6zu\n",
            find_for, node->min, node->max, node->i);
#endif
    if (find_for < node->min) {
      ns = table_tree_left(ns);
    } else if (find_for >= node->max) {
      ns = table_tree_right(ns);
    } else {
      node = table_node_for_tree(ns);
      i = node->i;
      break;
    }
  }
  if (!ns) {
    node = table_node_for_tree(nss);
    if (find_for < node->min) {
      i = node->i;
    }
    if (find_for >= node->max) {
      i = node->i;
    }
  }

  *np = i;
  if (i >= 0 && i < n - 1) {
    return TABLE_SUCCESS;
  } else {
    return TABLE_ERR_TABLE_RANGE;
  }
}

static table_error
table_search_BIN_TREE_MINMAX_find(void *d, double x, double y, table_node *node)
{
  table_index ix;
  table_index iy;
  table_error ex;
  table_error ey;

  table_search_BIN_TREE_MINMAX_data *dd;
  dd = (table_search_BIN_TREE_MINMAX_data *)d;

  TABLE_ASSERT(dd);
  TABLE_ASSERT(node);

  ex = table_search_BIN_TREE_MINMAX_core(x, dd->nx, dd->x, &ix);
  ey = table_search_BIN_TREE_MINMAX_core(y, dd->ny, dd->y, &iy);

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
const table_search_functions table_search_func_BIN_TREE_MINMAX = {
  .allocate   = table_search_BIN_TREE_MINMAX_alloc,
  .deallocate = table_search_BIN_TREE_MINMAX_dealloc,
  .init       = table_search_BIN_TREE_MINMAX_init,
  .find       = table_search_BIN_TREE_MINMAX_find,
};
