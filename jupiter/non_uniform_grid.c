#include "non_uniform_grid.h"
#include "csv.h"
#include "csvutil.h"
#include "geometry/list.h"
#include "struct.h"

#include <math.h>
#include <stdlib.h>

struct non_uniform_grid_funcs
{
  void *(*new_param)(void);
  int (*read_param)(void *arg, csv_data *csv, const char *fname, csv_row *row,
                    csv_column **col);
  void (*free_param)(void *arg);
  type (*dfd)(void *arg, type x);
  type (*fd)(void *arg, type x);
};

static const struct non_uniform_grid_funcs non_uniform_grid_CONST;
static const struct non_uniform_grid_funcs non_uniform_grid_CONST_RATIO_INC;
static const struct non_uniform_grid_funcs non_uniform_grid_CONST_RATIO_DEC;
static const struct non_uniform_grid_funcs non_uniform_grid_SINE;
static const struct non_uniform_grid_funcs non_uniform_grid_QSINE_B;
static const struct non_uniform_grid_funcs non_uniform_grid_QSINE_E;

static const struct non_uniform_grid_funcs *
non_uniform_grid_get_funcs(non_uniform_grid_function func)
{
  switch (func) {
#define GET_FUNC(n)               \
  case NON_UNIFORM_GRID_FUNC_##n: \
    return &non_uniform_grid_##n
    GET_FUNC(CONST);
    GET_FUNC(CONST_RATIO_INC);
    GET_FUNC(CONST_RATIO_DEC);
    GET_FUNC(SINE);
    GET_FUNC(QSINE_B);
    GET_FUNC(QSINE_E);
#undef GET_FUNC
  case NON_UNIFORM_GRID_FUNC_INVALID:
    break;
  }
  return NULL;
}

#ifndef M_E
#define M_E 2.7182818284590452354
#endif
#ifndef M_PI_2
#ifdef M_PI
#define M_PI_2 (M_PI * 0.5)
#else
#define M_PI_2 1.57079632679489661923
#endif
#endif
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static type non_uniform_grid_invalid_value(void)
{
#ifdef NAN
  return NAN;
#else
#ifdef JUPITER_DOUBLE
  return HUGE_VAL;
#else
  return HUGE_VALF;
#endif
#endif
}

void non_uniform_grid_input_data_init(struct non_uniform_grid_input_data *p)
{
  CSVASSERT(p);

  geom_list_init(&p->list);
  p->high = 0.0;
  p->func = NON_UNIFORM_GRID_FUNC_INVALID;
  p->ndiv = 0;
  p->funcs = NULL;
  p->func_param = NULL;
}

struct non_uniform_grid_input_data *non_uniform_grid_input_data_new(void)
{
  struct non_uniform_grid_input_data *d;
  d = (struct non_uniform_grid_input_data *)malloc(
    sizeof(struct non_uniform_grid_input_data));
  if (!d)
    return NULL;

  non_uniform_grid_input_data_init(d);
  return d;
}

struct non_uniform_grid_input_data *
non_uniform_grid_input_data_next(struct non_uniform_grid_input_data *p)
{
  CSVASSERT(p);

  return non_uniform_grid_input_entry(geom_list_next(&p->list));
}

struct non_uniform_grid_input_data *
non_uniform_grid_input_data_prev(struct non_uniform_grid_input_data *p)
{
  CSVASSERT(p);

  return non_uniform_grid_input_entry(geom_list_prev(&p->list));
}

type non_uniform_grid_input_data_start(struct non_uniform_grid_input_data *p)
{
  CSVASSERT(p);

  p = non_uniform_grid_input_data_prev(p);
  return p->high;
}

type non_uniform_grid_input_data_end(struct non_uniform_grid_input_data *p)
{
  CSVASSERT(p);
  return p->high;
}

type non_uniform_grid_input_data_width(struct non_uniform_grid_input_data *p)
{
  CSVASSERT(p);
  type rs = non_uniform_grid_input_data_start(p);
  type re = non_uniform_grid_input_data_end(p);
  if (isfinite(rs) && isfinite(re))
    return re - rs;
#ifdef isinf
  if (isfinite(rs) && isinf(re))
    return re - rs; // inf
#endif
  return non_uniform_grid_invalid_value();
}

int non_uniform_grid_input_data_ndiv(struct non_uniform_grid_input_data *p)
{
  CSVASSERT(p);
  return p->ndiv;
}

void non_uniform_grid_input_data_set_ndiv(struct non_uniform_grid_input_data *p,
                                          int ndiv)
{
  CSVASSERT(p);
  CSVASSERT(ndiv > 0);
  p->ndiv = ndiv;
}

non_uniform_grid_function
non_uniform_grid_input_data_function(struct non_uniform_grid_input_data *p)
{
  CSVASSERT(p);
  return p->func;
}

void non_uniform_grid_input_data_delete(struct non_uniform_grid_input_data *p)
{
  if (!p)
    return;

  geom_list_delete(&p->list);
  if (p->funcs && p->funcs->free_param)
    p->funcs->free_param(p->func_param);
  free(p);
}

void non_uniform_grid_input_data_delete_all(
  struct non_uniform_grid_input_data *head)
{
  CSVASSERT(head);

  struct geom_list *lp, *ln, *lh;
  lh = &head->list;
  geom_list_foreach_safe (lp, ln, lh) {
    struct non_uniform_grid_input_data *p;
    p = non_uniform_grid_input_entry(lp);
    non_uniform_grid_input_data_delete(p);
  }
}

int non_uniform_grid_input_read_csv(
  struct non_uniform_grid_input_data *out_head, int nreg, csv_data *csv,
  const char *fname, csv_row **row, csv_column **col, int *stat)
{
  int lstat;
  int ret;
  int n;
  type low = out_head->high;
  SET_P_INIT(csv, fname, row, col);

  lstat = OFF;
  ret = 0;
  n = 0;
  for (int ir = 0; (nreg < 0 && *col) || ir < nreg; ++ir) {
    struct non_uniform_grid_input_data *d;
    const struct non_uniform_grid_funcs *funcs;
    non_uniform_grid_function func;
    type high;
    int ndiv;
    void *pdata;

    d = non_uniform_grid_input_data_new();
    if (!d) {
      csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
                NULL);
      ret = 2;
    }

    SET_P_NEXT_REQUIRED(&high, double, HUGE_VAL, &lstat);
    if (!SET_P_PERROR_GREATER(high, low, OFF, OFF, ERROR,
                              "The coordinate of ending region must be greater "
                              "than the start, %g",
                              low)) {
      ret = 1;
    }

    SET_P_NEXT_REQUIRED(&func, non_uniform_grid_func,
                        NON_UNIFORM_GRID_FUNC_INVALID, &lstat);
    funcs = non_uniform_grid_get_funcs(func);
    if (func != NON_UNIFORM_GRID_FUNC_INVALID && !funcs) {
      SET_P_PERROR(ERROR, "Given function is not implemented.");
      ret = 1;
    }

    SET_P_NEXT_REQUIRED(&ndiv, int, 0, &lstat);
    if (ndiv < 1) {
      SET_P_PERROR(ERROR, "The number of divisions must be positive");
      ret = 1;
    }

    pdata = NULL;
    if (funcs && funcs->new_param) {
      /* new_param is allow to return NULL */
      pdata = funcs->new_param();
    }
    if (funcs && funcs->read_param) {
      if (funcs->read_param(pdata, csv, fname, *row, col))
        ret = 1;
    }

    if (!d && funcs && funcs->free_param)
      funcs->free_param(pdata);

    if (d) {
      d->funcs = funcs;
      d->func_param = pdata;
      d->func = func;
      d->high = high;
      d->ndiv = ndiv;
      geom_list_insert_prev(&out_head->list, &d->list);
    }
  }
  if (lstat != OFF && ret < 1)
    ret = 1;
  if (stat && ret)
    *stat = ON;
  return ret;
}

type non_uniform_grid_total_length(struct non_uniform_grid_input_data *head)
{
  struct non_uniform_grid_input_data *l;
  l = non_uniform_grid_input_entry(geom_list_prev(&head->list));
  return l->high - head->high;
}

int non_uniform_grid_total_ndivs(struct non_uniform_grid_input_data *head)
{
  struct geom_list *lp, *lh;
  int i = 0;
  lh = &head->list;
  geom_list_foreach (lp, lh) {
    struct non_uniform_grid_input_data *p;
    p = non_uniform_grid_input_entry(lp);
    i += p->ndiv;
  }
  return i;
}

static type non_uniform_grid_calc_factor(struct non_uniform_grid_input_data *p,
                                         int j)
{
  if (!p || !p->funcs || !p->funcs->fd || p->ndiv <= 0)
    return non_uniform_grid_invalid_value();

  if (j <= 0)
    return 0.; /* Assumes p->funcs->fd(0.) == 0. */
  if (j >= p->ndiv)
    return 1.; /* Assumes p->funcs->fd(0.) == 1. */
  return p->funcs->fd(p->func_param, (type)j / p->ndiv);
}

type non_uniform_grid_input_data_calc_relcoord(
  struct non_uniform_grid_input_data *p, int j)
{
  type fac, w;

  if (!p || !p->funcs || p->ndiv <= 0)
    return non_uniform_grid_invalid_value();

  w = non_uniform_grid_input_data_width(p);
  if (!isfinite(w) || w <= 0.0)
    return non_uniform_grid_invalid_value();

  if (j < 0) {
    type p0 = non_uniform_grid_input_data_calc_relcoord(p, 0);
    type p1 = non_uniform_grid_input_data_calc_relcoord(p, 1);
    CSVASSERT(p1 > p0);
    return (p1 - p0) * j;
  }
  if (j > p->ndiv) {
    type p0 = non_uniform_grid_input_data_calc_relcoord(p, p->ndiv);
    type p1 = non_uniform_grid_input_data_calc_relcoord(p, p->ndiv - 1);
    CSVASSERT(p0 > p1);
    return p0 + (p0 - p1) * (j - p->ndiv);
  }

  fac = non_uniform_grid_calc_factor(p, j);
  return fac * w;
}

type non_uniform_grid_input_data_calc_abscoord(
  struct non_uniform_grid_input_data *p, int j)
{
  type fac, l, w;

  if (!p || !p->funcs || p->ndiv <= 0)
    return non_uniform_grid_invalid_value();

  w = non_uniform_grid_input_data_width(p);
  if (!isfinite(w) || w <= 0.0)
    return non_uniform_grid_invalid_value();

  if (j > p->ndiv) {
    type p0 = non_uniform_grid_input_data_calc_abscoord(p, p->ndiv);
    type p1 = non_uniform_grid_input_data_calc_abscoord(p, p->ndiv - 1);
    CSVASSERT(p0 > p1);
    return p0 + (p0 - p1) * (j - p->ndiv);
  }

  if (j == p->ndiv)
    return p->high;

  l = non_uniform_grid_input_data_start(p);
  return l + non_uniform_grid_input_data_calc_relcoord(p, j);
}

int non_uniform_grid_build_mesh(struct non_uniform_grid_input_data *head, //
                                int js, int je, int circular, type *v)
{
  struct non_uniform_grid_input_data *p, *pb;
  struct geom_list *lp, *lh;
  int totdiv;
  int ijs, ije;
  int circle_count = 0;
  type base_v;
  type total_length = non_uniform_grid_total_length(head);

  lh = &head->list;
  if (js < 0) {
    if (circular) {
      lp = geom_list_prev(lh);
      if (lp != lh) {
        circle_count = -1;
        for (; js < 0; lp = geom_list_prev(lp)) {
          if (lp == lh) {
            circle_count -= 1;
            continue;
          }

          p = non_uniform_grid_input_entry(lp);
          js += p->ndiv;
          je += p->ndiv;
          if (js >= 0)
            break;
        }
      }
    } else {
      lp = geom_list_next(lh);
    }
  } else {
    struct geom_list *ln;
    lp = geom_list_next(lh);
    ln = geom_list_next(lp);
    for (; lp != lh; lp = ln, ln = geom_list_next(ln)) {
      p = non_uniform_grid_input_entry(lp);
      if (js < p->ndiv)
        break;

      if (ln == lh) {
        if (circular) {
          ln = geom_list_next(lh);
          circle_count += 1;
        } else {
          break;
        }
      }

      js -= p->ndiv;
      je -= p->ndiv;
    }
  }

  if (lp == lh)
    return 1;

  p = non_uniform_grid_input_entry(lp);
  base_v = circle_count * total_length;

  for (int i = 0, ii = js; ii < je; ++i, ++ii) {
    if (ii == p->ndiv) {
      struct geom_list *ln;
      ln = geom_list_next(lp);
      if (ln != lh || circular) {
        if (ln == lh) {
          circle_count += 1;
          ln = geom_list_next(ln);
          base_v = circle_count * total_length;
        }
        ii -= p->ndiv;
        js -= p->ndiv;
        je -= p->ndiv;
        lp = ln;
        p = non_uniform_grid_input_entry(lp);
      }
    }

    v[i] = base_v + non_uniform_grid_input_data_calc_abscoord(p, ii);
  }

  return 0;
}

void non_uniform_grid_set_derived_vars(int n, type *v, type *c, type *dv,
                                       type *dc, type *dcp, type *dcn,
                                       type *dvp, type *dvn)
{
  if (c) {
    for (int i = 0; i < n; ++i)
      c[i] = (v[i] + v[i + 1]) * 0.5;
  }

  if (dv) {
    for (int i = 0; i < n; ++i)
      dv[i] = v[i + 1] - v[i];
  }

  if (dc) {
    for (int i = 0; i < n - 1; ++i)
      dc[i] = c[i + 1] - c[i];
  }

  if (dcn || dcp) {
    for (int i = 0; i < n; ++i) {
      if (dcn)
        dcn[i] = c[i] - v[i];
      if (dcp)
        dcp[i] = v[i + 1] - c[i];
    }
  }

  if (dvn || dvp) {
    if (dvn)
      dvn[0] = -HUGE_VAL;
    if (dvp)
      dvp[n] = HUGE_VAL;
    for (int i = 0; i < n; ++i) {
      if (dvp)
        dvp[i] = c[i] - v[i];
      if (dvn)
        dvn[i + 1] = v[i + 1] - c[i];
    }
  }
}

//--- functions

static type non_uniform_grid_CONST_dfd(void *arg, type x) { return 1.0; }
static type non_uniform_grid_CONST_fd(void *arg, type x) { return x; }

static const struct non_uniform_grid_funcs non_uniform_grid_CONST = {
  .dfd = non_uniform_grid_CONST_dfd,
  .fd = non_uniform_grid_CONST_fd,
};

static type non_uniform_grid_CONST_RATIO_INC_dfd(void *arg, type x)
{
  return exp(x) / (M_E - 1.);
}

static type non_uniform_grid_CONST_RATIO_INC_fd(void *arg, type x)
{
  return (exp(x) - 1.) / (M_E - 1.);
}

static const struct non_uniform_grid_funcs non_uniform_grid_CONST_RATIO_INC = {
  .dfd = non_uniform_grid_CONST_RATIO_INC_dfd,
  .fd = non_uniform_grid_CONST_RATIO_INC_fd,
};

static type non_uniform_grid_CONST_RATIO_DEC_dfd(void *arg, type x)
{
  return exp(1. - x) / (M_E - 1.);
}

static type non_uniform_grid_CONST_RATIO_DEC_fd(void *arg, type x)
{
  return (M_E - exp(1. - x)) / (M_E - 1.);
}

static const struct non_uniform_grid_funcs non_uniform_grid_CONST_RATIO_DEC = {
  .dfd = non_uniform_grid_CONST_RATIO_DEC_dfd,
  .fd = non_uniform_grid_CONST_RATIO_DEC_fd,
};

static type non_uniform_grid_SINE_dfd(void *arg, type x)
{
  return M_PI_2 * sin(M_PI * x);
}

static type non_uniform_grid_SINE_fd(void *arg, type x)
{
  return 0.5 - 0.5 * cos(M_PI * x);
}

static const struct non_uniform_grid_funcs non_uniform_grid_SINE = {
  .dfd = non_uniform_grid_SINE_dfd,
  .fd = non_uniform_grid_SINE_fd,
};

static type non_uniform_grid_QSINE_B_dfd(void *arg, type x)
{
  return M_PI_2 * sin(M_PI_2 * x);
}

static type non_uniform_grid_QSINE_B_fd(void *arg, type x)
{
  return 1. - cos(M_PI_2 * x);
}

static const struct non_uniform_grid_funcs non_uniform_grid_QSINE_B = {
  .dfd = non_uniform_grid_QSINE_B_dfd,
  .fd = non_uniform_grid_QSINE_B_fd,
};

static type non_uniform_grid_QSINE_E_dfd(void *arg, type x)
{
  return M_PI_2 * cos(M_PI_2 * x);
}

static type non_uniform_grid_QSINE_E_fd(void *arg, type x)
{
  return sin(M_PI_2 * x);
}

static const struct non_uniform_grid_funcs non_uniform_grid_QSINE_E = {
  .dfd = non_uniform_grid_QSINE_E_dfd,
  .fd = non_uniform_grid_QSINE_E_fd,
};
