#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>

#include "table.h"
#include "table-func.h"

#include "linear.h"
#include "linear-save.h"
#include "bin-tree.h"
#include "bin-tree-minmax.h"

#ifdef TABLE_SEARCH_ALG_PINNED
#define TABLE_PINNED_E(x) TABLE_SALG_##x
#define TABLE_PINNED_S(x) table_search_func_##x
#define TABLE_PINNED_EE(x) TABLE_PINNED_E(x)
#define TABLE_PINNED_SS(x) TABLE_PINNED_S(x)
#define TABLE_PINNED_ENUM   TABLE_PINNED_EE(TABLE_SEARCH_ALG_PINNED)
#define TABLE_PINNED_STRUCT TABLE_PINNED_SS(TABLE_SEARCH_ALG_PINNED)
#endif

/**
 * @ingroup Table
 * @brief Table data
 */
struct table_data
{
  char *title;
  table_geometry geom;
  table_interp_mode mode;
  table_search_alg salg;
  double *x;
  double *y;
  double *d;
  table_size nx;
  table_size ny;
  table_index *ternary_offsets;
  void *alg_data;
#ifndef TABLE_SEARCH_ALG_PINNED
  const table_search_functions *funcs;
#endif
};

const char *table_errorstr(table_error e)
{
  switch(e) {
  case TABLE_SUCCESS:
    return "Success";
  case TABLE_ERR_ENDIAN:
    return "Wrong endianness used in binary file";
  case TABLE_ERR_EOF:
    return "Unexpected EOF reached in file";
  case TABLE_ERR_FORMAT:
    return "Invalid binary format";
  case TABLE_ERR_INVALID_GEOM:
    return "Invalid geometry specified";
  case TABLE_ERR_INVALID_IPMODE:
    return "Invalid interpolation mode specified";
   case TABLE_ERR_INVALID_SALG:
    return "Invalid search algorithm specified";
  case TABLE_ERR_NOMEM:
    return "Cannot allocate memory";
  case TABLE_ERR_NOT_INITED:
    return "Table data not initialized";
  case TABLE_ERR_NULLP:
    return "Null pointer exception";
  case TABLE_ERR_OVERFLOW:
    return "Too big table";
  case TABLE_ERR_RANGE:
    return "Values cannot be representable";
  case TABLE_ERR_TABLE_RANGE:
    return "Outside of table";
  case TABLE_ERR_SYS:
    return "System call error";
  default:
    return "(Unknown error occured)";
  }
}

int table_aerrorstr(char **buf, table_error e, int errval)
{
  const char *m;
  char *b;
  int sz, sz1;

  if (e == TABLE_ERR_SYS) {
    m = strerror(errval);
    if (!m) {
      m = table_errorstr(e);
    }
  } else {
    m = table_errorstr(e);
  }
  if (!m) return -1;
  sz = strlen(m);
  sz1 = sz + 1;
  if (sz < 0 || sz1 < sz) return -1;

  b = (char *)malloc(sizeof(char) * sz1);
  if (!b) return -1;
  strcpy(b, m);
  *buf = b;
  return sz;
}

table_size table_calc_data_size(table_geometry geom, table_size nx, table_size ny)
{
  table_index pnx;
  table_index pny;
  table_index pnxd;
  table_index pnyd;
  table_size ret;
  table_size ms = (sizeof(double) > sizeof(int)) ? sizeof(double) : sizeof(int);

  pnx = (table_index)nx;
  pny = (table_index)ny;
  pnxd = pnx * ms;
  pnyd = pny * ms;

  if (pnx <= 0 || pny <= 0 ||
      pnxd / ms != pnx || pnyd / ms != pny) {
    return (table_size)-1;
  }

  switch(geom) {
  case TABLE_GEOMETRY_RECTILINEAR:
    ret = pnx * pny;
    if (ret / pny != pnx) {
      return (table_size)-1;
    }
    break;

  case TABLE_GEOMETRY_SUM_CONSTANT:
    if (pnx != pny) {
      return (table_size)-1;
    }
    if (pnx + 1 <= 0) {
      return (table_size)-1;
    }
    ret = pnx * (pnx + 1);
    if (ret / pnx != pnx + 1) {
      return (table_size)-1;
    }
    ret /= 2;
    break;

  default:
    return (table_size)-1;
  }

  return ret;
}

table_size table_calc_ternary_size_at(table_size nx, table_index index)
{
  if (index < 0 || index >= nx) return (table_size)-1;
  return nx - index;
}

table_data *table_alloc(void)
{
  table_data *d;
  d = (table_data *)calloc(sizeof(table_data), 1);
  if (!d) return NULL;

  d->nx = 0;
  d->ny = 0;
  d->x = NULL;
  d->y = NULL;
  d->d = NULL;
  d->alg_data = NULL;
  d->ternary_offsets = NULL;
  d->salg = TABLE_SALG_INVALID;
  d->mode = TABLE_INTERP_INVALID;
  d->geom = TABLE_GEOMETRY_INVALID;
  d->title = NULL;
  return d;
}

static table_error table_init_alloc_data_copy(double **dest,
                                              const double *inp, table_size sz)
{
  double *d;
  table_size als;

  if (!dest) return TABLE_ERR_NULLP;
  if (!inp) {
    *dest = NULL;
    return TABLE_SUCCESS;
  }

  if (sz < 0) return TABLE_ERR_OVERFLOW;
  als = sizeof(double) * sz;
  if (als < 0 || (size_t)als / sizeof(double) != (size_t)sz) {
    return TABLE_ERR_OVERFLOW;
  }
  d = (double *)malloc(als);
  if (!d) {
    *dest = NULL;
    return TABLE_ERR_NOMEM;
  }

  /*
   * Note: For large sz *and* the computer system that has 4-or-more
   *       (typically same as running thread count) channels in
   *       accessing memory, OpenMP copy might faster than memcpy.
   */
#if 1
  memcpy(d, inp, sizeof(double) * sz);
#else
#pragma omp parallel for
  for (size_t is = 0; is < sz; ++is) {
    d[is] = inp[is];
  }
#endif

  *dest = d;
  return TABLE_SUCCESS;
}

table_error table_init(table_data *table, const char *title,
                       table_geometry geom, table_size nx, table_size ny,
                       table_interp_mode interpmode,
                       const double *xpos, const double *ypos,
                       const double *data)
{
  table_error e;
  table_size sz;

  if (!table) return TABLE_ERR_NULLP;

  switch(geom) {
  case TABLE_GEOMETRY_RECTILINEAR:
  case TABLE_GEOMETRY_SUM_CONSTANT:
    break;
  default:
    return TABLE_ERR_INVALID_GEOM;
  }

  switch(interpmode) {
  case TABLE_INTERP_LINEAR:
  case TABLE_INTERP_BARYCENTRIC:
    break;
  default:
    return TABLE_ERR_INVALID_IPMODE;
  }

  sz = table_calc_data_size(geom, nx, ny);
  if (sz == (table_size)-1) {
    return TABLE_ERR_RANGE;
  }

  table_destroy(table);

  e = table_init_alloc_data_copy(&table->x, xpos, nx);
  if (e != TABLE_SUCCESS) goto alloc_error;
  e = table_init_alloc_data_copy(&table->y, ypos, ny);
  if (e != TABLE_SUCCESS) goto alloc_error;
  e = table_init_alloc_data_copy(&table->d, data, sz);
  if (e != TABLE_SUCCESS) goto alloc_error;

  if (!title) {
    title = "";
  }
  sz = strlen(title) + 1;
  table->title = (char *)calloc(sizeof(char), sz);
  if (!table->title) goto alloc_error;

  memcpy(table->title, title, sz);

  if (geom == TABLE_GEOMETRY_SUM_CONSTANT) {
    table_size i;
    table_index *offs;

    offs = (table_index *)calloc(sizeof(table_index), ny + 1);
    if (!offs) {
      e = TABLE_ERR_NOMEM;
      goto alloc_error;
    }
    offs[0] = 0;
    for (i = 1; i < ny + 1; ++i) {
      offs[i] = offs[i - 1] + (nx - i + 1);
    }
    table->ternary_offsets = offs;
  }

  table->geom = geom;
  table->nx = nx;
  table->ny = ny;
  table->mode = interpmode;

  if (table_inited(table) && table->salg != TABLE_SALG_INVALID) {
    e = table_reset_search_state(table);
    if (e != TABLE_SUCCESS) goto alloc_error;
  }

  return TABLE_SUCCESS;

 alloc_error:
  table_destroy(table);
  return e;
}

int table_inited(table_data *table)
{
  if (!table) return 0;
  if (table->geom != TABLE_GEOMETRY_INVALID &&
      table->x && table->y && table->d) return 1;
  return 0;
}

table_error table_set_algorithm(table_data *table, table_search_alg alg)
{
#ifndef TABLE_SEARCH_ALG_PINNED
  table_error r;
  const table_search_functions *f;

  if (!table) return TABLE_ERR_NULLP;

  f = NULL;
  switch(alg) {
  case TABLE_SALG_LINEAR:
    f = &table_search_func_LINEAR;
    break;
  case TABLE_SALG_LINEAR_SAVE:
    f = &table_search_func_LINEAR_SAVE;
    break;
  case TABLE_SALG_BIN_TREE_MAX:
    f = &table_search_func_BIN_TREE_MAX;
    break;
  case TABLE_SALG_BIN_TREE_MINMAX:
    f = &table_search_func_BIN_TREE_MINMAX;
    break;
  default:
    return TABLE_ERR_INVALID_SALG;
  }

  table->funcs = f;

  r = TABLE_SUCCESS;
  if (table_inited(table) && table->salg != alg) {
    table->salg = alg;
    r = table_reset_search_state(table);
  }
  table->salg = alg;
  return r;
#else
  return TABLE_SUCCESS;
#endif
}

table_search_alg table_get_algorithm(table_data *table)
{
#ifndef TABLE_SEARCH_ALG_PINNED
  if (!table) return TABLE_SALG_INVALID;

  return table->salg;
#else
  return TABLE_PINNED_ENUM;
#endif
}

table_interp_mode table_get_interp_mode(table_data *table)
{
  if (!table) return TABLE_INTERP_INVALID;

  return table->mode;
}

table_error table_set_interp_mode(table_data *table, table_interp_mode mode)
{
  if (!table) return TABLE_ERR_NULLP;

  switch(mode) {
  case TABLE_INTERP_BARYCENTRIC:
  case TABLE_INTERP_LINEAR:
    table->mode = mode;
    break;
  default:
    return TABLE_ERR_INVALID_IPMODE;
  }
  return TABLE_SUCCESS;
}

table_geometry table_get_geometry(table_data *table)
{
  if (!table) return TABLE_GEOMETRY_INVALID;

  return table->geom;
}

table_size table_get_nx(table_data *table)
{
  if (!table) return (table_size)-1;

  return table->nx;
}

table_size table_get_ny(table_data *table)
{
  if (!table) return (table_size)-1;

  return table->ny;
}

table_size table_get_data_size(table_data *table)
{
  if (!table) return (table_size)-1;

  return table_calc_data_size(table->geom, table->nx, table->ny);
}

const double *table_get_xdata(table_data *table)
{
  if (!table) return NULL;

  return table->x;
}

const double *table_get_ydata(table_data *table)
{
  if (!table) return NULL;

  return table->y;
}

const double *table_get_data(table_data *table)
{
  if (!table) return NULL;

  return table->d;
}

const char *table_get_title(table_data *table)
{
  if (!table) return NULL;

  return table->title;
}

table_error table_set_title(table_data *table, const char *title, size_t n)
{
  char *buf;

  if (!table) return TABLE_ERR_NULLP;

  if (!title) {
    title = "";
    n = 0;
  }
  if (n == 0) {
    n = strlen(title) + 1;
  }
  if (table->title && strncmp(table->title, title, n) == 0) {
    /* Not changing */
    return TABLE_SUCCESS;
  }

  /* Old size is unknown, so allocating new one. */
  buf = (char *)malloc(sizeof(char) * n);
  if (!buf) return TABLE_ERR_NOMEM;

  strncpy(buf, title, n);
  buf[n - 1] = '\0';

  free(table->title);
  table->title = buf;

  return TABLE_SUCCESS;
}

table_error table_reset_search_state(table_data *table)
{
  table_error e;
  const table_search_functions *p;

  if (!table) return TABLE_ERR_NULLP;

#ifndef TABLE_SEARCH_ALG_PINNED
  p = table->funcs;
#else
  p = &(TABLE_PINNED_STRUCT);
#endif
  if (!p) {
    return TABLE_ERR_INVALID_SALG;
  }

  if (!p->deallocate || !p->allocate || !p->init) {
    return TABLE_ERR_INVALID_SALG;
  }

  e = TABLE_ERR_INVALID_SALG;
  if (table->alg_data) {
    p->deallocate(table->alg_data);
  }
  table->alg_data = p->allocate();
  if (!table->alg_data) {
    return TABLE_ERR_NOMEM;
  }
  if (table_inited(table)) {
    e = p->init(table->alg_data, table->geom,
                table->nx, table->ny, table->x, table->y);
  } else {
    e = TABLE_ERR_NOT_INITED;
  }

  return e;
}

table_error table_search_node(table_data *table, double x, double y,
                              table_node *node)
{
  table_error e;

  /*
   * Uses TABLE_ASSERT instead of returning TABLE_ERR_NULLP
   * to improve compiler optimization
   */
  TABLE_ASSERT(table);
  TABLE_ASSERT(node);
  TABLE_ASSERT(table_inited(table));
  TABLE_ASSERT(table->alg_data);

#ifdef TABLE_SEARCH_ALG_PINNED
  TABLE_ASSERT((TABLE_PINNED_STRUCT).find);

  e = (TABLE_PINNED_STRUCT).find(table->alg_data, x, y, node);
#else
  TABLE_ASSERT(table->funcs);
  TABLE_ASSERT(table->funcs->find);

  e = table->funcs->find(table->alg_data, x, y, node);
#endif

  if (table->geom == TABLE_GEOMETRY_SUM_CONSTANT) {
    table_size ls;
    /*
     * In case of very special point such as (x, y) = (0.5, 0.5) when
     * x + y <= 1.0. Since indices are returned by lower left corner,
     * these values are valid but indicating mesh is invalid.
     */
    ls = table_calc_ternary_size_at(table->nx, node->x_index);
    if (ls != (table_size)-1 && node->y_index == ls - 1) {
      if (x == table->x[node->x_index] &&
          y == table->y[node->y_index]) {
        node->x_index--;
        e = TABLE_SUCCESS;
      }
    }
  }

  return e;
}

double table_interpolate(table_data *table, table_node node,
                         double x, double y, table_error *err)
{
  table_error e;
  table_index off[4];
  table_size nx;
  table_size ny;
  double *xp;
  double *yp;
  double ret;

  if (!table) {
    if (err) *err = TABLE_ERR_NULLP;
    return NAN;
  }
  if (!table_inited(table)) {
    if (err) *err = TABLE_ERR_NOT_INITED;
    return NAN;
  }

  e = TABLE_SUCCESS;

  nx = table->nx;
  ny = table->ny;

  TABLE_ASSERT((table->geom == TABLE_GEOMETRY_SUM_CONSTANT) ? (ny == nx && nx > 1) : (1));

  if (nx > 1) {
    if (node.x_index < 0) {
      e = TABLE_ERR_TABLE_RANGE;
      node.x_index = 0;
    }
    if (node.x_index >= nx - 1) {
      e = TABLE_ERR_TABLE_RANGE;
      node.x_index = nx - 2;
    }
  } else {
    node.x_index = 0;
  }
  if (ny > 1) {
    if (node.y_index < 0) {
      e = TABLE_ERR_TABLE_RANGE;
      node.y_index = 0;
    }
    if (node.y_index >= ny - 1) {
      e = TABLE_ERR_TABLE_RANGE;
      node.y_index = ny - 2;
    }
  } else {
    node.y_index = 0;
  }

  xp = table->x + node.x_index;
  yp = table->y + node.y_index;

  switch(table->geom) {
  case TABLE_GEOMETRY_RECTILINEAR:
    off[0] = nx * node.y_index + node.x_index;
    if (nx > 1) {
      off[1] = off[0] + 1;
    } else {
      off[1] = -1;
    }
    if (ny > 1) {
      off[2] = off[0] + nx;
      if (nx > 1) {
        off[3] = off[2] + 1;
      } else {
        off[3] = -1;
      }
    } else {
      off[2] = -1;
      off[3] = -1;
    }
    break;
  case TABLE_GEOMETRY_SUM_CONSTANT:
    off[0] = table->ternary_offsets[node.y_index] + node.x_index;
    off[1] = off[0] + 1;
    off[2] = table->ternary_offsets[node.y_index + 1];
    if (off[1] >= off[2]) {
      if (err) *err = TABLE_ERR_TABLE_RANGE;

      off[1] = off[2] - 1;
      off[0] = off[2] - 2;
      node.x_index = off[0] - table->ternary_offsets[node.y_index];
    }
    off[2] += node.x_index;
    off[3] = off[2] + 1;
    if (off[3] >= table->ternary_offsets[node.y_index + 2]) {
      off[3] = -1;
    }
    break;
  default:
    if (err) *err = TABLE_ERR_INVALID_GEOM;
    return NAN;
  }

  switch(table->mode) {
  case TABLE_INTERP_LINEAR:
    {
      double t00, t01, t10, t11, dx, dy;

      t00 = table->d[off[0]];
      if (off[1] >= 0) {
        t01 = table->d[off[1]];
        dx = (x - xp[0]) / (xp[1] - xp[0]);
      } else {
        t01 = 0.0;
        dx = 0.0;
      }
      if (off[2] >= 0) {
        t10 = table->d[off[2]];
        dy = (y - yp[0]) / (yp[1] - yp[0]);

        if (off[3] >= 0) {
          t11 = table->d[off[3]];
        } else {
          t11 = 0.0;
        }
      } else {
        dy = 0.0;
        t10 = 0.0;
        t11 = 0.0;
      }

      if (off[1] >= 0) {
        ret = (t01 - t00) * dx + t00;
      } else {
        ret = t00;
      }
      if (off[2] >= 0) {
        if (off[3] >= 0) {
          t10 = (t11 - t10) * dx + t10;
        } else {
          if (off[1] >= 0) {
            if (err) *err = TABLE_ERR_TABLE_RANGE;
            return NAN;
          }
          /* t10 = t10; */
        }
        ret = (t10 - ret) * dy + t00;
      }
    }
    break;
  case TABLE_INTERP_BARYCENTRIC:
    {
      double dx, dy;      /* Cell width and height */
      double xs, ys;      /* Offset from cell boundary */
      double a0, a1, a2;  /* Each area of triangles */
      double a;           /* Total area of triangles */
      double l1, l2;      /* Each base lengths of triangles */
      double h1, h2;      /* Each heights of triangles */
      double t0, t1, t2;  /* Each data values */
      double cs60 = 0.5;                     /* Cosine of 60-degrees */
      double sn60 = sqrt(1.0 - cs60 * cs60); /* Sine of 60-degrees */

      dx = xp[1] - xp[0];
      dy = yp[1] - yp[0];

      xs = x - xp[0];
      ys = y - yp[0];
      if (dy * xs + dx * ys > dx * dy) {
        /*
         * (x, y) is pointing upper side of
         * rectangle (in case of rectilinear geometry) or
         * parallelogram (in case of ternary geometry)
         *
         *                 l2
         * (off[2]) t1 *---------* t0 (off[3])
         *            / \   |h2 /
         *           /   \  *  / l1
         *          /     \h1`/
         *         /       \ /
         *        *---------* t2 (off[1])
         */
        if (off[3] >= 0) {
          xs = dx - xs;
          ys = dy - ys;
          t0 = table->d[off[3]];
          t1 = table->d[off[2]];
          t2 = table->d[off[1]];
        } else {
          e = TABLE_ERR_TABLE_RANGE;
          t0 = table->d[off[0]];
          t1 = table->d[off[1]];
          t2 = table->d[off[2]];
        }
      } else {
        /*
         *      (off[2]) t2 *---------*
         *                 / \       /
         *                /h1 \     /
         *            l1 / `*  \   /
         *              /   |h2 \ /
         * (off[0]) t0 *---------* t1 (off[1])
         *                  l2
         */
        t0 = table->d[off[0]];
        t1 = table->d[off[1]];
        t2 = table->d[off[2]];
      }

      /* For area, multipling 0.5 is a common part, so omitted. */
      if (table->geom == TABLE_GEOMETRY_RECTILINEAR) {
        l1 = dy;
        h1 = xs;
        l2 = dx;
        h2 = ys;
        a = dx * dy;
      } else {
        l1 = dy / sn60;
        h1 = xs;
        l2 = dx / sn60;
        h2 = ys;
        a = l2 * dy;
      }
      a1 = l1 * h1;
      a2 = l2 * h2;
      a0 = a - (a1 + a2);
      ret = (a0 * t0 + a1 * t1 + a2 * t2) / a;
    }
    break;
  default:
    if (err) *err = TABLE_ERR_INVALID_IPMODE;
    return NAN;
  }
  if (err) *err = e;
  return ret;
}

double table_search(table_data *table, double x, double y, table_error *err)
{
  table_error es, ei;
  table_node n;
  double r;

  es = table_search_node(table, x, y, &n);
  r = table_interpolate(table, n, x, y, &ei);
  if (err) {
    if (es != TABLE_SUCCESS) {
      *err = es;
    } else {
      *err = ei;
    }
  }
  return r;
}


void table_destroy(table_data *table)
{
  const table_search_functions *p;

  if (!table) return;

#ifndef TABLE_SEARCH_ALG_PINNED
  p = table->funcs;
#else
  p = &(TABLE_PINNED_STRUCT);
#endif

  if (table->alg_data && p && p->deallocate) {
    p->deallocate(table->alg_data);
  }
  free(table->title);
  free(table->d);
  free(table->x);
  free(table->y);
  free(table->ternary_offsets);

  table->geom = TABLE_GEOMETRY_INVALID;
  table->nx = 0;
  table->ny = 0;
  table->x = NULL;
  table->y = NULL;
  table->d = NULL;
  table->ternary_offsets = NULL;
  table->alg_data = NULL;
}

void table_free(table_data *table)
{
  if (!table) return;

  table_destroy(table);
  free(table);
}

#if defined(__GNUC__) && !(defined(__PGI) || defined(__NVCOMPILER))
#define TABLE_TRAP() __builtin_trap()
#else
#ifdef _MSC_VER
#include <windows.h>
#define TABLE_TARP() RaiseException(EXCEPTION_ILLEGAL_INSTRUCTION, EXCEPTION_NONCONTINUABLE, 0, NULL)
#else
#define TABLE_TRAP() do { char *s = NULL; *s = 1; } while(1)
#endif
#endif

void table_assert_impl(const char *file, long line, const char *cond_str)
{
  fprintf(stderr, "Assertion failed in %s(%ld): %s\n",
          file, line, cond_str);
  TABLE_TRAP();
}
