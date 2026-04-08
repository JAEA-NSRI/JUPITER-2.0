
#ifndef TMCALC_H
#define TMCALC_H

#include <math.h>   /* for HUGE_VAL */
#include <string.h> /* for memset */

#include "component_info_defs.h"
#include "table/table.h"
#include "tmcalc_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(TMCALC_STANDALONE) && TMCALC_STANDALONE != 0
typedef double type;
#else
#include "struct.h" /* for floating point `type` */
#endif

static inline
type tm_calc_arithmetic_mean(int n, type *w, type *tm)
{
  int i;
  volatile type ws;
  type ts;
  ws = 0.0;
  ts = 0.0;
  for (i = 0; i < n; ++i) {
    ws += w[i];
    ts += w[i] * tm[i];
  }
  if (ws != 0.0) {
    return ts / ws;
  } else {
    return 0.0;
  }
}

static inline
type tm_calc_harmonic_mean(int n, type *w, type *tm)
{
  int i;
  type ws;
  volatile type ts;
  ws = 0.0;
  ts = 0.0;
  for (i = 0; i < n; ++i) {
    ws += w[i];
    ts += w[i] / tm[i];
  }
  if (ts != 0.0) {
    return ws / ts;
  } else {
    return 0.0;
  }
}

static inline type
tm_calc_get_Y(int nc, type *Y, const struct component_info_data *get)
{
  int ic;

  if (!get->d)
    return 0.0;

  ic = get->d->comp_index;
  if (ic >= 0 && ic < nc)
    return Y[ic];
  return 0.0;
}

static inline void
tm_calc_set_used(int nc, int *uflg, const struct component_info_data *id_to_set)
{
  int ic;

  if (!id_to_set->d)
    return;

  ic = id_to_set->d->comp_index;
  if (ic >= 0 && ic < nc) {
    uflg[ic] = 1;
  }
}

static inline type
tm_calc_linear(type x, type startx, type endx, type starty, type endy)
{
  return (x - startx) * (endy - starty) / (endx - startx) + starty;
}

static inline type
tm_calc_linear_degC(type x, type startx, type endx, type starty, type endy)
{
  return tm_calc_linear(x, startx, endx, starty, endy) + 273.0;
}

static inline type
tm_calc_Fe_Zr_liq(type Yzr)
{
  if (Yzr < 0.151) {
    return tm_calc_linear_degC(Yzr, 0.0, 0.151, 1465.93, 1305.0);
  }
  if (Yzr < 0.457) {
    return tm_calc_linear_degC(Yzr, 0.151, 0.457, 1305.0, 1673.0);
  }
  if (Yzr < 0.838) {
    return tm_calc_linear_degC(Yzr, 0.457, 0.838, 1673.0, 928.0);
  }
  return tm_calc_linear_degC(Yzr, 0.838, 1.0, 928.0, 1855.0);
}

static inline type
tm_calc_Fe_B_liq(type Yb)
{
  if (Yb < 0.037) {
    return tm_calc_linear_degC(Yb, 0.0, 0.037, 1450.0, 1195.0);
  }
  if (Yb < 0.162) {
    return tm_calc_linear_degC(Yb, 0.037, 0.162, 1195.0, 1650.0);
  }
  if (Yb < 0.25) {
    return tm_calc_linear_degC(Yb, 0.162, 0.25, 1650.0, 1500.0);
  }
  return tm_calc_linear_degC(Yb, 0.25, 1.0, 1500.0, 2763.0);
}

static inline void
tm_calc_table(int nc, struct tm_table_param *tmtable,
              type *Y, int *nd, type *tptr, type *yptr,
              int *uflg, table_error *err, type *x, type *y)
{
  table_error terr;
  type Yx, Yy, Yr, Ys, Yxd, Yyd;

  terr = TABLE_SUCCESS;
  if (!tmtable) return;
  for (; tmtable; tmtable = tmtable->next) {
    Yx = tm_calc_get_Y(nc, Y, &tmtable->xid);
    Yy = tm_calc_get_Y(nc, Y, &tmtable->yid);
    Yr = tm_calc_get_Y(nc, Y, &tmtable->rid);
    Ys = Yx + Yy + Yr;
    if (Ys != 0.0) {
      Yxd = Yx / Ys;
      Yyd = Yy / Ys;
    } else {
      Yxd = 0.0;
      Yyd = 0.0;
    }
    if (*nd >= nc) break; /* Too many tables */
    tm_calc_set_used(nc, uflg, &tmtable->xid);
    tm_calc_set_used(nc, uflg, &tmtable->yid);
    tm_calc_set_used(nc, uflg, &tmtable->rid);
    tptr[*nd] = table_search(tmtable->table, Yxd, Yyd, &terr);
    yptr[*nd] = Ys;
    (*nd)++;
    if (terr != TABLE_SUCCESS) {
      if (err) *err = terr;
      if (x) *x = Yxd;
      if (y) *y = Yyd;
    }
  }
}

static inline void
tm_calc_funcs2(int nc, struct tm_func2_param *tmfuncs,
               type *Y, int *nd, type *tptr, type *yptr, int *uflg)
{
  type Yx, Yr, Ys, Tm;

  if (!tmfuncs) return;
  for (; tmfuncs; tmfuncs = tmfuncs->next) {
    Yx = tm_calc_get_Y(nc, Y, &tmfuncs->xid);
    Yr = tm_calc_get_Y(nc, Y, &tmfuncs->yid); /* for yid < 0, gets 0.0. */
    Ys = Yx + Yr;
    switch(tmfuncs->model) {
    case TM_FUNC_LIQUIDUS_FE_ZR:
      Yx = (Yx*0.98)/(Yx*0.98+(1.0-Yx)*0.71);//modifed by Chai to convert the compoent in mixture to pure material in the phase diagram;
      Tm = tm_calc_Fe_Zr_liq(Yx);
      break;
    case TM_FUNC_LIQUIDUS_FE_B:
      Tm = tm_calc_Fe_B_liq(Yx);
      break;
    default:
      Tm = HUGE_VAL;
      break;
    }
    if (*nd >= nc) return; /* Too many func definitions */
    tm_calc_set_used(nc, uflg, &tmfuncs->xid);
    tm_calc_set_used(nc, uflg, &tmfuncs->yid);
    tptr[*nd] = Tm;
    yptr[*nd] = Ys;
    (*nd)++;
  }
}


/* YSE: Solidus / Liquidus temperature calculation with table */
/**
 * @brief Solidus / Liquidus temperature calculation for solute_diff model
 * @raram nc NumberOfComponent
 * @param Y  List of Concentrations (Array of nc)
 * @param tm List of Tempetures (Array of nc)
 * @param tmtable Table parameters (Chained with tmtable->next, NULL for end).
 * @param tmfuncs Function parameters (Chained with tmfuncs->next, NULL for end).
 * @param err if not NULL, sets errors by table_search.
 * @return calculated value.
 *
 * List tm is for materials that not referenced by none of table or func.
 */
static inline
type tm_calc_all(int nc, type *Y, type *tm, struct tm_table_param *tmtable,
                 struct tm_func2_param *tmfuncs, table_error *err,
                 type *x, type *y)
{
  type tval[nc];
  type yval[nc];
  int uflg[nc];
  int i;
  int nd; /* Number of materials which are defined by table or function */

  nd = 0;
  memset(uflg, 0, sizeof(int) * nc);

  tm_calc_table(nc, tmtable, Y, &nd, tval, yval, uflg, err, x, y);
  tm_calc_funcs2(nc, tmfuncs, Y, &nd, tval, yval, uflg);
  if (nd == 0) {
    return tm_calc_arithmetic_mean(nc, Y, tm);
  }
  for (i = 0; i < nc; ++i) {
    if (nd >= nc) break;
    if (!uflg[i]) {
      tval[nd] = tm[i];
      yval[nd] = Y[i];
      nd++;
    }
  }
  return tm_calc_arithmetic_mean(nd, yval, tval);
}

#ifdef __cplusplus
}
#endif

#endif
