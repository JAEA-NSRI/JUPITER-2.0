
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
  double v;
  table_index i;
};
#define table_node_for_tree(ptr) \
  table_tree_container_of(ptr, struct table_tree_node, tree)

struct table_search_BIN_TREE_MAX_data
{
  table_geometry geom;
  table_size nx;
  table_size ny;
  struct table_tree_node *x;
  struct table_tree_node *y;
};
typedef struct table_search_BIN_TREE_MAX_data table_search_BIN_TREE_MAX_data;

/*
 * Definition of functions should be static.
 * (table library accesses them via a structure defined on the end)
 */
static void *table_search_BIN_TREE_MAX_alloc(void)
{
  return calloc(sizeof(table_search_BIN_TREE_MAX_data), 1);
}

static void table_search_BIN_TREE_MAX_dealloc(void *d)
{
  if (d) {
    table_search_BIN_TREE_MAX_data *p;
    p = (table_search_BIN_TREE_MAX_data *)d;
    free(p->x);
    free(p->y);
  }
  free(d);
}

static void
table_search_BIN_TREE_MAX_split(const double *inp,
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

  parent->v = inp[mid];
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
    table_search_BIN_TREE_MAX_split(inp, ln, free_loc, min, mid);
  }
  if (rn) {
    table_search_BIN_TREE_MAX_split(inp, rn, free_loc, mid + 1, max);
  }

#ifdef TABLE_PRINT_TREE
  fprintf(stderr, "%5zu %5zu %5zu %.6f %p %p %p\n", min, max, mid,
          parent->v, parent, ln, rn);
#endif
}

static table_error
table_search_BIN_TREE_MAX_init(void *d, table_geometry g,
                               table_size nx, table_size ny,
                               const double *x, const double *y)
{
  table_search_BIN_TREE_MAX_data *dd;
  struct table_tree_node *t;

  dd = (table_search_BIN_TREE_MAX_data *)d;

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
  table_search_BIN_TREE_MAX_split(x, &dd->x[0], &t, 0, nx);

  table_tree_init(&dd->y[0].tree);
  t = &dd->y[1];
  table_search_BIN_TREE_MAX_split(y, &dd->y[0], &t, 0, ny);

  return TABLE_SUCCESS;
}

static table_error
table_search_BIN_TREE_MAX_core(double find_for,
                               table_size n,
                               struct table_tree_node *node, table_index *np)
{
  table_index i;
  struct table_tree *ns, *nss;

  TABLE_ASSERT(np);

  ns = &node->tree;
  nss = ns;

  if (n <= 1) {
    *np = 0;
    return TABLE_SUCCESS;
  }

  while (ns) {
    nss = ns;
    node = table_node_for_tree(ns);
    if (find_for <= node->v) {
      ns = table_tree_left(ns);
    } else {
      ns = table_tree_right(ns);
    }
  }
  node = table_node_for_tree(nss);
  if (find_for <= node->v) {
    i = node->i - 1;
  } else {
    i = node->i;
  }
  if (i < 0 && find_for == node->v) {
    i = node->i;
  }
  if (i >= n - 1 && find_for == node->v) {
    i = node->i - 1;
  }

  *np = i;
  if (i >= 0 && i < n - 1) {
    return TABLE_SUCCESS;
  } else {
    return TABLE_ERR_TABLE_RANGE;
  }
}

static table_error
table_search_BIN_TREE_MAX_find(void *d, double x, double y, table_node *node)
{
  table_index ix;
  table_index iy;
  table_error ex;
  table_error ey;

  table_search_BIN_TREE_MAX_data *dd;
  dd = (table_search_BIN_TREE_MAX_data *)d;

  TABLE_ASSERT(dd);
  TABLE_ASSERT(node);

  ex = table_search_BIN_TREE_MAX_core(x, dd->nx, dd->x, &ix);
  ey = table_search_BIN_TREE_MAX_core(y, dd->ny, dd->y, &iy);

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
const table_search_functions table_search_func_BIN_TREE_MAX = {
  .allocate   = table_search_BIN_TREE_MAX_alloc,
  .deallocate = table_search_BIN_TREE_MAX_dealloc,
  .init       = table_search_BIN_TREE_MAX_init,
  .find       = table_search_BIN_TREE_MAX_find,
};
